#include <notojs/engine.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/module.hpp>
#include <notojs/writer.hpp>
#include <memory.hpp>
#include <bridge.hpp>
#include <global.hpp>
#include <notodb.hpp>

#include <notojs/detail/config.hpp>
#include <quickjs/quickjs-libc.h>
#include <rapidjson/rapidjson.h>
#include <dlfcn.h>
#include <iostream>

namespace notojs {
namespace {

template<typename W>
void error(JSContext *ctx, JSValue exc, W &writer)
{
    std::size_t len;
    JSValue name = JS_GetPropertyStr(ctx, exc, "name");
    if(!JS_IsUndefined(name))
    {
        const char *str = JS_ToCStringLen(ctx, &len, name);
        writer.startObject(str, len);
        JS_FreeCString(ctx, str);
    }
    else
    {
        writer.startObject("Error", 5);
    }
    JS_FreeValue(ctx, name);
    writer.startObject();

    JSValue message = JS_GetPropertyStr(ctx, exc, "message");
    if(!JS_IsUndefined(message))
    {
        const char *str = JS_ToCStringLen(ctx, &len, message);
        writer.string("message", str, len);
        JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, message);

    JSValue detail = JS_GetPropertyStr(ctx, exc, "detail");
    if(!JS_IsUndefined(detail))
    {
        const char *str = JS_ToCStringLen(ctx, &len, detail);
        writer.string("detail", str, len);
        JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, detail);

    JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
    if(!JS_IsUndefined(stack))
    {
        const char *str = JS_ToCStringLen(ctx, &len, stack);
        writer.string("stack", str, len);
        JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, stack);

    writer.endObject();
    writer.endObject();
}

template<typename W>
void json(JSContext *ctx, JSValue glob, JSValue value, W &writer)
{
    rapidjson::Type type;
    switch(JS_VALUE_GET_TAG(value))
    {
    case JS_TAG_OBJECT:
        type = JS_IsArray(ctx, value) ? rapidjson::kArrayType : rapidjson::kObjectType;
        break;
    case JS_TAG_STRING:
        type = rapidjson::kStringType;
        break;
    case JS_TAG_INT:
    case JS_TAG_FLOAT64:
        type = rapidjson::kNumberType;
        break;
    case JS_TAG_BOOL:
        type = JS_ToBool(ctx, value) ? rapidjson::kTrueType : rapidjson::kFalseType;
        break;
    default:
        writer.null();
        return;
    }

    JSValue json = JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED);

    std::size_t len;
    const char *cstr = JS_ToCStringLen(ctx, &len, json);
    writer.json(cstr, len, type);
    JS_FreeCString(ctx, cstr);

    JS_FreeValue(ctx, json);
}

BOOST_FORCEINLINE bool jschar(char ch, std::size_t idx)
{
    return ch == '_' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9' && idx > 0) || ((ch == ':' || ch == '!') && idx == 0);
}

} // namespace

void Engine::configure(boost::property_tree::ptree const &pt)
{
    constexpr auto convert = [](std::string &&value) -> std::filesystem::path {
        return std::move(value);
    };

    jspath = pt.get_optional<std::string>("engine.jspath").map(convert);
    sopath = pt.get_optional<std::string>("engine.sopath").map(convert);
}

JSModuleDef *Engine::loader(JSContext *ctx, const char *name, void *data)
{
    Engine *engine = reinterpret_cast<Engine *>(data);
    if(auto s = ::strlen(name); s > 3 && !::strcmp(name + s - 3, ".so"))
    {
        if(engine->sopath)
        {
            std::string const path = (*engine->sopath / name).string();
            if(void *handle = ::dlopen(path.c_str(), RTLD_NOW); handle)
            {
                if(js_module_init_fn init = (js_module_init_fn)::dlsym(handle, "js_init_module"); init)
                {
                    return init(ctx, name);
                }
                JS_ThrowReferenceError(ctx, "Module %s not loaded", name);
                return NULL;
            }
            JS_ThrowReferenceError(ctx, "Module %s not found", name);
            return NULL;
        }
    }
    else if(auto url = facade::URL::parse(name); url && !url->scheme().empty())
    {
        if(boost::urls::scheme::unknown != url->scheme_id())
        {
            return engine->get<Module>().load(ctx, Module::MODULE, std::move(*url), name);
        }
        else if("noto" == url->scheme())
        {
            if(auto def = engine->get<Module>().load(ctx, name); def)
            {
                return def;
            }
        }
    }
    if(auto s = ::strlen(name); s > 3 && !::strcmp(name + s - 3, ".js"))
    {
        if(auto const path = engine->get<Folder>().lib(name); path)
        {
            return engine->get<Module>().load(ctx, name, *path);
        }
        if(engine->jspath)
        {
            if(auto path = *engine->jspath / name; std::filesystem::is_regular_file(path))
                return engine->get<Module>().load(ctx, name, path);
            else if(auto file = path / "server.js"; std::filesystem::is_directory(path) && std::filesystem::is_regular_file(file))
                return engine->get<Module>().load(ctx, name, file);
        }
    }

    try {
        std::optional<boost::urls::url> url;
        lmdb::val k{name, std::strlen(name)};

        auto [tx, db] = DB(ctx).pkgs();
        if(lmdb::val v; db.get(tx, k, v) && detail::INI::is_module(v))
        {
            url = facade::URL::parse(v.data());
        }
        tx.abort();

        if(url)
        {
            if(auto mdef = engine->get<Module>().load(ctx, Module::MODULE, std::move(*url), name, true))
                return mdef;
        }
    } catch(std::runtime_error const &e) {
        JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        return NULL;
    }

    JS_ThrowReferenceError(ctx, "Module %s not found", name);
    return NULL;
}

