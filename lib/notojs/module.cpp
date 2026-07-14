#include <notojs/module.hpp>
#include <notojs/engine.hpp>
#include <notojs/global.hpp>
#include <notojs/server.hpp>
#include <notojs/logger.hpp>

#include <boost/beast.hpp>
#include <boost/config.hpp>
#include <boost/hana.hpp>

#include <quickjs/quickjs-libc.h>

#include <notojs/detail/jscode.hpp>

#include <notojs/module/assert.hpp>
#include <notojs/module/core.hpp>
#include <notojs/module/db.hpp>
#include <notojs/module/doc.hpp>
#include <notojs/module/dom.hpp>
#include <notojs/module/fs.hpp>
#ifdef WITH_LIBZIP
#include <notojs/module/zip.hpp>
#endif
#ifdef WITH_ZLIB
#include <notojs/module/gzip.hpp>
#endif

#include <lmdbxx/lmdb++.h>
#include <notodb.hpp>

#include <future>
#include <fstream>
#include <iostream>
#include <shared_mutex>

namespace notojs  {
namespace {

using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;
using SSLStream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

thread_local std::unordered_map<std::string, detail::Version> versions;
thread_local std::unordered_map<std::string, detail::Bytecode> bytecode;

void cache_save(const char *name, std::uint8_t *code, std::size_t size)
{
    bytecode.emplace(name, detail::Bytecode(code, code + size));
}

void cache_save(std::string const &name, std::uint8_t *code, std::size_t size, detail::Version version)
{
    bytecode[name].assign(code, code + size);
    versions[name] = version;
}

JSModuleDef *cache_load(JSContext *ctx, decltype(Module::MODULE), const char *name)
{
    if(auto it = bytecode.find(name); it != std::end(bytecode))
    {
        JSValue v = JS_ReadObject(ctx, it->second.data(), it->second.size(), JS_READ_OBJ_BYTECODE);
        if(!JS_IsException(v) && 0 == JS_ResolveModule(ctx, v))
        {
            JSModuleDef *m = (JSModuleDef*)JS_VALUE_GET_PTR(v);
            JS_FreeValue(ctx, v);
            return m;
        }
        JS_FreeValue(ctx, v);
    }
    return NULL;
}

std::optional<JSValue> cache_load(JSContext *ctx, decltype(Module::SCRIPT), const char *name)
{
    if(auto it = bytecode.find(name); it != std::end(bytecode))
    {
        JSValue v = JS_ReadObject(ctx, it->second.data(), it->second.size(), JS_READ_OBJ_BYTECODE);
        if(!JS_IsException(v))
        {
            return JS_EvalFunction(ctx, v);
        }
        JS_FreeValue(ctx, v);
    }
    return std::nullopt;
}

JSModuleDef *cache_load(JSContext *ctx, std::string const &name, detail::Version version)
{
    if(auto it = versions.find(name); it == std::end(versions) || it->second  >= version)
    {
        if(auto jt = bytecode.find(name); jt != std::end(bytecode))
        {
            JSValue v = JS_ReadObject(ctx, jt->second.data(), jt->second.size(), JS_READ_OBJ_BYTECODE);
            if(!JS_IsException(v) && 0 == JS_ResolveModule(ctx, v))
            {
                JSModuleDef *m = (JSModuleDef*)JS_VALUE_GET_PTR(v);
                JS_FreeValue(ctx, v);
                return m;
            }
            JS_FreeValue(ctx, v);
        }
    }
    return NULL;
}

template<typename S, typename F>
auto http_response(JSContext *ctx, const char * name, boost::urls::url &&url, S &&stream, F &&callback)
-> decltype(callback(std::declval<boost::urls::url &&>(), std::declval<std::string &&>()))
{
    std::string path = url.path();
    if(auto q = url.encoded_query(); !q.empty())
    {
        path.append("?", 1);
        path.append(q.data(), q.size());
    }

    boost::system::error_code ec;

    auto request = Request{boost::beast::http::verb::get, path, 11};
    request.set(boost::beast::http::field::host, url.host());
    if(boost::beast::http::write(stream, request, ec); ec)
        return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %s", name, ec.message().c_str()));

    Response response;
    boost::asio::streambuf buffer;
    if(boost::beast::http::read(stream, buffer, response, ec); ec)
        return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %s", name, ec.message().c_str()));

    if(boost::beast::http::status::ok != response.result())
    {
        auto const reason = boost::beast::http::obsolete_reason(response.result());
        return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %d %s", name, response.result_int(), reason.data()));
    }

    if constexpr (std::is_same_v<S, SSLStream>)
        return callback(std::move(url), std::move(response.body()));
    else
        return callback(std::move(url), std::move(response.body()));
}

template<typename F>
auto http_request(JSContext *ctx, const char * name, boost::urls::url &&url, Server &server, bool cache, F &&callback)
-> decltype(callback(std::declval<boost::urls::url &&>(), std::declval<std::string &&>()))
{
    auto scheme = url.scheme_id(); 
    if(boost::urls::scheme::https == scheme)
    {
        if(cache) try
        {
            std::string code;
            lmdb::val k{url.buffer().data(), url.buffer().size() + 1};
            auto [tx, db] = DB(ctx).http();
            if(lmdb::val v; db.get(tx, k, v))
                std::string(v.data()).swap(code);
            tx.abort();
            if(!code.empty())
                return callback(std::move(url), std::move(code));
        } catch(...) {}
    }
    else if(boost::urls::scheme::http != scheme)
    {
        std::string s{std::begin(url.scheme()), std::end(url.scheme())};
        return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: unsupported scheme [%s]", name, s.c_str()));
    }

    std::string port = url.port();
    if(port.empty())
        port = std::to_string(boost::urls::scheme::http == scheme ? 80 : 443);

    boost::system::error_code ec;

    auto const results = server.resolver.resolve(url.host(), port, ec);
    if(ec) return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %s", name, ec.message().c_str()));

    if(boost::urls::scheme::http == scheme)
    {
        boost::beast::tcp_stream stream(server.resolver.get_executor());
        if(stream.connect(results, ec); ec) return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %s", name, ec.message().c_str()));
        else return http_response(ctx, name, std::move(url), std::move(stream), std::move(callback));
    }

    SSLStream stream{server.resolver.get_executor(), server.sslcontext};
    if(!SSL_set_tlsext_host_name(stream.native_handle(), url.host().data()))
    {
        auto err = ERR_get_error();
        char buf[256];

        ERR_error_string_n(err, buf, sizeof(buf));
        return callback(JS_ThrowReferenceError(ctx, "Module %s not loaded: %s", name, &buf[0]));
    }

    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(boost::asio::ssl::stream_base::client);

    if(cache && boost::urls::scheme::https == scheme)
    {
        return http_response(ctx, name, std::move(url), std::move(stream), boost::hana::overload_linearly(
            [ctx, callback](boost::urls::url &&url, std::string &&code) {
                try
                {
                    auto [tx, db] = DB(ctx).http(DB::RW);
                    lmdb::val k{url.buffer().data(), url.buffer().size() + 1};
                    lmdb::val v{code.data(), code.size() + 1};
                    db.put(tx, k, v);
                    tx.commit();
                } catch(...) {}
                return callback(std::move(url), std::move(code));
            },
            [callback](JSValue v) { return callback(v); }
        ));
    }
    else
    {
        return http_response(ctx, name, std::move(url), std::move(stream), std::move(callback));
    }
}

} // namespace

Module::Module()
: modules{
#define MODULE(name) {#name, { \
    &notojs_init_##name,\
    &notojs_init_##name,\
    &notojs_init_##name,\
    &notojs_init_##name \
}}
    MODULE(assert),
    MODULE(core),
    MODULE(db),
    MODULE(doc),
    MODULE(dom),
    MODULE(fs)