void Engine::tracker(JSContext *ctx, JSValue, JSValue result, int handled, void*)
{
    auto *ptr = Global::Context::ptr(ctx);
    if(!handled && ptr && (JS_IsError(ctx, result) || JS_IsException(result)) && !ptr->perror)
        ptr->perror = JS_DupValue(ctx, result);
}

JSRuntime *Engine::get_runtime() const
{
    thread_local bridge::Runtime runtime(make_runtime());
    return runtime.get();
}

JSRuntime *Engine::make_runtime() const
{
    JSRuntime *rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc(rt, NULL, &Engine::loader, (void*)this);
    JS_SetHostPromiseRejectionTracker(rt, &Engine::tracker, NULL);
    JS_SetRuntimeOpaque(rt, (void*)(&get<Global>()));
    get<Global>().init(rt);
    get<Module>().init(rt);
    return rt;
}

template<typename W>
boost::beast::http::status Engine::eval(JSValue &mod, W &writer, JSContext *ctx, JSValue &glob, std::optional<std::string> &&name) const
{
    boost::beast::http::status result = boost::beast::http::status::ok;
    Global::Context &context = *Global::Context::ptr(ctx);
    writer.start();

    if(JS_IsException(mod))
    {
        JSValue exc = JS_GetException(ctx);
        if(JS_IsError(ctx, exc))
        {
            error(ctx, exc, writer);
        }
        JS_FreeValue(ctx, exc);
        result = boost::beast::http::status::bad_request;
    }
    else
    {
        JSValue prs = JS_EvalFunction(ctx, mod);
        context.wait(ctx);
        JSValue res = JS_PromiseResult(ctx, prs);

        bool error_{false};
        if(!JS_IsUndefined(res))
        {
            if(JS_IsError(ctx, res))
            {
                error_ = (error(ctx, res, writer), true);
            }
            else
            {
                writer.startObject("Error", 5);
                json(ctx, glob, res, writer);
                writer.endObject();
            }
        }
        else if(name)
        {
            JSValue ns = JS_GetModuleNamespace(ctx, (JSModuleDef*)JS_VALUE_GET_PTR(mod));

            std::uint32_t count;
            JSPropertyEnum *props;

            if(JS_GetOwnPropertyNames(ctx, &props, &count, ns, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) >= 0)
            {
                for(std::uint32_t i = 0; i < count; ++i)
                {
                    JSAtom atom = props[i].atom;
                    JS_SetProperty(ctx, glob, atom, JS_GetProperty(ctx, ns, atom));
                    JS_FreeAtom(ctx, atom);
                }
                js_free(ctx, props);
            }

            JS_FreeValue(ctx, ns);
        }

        if(!error_ && context.perror)
        {
            error(ctx, *context.perror, writer);
            error_ = true;
        }

        JSValue out = JS_JSONStringify(ctx, context.output, JS_UNDEFINED, JS_UNDEFINED);
        if(JS_IsException(out))
        {
            if(!error_)
            {
                JSValue exc = JS_GetException(ctx);
                writer.startObject("Error", 5);
                json(ctx, glob, exc, writer);
                writer.endObject();
                JS_FreeValue(ctx, exc);
            }
        }
        else
        {
            std::size_t len;
            char const *str = JS_ToCStringLen(ctx, &len, out);

            writer.startObject("notojs.Output", 13);
            writer.json(str, len, rapidjson::kArrayType);
            writer.endObject();
            writer.renderers(context.renderers);

            JS_FreeCString(ctx, str);
        }
        JS_FreeValue(ctx, out);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, prs);
    }

    writer.end();
    return result;
}

template<typename W>
boost::beast::http::status Engine::eval(std::string const &code, W &writer, JSContext *ctx, std::optional<std::string> &&name) const
{
    JSValue glob = JS_GetGlobalObject(ctx);
    auto context = get<Global>().make(ctx, glob);

    JSValue mod = JS_Eval(ctx, code.c_str(), code.size(), name.value_or("<cell>").c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    auto result = eval(mod, writer, ctx, glob, std::move(name));
    return (JS_FreeValue(ctx, glob), context->free(ctx), result);
}

template<typename W>
void Engine::eval(std::string const &code, detail::Bytecode &byte, W &writer, std::optional<std::string> &&name) const
{
    bridge::Context ctx{JS_NewContext(get_runtime())};
    eval(code, byte, writer, ctx.get(), std::move(name));
}

template<typename W>
void Engine::eval(std::string const &code, detail::Bytecode &byte, W &writer, JSContext *ctx, std::optional<std::string> &&name) const
{
    JSValue glob = JS_GetGlobalObject(ctx);
    auto context = get<Global>().make(ctx, glob);

    JSValue mod = JS_Eval(ctx, code.c_str(), code.size(), name.value_or("<cell>").c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if(JS_IsException(mod))
    {
        JSValue exc = JS_GetException(ctx);
        if(JS_IsError(ctx, exc))
        {
            error(ctx, exc, writer);
        }
        JS_FreeValue(ctx, exc);
    }
    else
    {
        std::size_t size;
        if(std::uint8_t *code = JS_WriteObject(ctx, &size, mod, JS_WRITE_OBJ_BYTECODE); code)
        {
            byte.assign(code, code + size);
            js_free(ctx, code);
        }
        else
        {
            writer.startObject("WriteObject", 11);
            writer.endObject();
        }
    }

    JS_FreeValue(ctx, mod);
    JS_FreeValue(ctx, glob);
    context->wait(ctx);
    context->free(ctx);
}

template<typename W>
boost::beast::http::status Engine::eval(detail::Bytecode const &byte, W &writer) const
{
    bridge::Context ctx{JS_NewContext(get_runtime())};
    return eval(byte, writer, ctx.get());
}

template<typename W>
boost::beast::http::status Engine::eval(detail::Bytecode const &code, W &writer, JSContext *ctx, std::optional<std::string> &&name) const
{
    JSValue glob = JS_GetGlobalObject(ctx);
    auto context = get<Global>().make(ctx, glob);

    JSValue mod = JS_ReadObject(ctx, code.data(), code.size(), JS_READ_OBJ_BYTECODE);
    if(!JS_IsException(mod) && 0 != JS_ResolveModule(ctx, mod))
    {
        writer.start();
        writer.startObject("Error", 5);
        writer.string("Resolving module");
        writer.endObject();
        writer.end();

        JS_FreeValue(ctx, glob);
        context->wait(ctx);
        context->free(ctx);
        return boost::beast::http::status::internal_server_error;
    }

    auto result = eval(mod, writer, ctx, glob, std::move(name));
    return (JS_FreeValue(ctx, glob), context->free(ctx), result);
}

template<typename W>
boost::beast::http::status Engine::eval(std::string const &code, W &writer) const
{
    bridge::Context ctx{JS_NewContext(get_runtime())};
    return eval(code, writer, ctx.get());
}

void Engine::set_sopath(std::filesystem::path &&path)
{
    sopath = std::move(path);
}

void Engine::set_jspath(std::filesystem::path &&path)
{
    jspath = std::move(path);
}

JSContext *Engine::get_context() const
{
    return JS_NewContext(get_runtime());
}

void Engine::preprocess(std::string &code)
{
    enum { JS, O1, O2, MD, C1, C2, C3 } state{JS};

    std::size_t ms{0};
    std::size_t me{0};
    std::size_t vs{0};
    std::size_t ve{0};
    std::size_t tm{0};

    for(std::size_t i = 0; i < code.size(); ++i)
    {
        if(code[i] == '`') tm = (++tm) & 0x1;
        else switch(state)
        {
        case JS:
            state = (!tm && code[i] == '<' && (i == 0 || code[i - 1] == '\n')) ? O1 : JS;
            break;
        case O1:
            state = (code[i] == '[') ? (vs = i + 1, O2) : JS;
            break;
        case O2:
            if(code[i] == '[') { ve = i; ms = (i + 1); state = MD; }
            else if(!jschar(code[i], i - vs)) state = JS;
            break;
        case MD:
            state = (!tm && code[i] == ']' && code[i - 1] == '\n') ? (me = i, C1) : MD;
            break;
        case C1:
            state = (code[i] == ']') ? C2 : MD;
            break;
        case C2:
            state = (code[i] == '>') ? C3 : MD;
            if(i != (code.size() - 1)) break;
        case C3:
            if(code[i] == '\n' || i == (code.size() - 1))
            {
                bool const name = (ve != vs);
                bool const echo = name && (1 == (ve - vs) && code[vs] == '!');

                if(echo)
                {
                    code[me] = ' ';
                    code[me + 1] = ' ';
                    code[me + 2] = ' ';
                }
                else
                {
                    code[me + 1] = '*';
                    code[me + 2] = '/';
                }

                std::string snippet;
                if(echo)
                {
                    snippet.append("print(new $.__Markdown(");

                    Writer writer{snippet};
                    writer.startObject();
                    std::string js{"```js!noplay\n"};
                    js.append(&code[ms + 1], (me - ms > 1) ? me - ms - 2 : 0);
                    js.append("\n```");
                    writer.string("data", js.c_str(), js.size());
                    writer.endObject();
                    writer.Flush();

                    snippet.append("));");
                }
                else if(name && (code[vs] != ':' || ve - vs > 1))
                {
                    if(code[vs] == '!')
                    {
                        snippet.append("export const ");
                        snippet.append(&code[vs + 1], ve - vs - 1);
                    }
                    else
                    {
                        snippet.append("const ");
                        snippet.append(&code[vs], ve - vs);
                    }
                    snippet.append(" = new $.__Markdown(");

                    Writer writer{snippet};
                    writer.startObject();
                    writer.string("data", &code[ms + 1], (me - ms > 1) ? me - ms - 2 : 0);
                    writer.endObject();
                    writer.Flush();

                    snippet.append(");");
                }
                else
                {
                    if(code[vs] == ':') snippet.append("print[':'](new $.__Markdown(");
                    else snippet.append("print(new $.__Markdown(");

                    Writer writer{snippet};
                    writer.startObject();
                    writer.string("data", &code[ms + 1], (me - ms > 1) ? me - ms - 2 : 0);
                    writer.endObject();
                    writer.Flush();

                    snippet.append("));");
                }
                if(!echo) snippet.append("/*");

                code.replace(vs - 2, echo ? 4 : 2, snippet);
                i += (snippet.size() - (echo ? 3 : 1));
                state = JS;
            }
            else
            {
                state = MD;
            }
            break;
        }
    }
}

void Engine::render(std::string const &name, std::string &body) const
{
    if(auto p = *jspath / name / "client.js"; std::filesystem::is_regular_file(p))
    {
        std::ifstream f(p, std::ios::binary);
        body.insert(0, "register(window.Handlers);</script>");
        body.insert(0, std::string{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()});
        body.insert(0, "<script type=\"module\">");
    }
}

void Engine::render(boost::beast::http::fields const &headers, std::string const &name,
    boost::beast::http::response<boost::beast::http::string_body> &response) const
{
    if(!jspath) response.result(boost::beast::http::status::not_found);
    else if(auto p = *jspath / name / "client.js"; std::filesystem::is_regular_file(p))
    {
        auto const etag = "\"TS-" + std::to_string(static_cast<std::uint64_t>(std::filesystem::last_write_time(p).time_since_epoch().count())) + "\"";
        if(auto it = headers.find(boost::beast::http::field::if_none_match); it == headers.end() || etag != it->value()) {
            std::ifstream f(p, std::ios::binary);
            std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()).swap(response.body());
        } else {
            response.result(boost::beast::http::status::not_modified);
        }
        response.set(boost::beast::http::field::content_type, "application/javascript");
        response.set(boost::beast::http::field::cache_control, "public, max-age=0, must-revalidate");
        response.set(boost::beast::http::field::etag, etag);
    }
    else response.result(boost::beast::http::status::not_found);
}

} // namespace notojs

template boost::beast::http::status notojs::Engine::eval<notojs::Silent>(detail::Bytecode const &, notojs::Silent &, JSContext *, std::optional<std::string> &&) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Silent>(std::string const &, notojs::Silent &, JSContext *, std::optional<std::string> &&) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Silent>(detail::Bytecode const &, notojs::Silent &) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Silent>(std::string const &, notojs::Silent &) const;
template void notojs::Engine::eval<notojs::Silent>(std::string const &, detail::Bytecode &, notojs::Silent &, std::optional<std::string> &&) const;
template void notojs::Engine::eval<notojs::Silent>(std::string const &, detail::Bytecode &, notojs::Silent &, JSContext *ctx, std::optional<std::string> &&) const;

template boost::beast::http::status notojs::Engine::eval<notojs::Writer>(detail::Bytecode const &, notojs::Writer &, JSContext *, std::optional<std::string> &&) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Writer>(std::string const &, notojs::Writer &, JSContext *, std::optional<std::string> &&) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Writer>(detail::Bytecode const &, notojs::Writer &) const;
template boost::beast::http::status notojs::Engine::eval<notojs::Writer>(std::string const &, notojs::Writer &) const;