#ifdef WITH_ZLIB
    , MODULE(gzip)
#endif
#ifdef WITH_LIBZIP
    , MODULE(zip)
#endif
#undef MODULE
}
{
    for(auto const &[_, m] : modules) m.init0();
}

void Module::configure(boost::property_tree::ptree const &cfg)
{
    for(auto const &[_, m] : modules) m.init2(cfg);
}

void Module::init(JSRuntime *rt) const
{
    for(auto const &[_, m] : modules) m.init1(rt);
}

JSModuleDef *Module::load(JSContext *ctx, const char *name)
{
    if(auto it = modules.find(name + 5); it != modules.end())
    {
        return it->second.init3(ctx, name);
    }
    if(auto it = plugins.find(name + 5); it != plugins.end())
    {
        return it->second(ctx, name);
    }
    return NULL;
}

JSValue Module::load(JSContext *ctx, decltype(SCRIPT), boost::urls::url &&url, const char *name, bool cache)
{
    if(std::optional<JSValue> cached = cache_load(ctx, Module::SCRIPT, url.buffer().data()); cached)
    {
        return *cached;
    }

    auto ts = std::chrono::system_clock::now();
    return http_request(ctx, name, std::move(url), get<Server>(), cache, boost::hana::overload_linearly(
        [this, ctx, name, ts](boost::urls::url &&url, std::string &&code) -> JSValue {
            JSValue fn = JS_Eval(ctx, code.c_str(), code.size(), name, JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
            if(JS_IsError(ctx, fn)) return fn;

            NOTOJS_LOG("Script resolved",
                (std::string(name))
                (std::this_thread::get_id())
                (std::chrono::system_clock::now() - ts)
                (code.size()));

            if(boost::urls::scheme::https == url.scheme_id())
            {
                std::size_t size;
                if(std::uint8_t *code = JS_WriteObject(ctx, &size, fn, JS_WRITE_OBJ_BYTECODE); code)
                {
                    cache_save(url.c_str(), code, size);
                    js_free(ctx, code);
                }

                NOTOJS_LOG("Script compiled",
                    (std::string(name))
                    (std::this_thread::get_id())
                    (std::chrono::system_clock::now() - ts)
                    (size));
            }
            return JS_EvalFunction(ctx, fn);
        },
        [](JSValue v) -> JSValue { return v; }
    ));
}

JSModuleDef *Module::load(JSContext *ctx, decltype(Module::MODULE), boost::urls::url &&url, const char *name, bool cache)
{
    if(JSModuleDef *cached = cache_load(ctx, Module::MODULE, url.buffer().data()); cached)
    {
        return cached;
    }

    auto ts = std::chrono::system_clock::now();
    return http_request(ctx, name, std::move(url), get<Server>(), cache, boost::hana::overload_linearly(
        [this, ctx, name, ts](boost::urls::url &&url, std::string &&code) -> JSModuleDef* {
            JSValue fn = JS_Eval(ctx, code.c_str(), code.size(), name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            if(JS_IsError(ctx, fn))
            {
                JS_FreeValue(ctx, fn);
                return NULL;
            }

            NOTOJS_LOG("Module resolved",
                (std::string(name))
                (std::this_thread::get_id())
                (std::chrono::system_clock::now() - ts)
                (code.size()));

            if(boost::urls::scheme::https == url.scheme_id())
            {
                std::size_t size;
                if(std::uint8_t *code = JS_WriteObject(ctx, &size, fn, JS_WRITE_OBJ_BYTECODE); code)
                {
                    cache_save(url.c_str(), code, size);
                    js_free(ctx, code);
                }

                NOTOJS_LOG("Module compiled",
                    (std::string(name))
                    (std::this_thread::get_id())
                    (std::chrono::system_clock::now() - ts)
                    (size));
            }
            JSModuleDef *m = (JSModuleDef*)JS_VALUE_GET_PTR(fn);
            JS_FreeValue(ctx, fn);
            return m;
        },
        [](JSValue) -> JSModuleDef* { return NULL; }
    ));
}

JSModuleDef *Module::load(JSContext *ctx, const char *name, std::filesystem::path const &path)
{
    if(JSModuleDef *cached = cache_load(ctx, path.u8string(), std::filesystem::last_write_time(path)); cached)
    {
        return cached;
    }

    auto const ts = std::chrono::system_clock::now();
    using M = std::tuple<detail::Version, std::string>;

    auto [version, content] = std::invoke([&path, ctx, this]{
        std::promise<M> code;
        auto fut = code.get_future();
        boost::asio::post(*get<Server>().disk, [&]{
            std::ifstream stream(path);
            code.set_value(M{
                std::filesystem::last_write_time(path),
                std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>())
            });
        });
        return fut.get();
    });

    Engine::preprocess(content);

    JSValue fn = JS_Eval(ctx, content.c_str(), content.size(), name,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if(JS_IsException(fn))
    {
        JS_FreeValue(ctx, fn);
        return NULL;
    }
    else
    {
        std::size_t size;
        if(std::uint8_t *code = JS_WriteObject(ctx, &size, fn, JS_WRITE_OBJ_BYTECODE); code)
        {
            cache_save(path, code, size, version);
            js_free(ctx, code);
        }

        JSModuleDef *m = (JSModuleDef*)JS_VALUE_GET_PTR(fn);
        JS_FreeValue(ctx, fn);

        NOTOJS_LOG("Module compiled",
            (path.filename().string())
            (std::this_thread::get_id())
            (std::chrono::system_clock::now() - ts)
            (size));

        return m;
    }
}

} // namespace notojs
