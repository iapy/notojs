#include <notojs/detail/header.hpp>
#include <notojs/detail/router.hpp>
#include <notojs/detail/cellid.hpp>
#include <notojs/detail/notodb.hpp>
#include <notojs/cacher.hpp>
#include <notojs/engine.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/handle.hpp>
#include <notojs/logger.hpp>
#include <notojs/router.hpp>
#include <notojs/server.hpp>
#include <notojs/window.hpp>
#include <notojs/writer.hpp>
#include <memory.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/url/parse.hpp>
#include <lmdbxx/lmdb++.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <utility>

namespace notojs {

extern const std::string_view CLIENT_JS;
extern const std::string_view CTYPES_JS;

extern const std::string_view EDITOR_CSS;
extern const std::string_view EDITOR_JS;

extern const std::string_view ANDROID_CHROME_192X192_PNG;
extern const std::string_view ANDROID_CHROME_512X512_PNG;
extern const std::string_view APPLE_TOUCH_ICON_PNG;
extern const std::string_view FAVICON_ICO;
extern const std::string_view FAVICON_16X16_PNG;
extern const std::string_view FAVICON_32X32_PNG;
extern const std::string_view KEYMAP_INI;
extern const std::string_view LOGO_PNG;
extern const std::string_view NOTOJS_CSS;
extern const std::string_view NOTOJS_JS;
extern const std::string_view NOTOUI_JS;
extern const std::string_view SITE_WEBMANIFEST;

extern const std::string_view WINDOW_CSS;
extern const std::string_view WINDOW_JS;

extern const std::string_view CODEMIRROR_CSS;
extern const std::string_view CODEMIRROR_JS;
extern const std::string_view ICONS_CSS;
extern const std::string_view ICONS_TTF;
extern const std::string_view JAVASCRIPT_JS;
extern const std::string_view MARKDOWN_JS;

extern const std::string_view ETAG;

namespace {

BOOST_FORCEINLINE void wrap(std::string &body, Engine const &engine, std::unordered_set<std::string> const &render)
{
    body.insert(0, "<script type=\"module\">Promise.all(");
    for(auto const &r: render) engine.render(r.substr(r.rfind('/') + 1), body);
    body.insert(0, "window.render=render;window.Handlers=Handlers;</script>");
    body.insert(0, NOTOJS_JS.data(), NOTOJS_JS.size());
    body.insert(0, "div.nj-block{margin:auto}nj-view{margin-left:auto;margin-right:auto;max-width:var(--nj-max-editor-width)}nj-view>div.nj-block{margin:unset}</style></head><body class=\"nj-page\"><div class=\"nj-output\"></div></body><script type=\"module\">");
    body.insert(0, NOTOJS_CSS.data(), NOTOJS_CSS.size());
    body.insert(0, "<html><head><meta charset=\"utf-8\"/><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1 user-scalable=no\"/><style>");
    body.insert(0, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    body.append(".map((d) => window.render(document.querySelector(\".nj-output\"), d))).then(_ => document.querySelectorAll(\"nj-view\").forEach(e => e.resize()))</script></html>");
}

BOOST_FORCEINLINE std::array<std::string, 2> parse_kv(std::string const &tail)
{
    std::array<std::string, 2> parts;
    if(auto pos = tail.find('/'); pos == std::string::npos)
    {
        parts[1] = tail;
    }
    else
    {
        parts[0] = tail.substr(0, pos);
        parts[1] = tail.substr(1 + pos);
    }
    if(parts[0] == "-") parts[0].clear();
    return parts;
}

constexpr auto router = detail::router(
    detail::route("/client.js"),
    detail::route("/ctypes.js"),
    detail::route("/editor.js"),
    detail::route("/editor.css"),
    detail::route("/notojs.js"),
    detail::route("/notojs.css"),
    detail::route("/notoui.js"),
    detail::route("/window.js"),
    detail::route("/window.css"),

    detail::route("/android-chrome-192x192.png"),
    detail::route("/android-chrome-512x512.png"),
    detail::route("/apple-touch-icon.png"),
    detail::route("/codemirror.css"),
    detail::route("/codemirror.js"),
    detail::route("/favicon-16x16.png"),
    detail::route("/favicon-32x32.png"),
    detail::route("/favicon.ico"),
    detail::route("/icons.css"),
    detail::route("/icons.ttf"),
    detail::route("/javascript.js"),
    detail::route("/logo.png"),
    detail::route("/markdown.js"),
    detail::route("/site.webmanifest"),

    detail::route("/"),
    detail::route("/api/v1/app/"),
    detail::route("/a/"),
    detail::route("/api/v1/folder/"),
    detail::route("/f/"),
    detail::route("/api/v1/packages"),
    detail::route("/p"),
    detail::route("/api/v1/result/"),
    detail::route("/r/"),
    detail::route("/api/v1/storage/"),
    detail::route("/s/"),
    detail::route("/api/v1/kernel/"),
    detail::route("/k/"),
    detail::route("/api/v1/keymap"),
    detail::route("/m"),
    detail::route("/api/v1/window"),
    detail::route("/w"),
    detail::route("/api/v1/log/rotate"),

    detail::route("/notojs.Render/")
);

using Static = std::tuple<std::string_view const *, std::string_view>;
constexpr std::array<Static, router["/"]> static_{{
    {&CLIENT_JS,       "application/javascript"},
    {&CTYPES_JS,       "application/javascript"},
    {&EDITOR_JS,       "application/javascript"},
    {&EDITOR_CSS,      "text/css"},
    {&NOTOJS_JS,       "application/javascript"},
    {&NOTOJS_CSS,      "text/css"},
    {&NOTOUI_JS,       "application/javascript"},
    {&WINDOW_JS,       "application/javascript"},
    {&WINDOW_CSS,      "text/css"},

    {&ANDROID_CHROME_192X192_PNG, "image/png"},
    {&ANDROID_CHROME_512X512_PNG, "image/png"},
    {&APPLE_TOUCH_ICON_PNG, "image/png"},
    {&CODEMIRROR_CSS,       "text/css"},
    {&CODEMIRROR_JS,        "application/javascript"},
    {&FAVICON_ICO,          "image/x-icon"},
    {&FAVICON_16X16_PNG,    "image/png"},
    {&FAVICON_32X32_PNG,    "image/png"},
    {&ICONS_CSS,            "text/css"},
    {&ICONS_TTF,            "font/ttf"},
    {&JAVASCRIPT_JS,        "application/javascript"},
    {&LOGO_PNG,             "image/png"},
    {&MARKDOWN_JS,          "application/javascript"},
    {&SITE_WEBMANIFEST,     "application/manifest+json"}
}};

thread_local Cacher::Cached applications;

BOOST_FORCEINLINE void set_headers(Socket &handle, std::thread::id t, std::chrono::system_clock::duration d)
{
    static std::hash<std::thread::id> hash;
    handle.response.set(detail::EXEC_TIME, std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(d).count()
    ) + "ms");
    handle.response.set(detail::EXEC_PROC, std::to_string(hash(t) % 10000));
}

class Render : Writer
{
public:
    Render(std::unordered_set<std::string> &target, std::string &output)
    : Writer(output)
    , target{target}
    {}

    virtual void renderers(std::unordered_set<std::string> const &rs) override
    {
        for(auto const &r : rs) target.insert(r);
        Writer::renderers(rs);
    }

    BOOST_FORCEINLINE Writer &base() { return *this; }

private:
    std::unordered_set<std::string> &target;
};

} // namespace

void Router::jsexec(std::shared_ptr<Handle> handle, Engine const &engine, boost::asio::thread_pool &pool, detail::Textcode &&code, Flag flag) const
{
    boost::asio::post(pool, [this, flag, handle, &engine, code=std::move(code)]() mutable {
        auto const ts = std::chrono::system_clock::now();
        bridge::Context ctx{engine.get_context()};

        auto const it = handle->parser.get().find(boost::beast::http::field::prefer);
        flag.skip = it != handle->parser.get().end() && "return=minimal" == it->value();

        if(auto const &body = handle->parser.get().body(); !body.empty())
        {
            JSValue glob = JS_GetGlobalObject(ctx.get());
            if(auto it = handle->parser.get().find(boost::beast::http::field::content_type);
                it != handle->parser.get().end() && "application/json" == it->value())
            {
                JS_SetPropertyStr(ctx.get(), glob, "input", JS_ParseJSON(ctx.get(), body.c_str(), body.size(), "<input>"));
            }
            else
            {
                JS_SetPropertyStr(ctx.get(), glob, "input", JS_NewStringLen(ctx.get(), body.c_str(), body.size()));
            }
            JS_FreeValue(ctx.get(), glob);
        }

        std::unordered_set<std::string> render;
        Writer global{handle->response.body()};
        std::optional<Folder::File> file;
        if(!flag.name.empty())
        {
            file.emplace(flag.name, get<Folder>());
        }
        if(!flag.skip)
        {
            if(flag.html) handle->response.set(boost::beast::http::field::content_type, "text/html");
            else handle->response.set(boost::beast::http::field::content_type, "application/json");
            global.start();
        }
        for(std::size_t i = 0; i < code.size(); ++i)
        {
            std::string output;
            Render w{render, output};

            bool const disabled = detail::disabled(code[i]);
            if(file) file->code(code[i], !disabled);
            if(disabled) continue;

            Engine::preprocess(code[i]);
            engine.eval(code[i], w.base(), ctx.get(), detail::cell_id(i));
            if(file) file->data(output);
            if(!flag.skip) global.string(std::move(output));
        }
        if(!flag.skip)
        {
            global.end();
            if(flag.html) wrap(handle->response.body(), engine, render);
        }

        if(file) boost::asio::post(*get<Server>().disk, [file=std::move(*file)]{
            file.save();
            std::clog << "Updated" << notojs::values(std::this_thread::get_id(), file.name());
        });

        set_headers(*handle, std::this_thread::get_id(), std::chrono::system_clock::now() - ts);
        handle->finish();
    });
}

void Router::jsexec(std::shared_ptr<Handle> handle, Engine const &engine, boost::asio::thread_pool &pool, boost::urls::url_view const &url)
{
    auto const params = url.params();
    std::optional<std::string> wid;
    std::optional<std::string> pid;

    if(auto it = params.find("wid"); it != params.end())
        wid = (*it).value;

    if(auto it = params.find("pid"); it != params.end())
        pid = (*it).value;

    if(!pid || !wid || !get<Window>().jsexec(handle, *wid, *pid))
    {
        boost::asio::post(pool, [handle, &engine]{
            handle->response.set(boost::beast::http::field::content_type, "application/json");

            Writer writer{handle->response.body()};
            auto const ts = std::chrono::system_clock::now();

            Engine::preprocess(handle->parser.get().body());
            handle->response.result(engine.eval(handle->parser.get().body(), writer));

            set_headers(*handle, std::this_thread::get_id(), std::chrono::system_clock::now() - ts);
            handle->finish();
        });
    }
}

void Router::jsexec(std::shared_ptr<Handle> handle, Engine const &engine, boost::asio::thread_pool &pool, detail::Version &&version, std::string &&name, std::string &&path)
{
    if(auto const p = handle->parser.get().target().find_first_of('?'); p != std::string::npos)
        handle->parser.get().target(std::move(path) + std::string(handle->parser.get().target().substr(p)));
    else handle->parser.get().target(std::move(path));

    boost::asio::post(pool, [handle, version=std::move(version), &engine, name=std::move(name), this]() mutable {
        auto [it, id, ts] = get<Cacher>().cache(applications, version, name);
        if(it != std::end(applications))
        {
            bridge::Context ctx{engine.get_context()};

            Silent::Output error;
            for(std::size_t i = 0; i < it->second.code->size(); ++i)
            {
                Silent w{error};
                if(i == it->second.code->size() - 1) Global::set_handle(ctx.get(), *handle);
                if(engine.eval(it->second.code->at(i), w, ctx.get(), detail::cell_id(i)); error)
                {
                    handle->response.result(boost::beast::http::status::internal_server_error);
                    handle->response.set(boost::beast::http::field::content_type, "application/json");
                    handle->response.body() = std::move(error->json);
                    break;
                }
            }
        }
        else
        {
            handle->response.result(boost::beast::http::status::not_found);
        }

        set_headers(*handle, std::this_thread::get_id(), std::chrono::system_clock::now() - ts);
        handle->finish();
    });
}

bool Router::storage(std::shared_ptr<Handle> &&handle, std::string const &tail, boost::asio::thread_pool &pool, boost::urls::url_view const &url)
{
    static constexpr std::string_view prefix{"sys:storage:"};
    static constexpr auto dump = [](lmdb::cursor &cr, Writer &wr, lmdb::val const *ns)
    {
        lmdb::val k, v;
        if(cr.get(k, v, MDB_FIRST)) do {
            wr.startObject();
            if(ns) wr.string("ns", ns->data() + prefix.size(), ns->size() - prefix.size());
            wr.string("key", k.data(), k.size());
            if(detail::is_object<true>(v))
            {
                rapidjson::Document d;
                d.Parse(v.data(), v.size() - 1);
                if(d.HasParseError())
                {
                    wr.integer("type", 2);
                    wr.string("value", v.data(), v.size() - 1);
                }
                else
                {
                    wr.integer("type", 1);
                    wr.json("value", v.data(), v.size() - 1, d.GetType());
                }
            }
            else
            {
                wr.integer("type", 0);
                wr.string("value", v.data(), v.size());
            }
            wr.endObject();
        } while(cr.get(k, v, MDB_NEXT));
    };

    switch(handle->parser.get().method())
    {
    case boost::beast::http::verb::get:
        if(tail.empty()) boost::asio::post(pool, [this, handle, url]{
            NOTOJS_LOG_THIS_THREAD
            try {
                auto const js = std::invoke([&]{
                    auto const params = url.params();
                    if(auto it = params.find("var"); it != std::end(params)) {
                        handle->response.body() = ("var " + (*it).value + " = ");
                        return true;
                    }
                    return false;
                });

                Writer writer{handle->response.body()};
                writer.start();

                lmdb::val k{prefix.data(), prefix.size()}, v;

                auto tx = lmdb::txn::begin(get<Folder>().env(), nullptr, MDB_RDONLY);
                auto db = lmdb::dbi::open(tx, nullptr);
                auto cr = lmdb::cursor::open(tx, db);

                if(cr.get(k, v, MDB_SET_RANGE)) do {
                    if(k.size() < prefix.size() || std::memcmp(k.data(), prefix.data(), prefix.size()) != 0)
                        break;

                    auto dbx = lmdb::dbi::open(tx, k.data());
                    auto crx = lmdb::cursor::open(tx, dbx);
                    dump(crx, writer, &k);
                    crx.close();
                } while(cr.get(k, v, MDB_NEXT));

                cr.close();
                tx.abort();

                writer.end();
                if(js) {
                    handle->response.set(boost::beast::http::field::content_type, "application/javascript");
                    handle->response.body().append(";", 1);
                } else {
                    handle->response.set(boost::beast::http::field::content_type, "application/json");
                }
            } catch(...) {
                handle->response.result(boost::beast::http::status::internal_server_error);
            }
            boost::asio::post(handle->socket.get_executor(), [handle]{
                handle->response.prepare_payload();
                handle->finish(nullptr);
            });
        });
        else boost::asio::post(pool, [this, handle, tail, url]{
            NOTOJS_LOG_THIS_THREAD
            auto const part = parse_kv(tail);
            try {
                if(part[0].empty()) {
                    auto const js = std::invoke([&]{
                        auto const params = url.params();
                        if(auto it = params.find("var"); it != std::end(params)) {
                            handle->response.body() = ("var " + (*it).value + " = ");
                            return true;
                        }
                        return false;
                    });

                    Writer writer{handle->response.body()};
                    writer.start();

                    auto tx = lmdb::txn::begin(get<Folder>().env(), nullptr, MDB_RDONLY);
                    auto db = lmdb::dbi::open(tx, (std::string{prefix} + part[1]).c_str());
                    auto cr = lmdb::cursor::open(tx, db);
                    dump(cr, writer, nullptr);
                    cr.close();
                    tx.abort();

                    writer.end();
                    if(js) {
                        handle->response.set(boost::beast::http::field::content_type, "application/javascript");
                        handle->response.body().append(";", 1);
                    } else {
                        handle->response.set(boost::beast::http::field::content_type, "application/json");
                    }
                } else {
                    auto tx = lmdb::txn::begin(get<Folder>().env(), nullptr, MDB_RDONLY);
                    auto db = lmdb::dbi::open(tx, (std::string{prefix} + part[0]).c_str());
                    lmdb::val k{part[1].data(), part[1].size()};
                    if(lmdb::val v; db.get(tx, k, v))
                    {
                        if(detail::is_object<true>(v))
                        {
                            auto const js = std::invoke([&]{
                                auto const params = url.params();
                                if(auto it = params.find("var"); it != std::end(params)) {
                                    handle->response.body() = ("var " + (*it).value + " = ");
                                    return true;
                                }
                                return false;
                            });
                            handle->response.set(boost::beast::http::field::content_type, js
                                ? "application/javascript"
                                : "application/json");
                            handle->response.body().append(v.data(), v.size() - 1);
                            if(js) handle->response.body().append(";", 1);
                        }
                        else
                        {
                            handle->response.set(boost::beast::http::field::content_type, "text/plain");
                            handle->response.body().assign(v.data(), v.size());
                        }
                    }
                    else
                    {
                        handle->response.result(boost::beast::http::status::not_found);
                    }
                    tx.abort();
                }
            } catch(lmdb::error const &e) {
                if(MDB_NOTFOUND == e.code()) handle->response.result(boost::beast::http::status::not_found);
                else handle->response.result(boost::beast::http::status::internal_server_error);
            } catch(...) {
                handle->response.result(boost::beast::http::status::internal_server_error);
            }
            boost::asio::post(handle->socket.get_executor(), [handle]{
                handle->response.prepare_payload();
                handle->finish(nullptr);
            });
        });
        break;
    case boost::beast::http::verb::put: boost::asio::post(pool, [this, handle, tail]{
            NOTOJS_LOG_THIS_THREAD
            auto part = parse_kv(tail);
            if(part[0].empty() || part[1].empty()) {
                handle->response.result(boost::beast::http::status::not_found);
            }
            else try {
                auto const it = handle->parser.get().find(boost::beast::http::field::content_type);
                auto const json = (it != std::end(handle->parser.get()) && it->value() == "application/json");

                if(json)
                {
                    rapidjson::Document d;
                    d.Parse(handle->parser.get().body().data(), handle->parser.get().body().size());
                    if(d.HasParseError())
                    {
                        handle->response.result(boost::beast::http::status::bad_request);
                        handle->response.body() = rapidjson::GetParseError_En(d.GetParseError());
                    }
                }

                lmdb::val lk{part[1].data(), part[1].size()};
                lmdb::val lv{handle->parser.get().body().c_str(), handle->parser.get().body().size() + json};

                auto tx = lmdb::txn::begin(get<Folder>().env());
                auto db = lmdb::dbi::open(tx, (std::string{prefix} + part[0]).c_str());
                db.put(tx, lk, lv);
                tx.commit();
            } catch(...) {
                handle->response.result(boost::beast::http::status::internal_server_error);
            }
            boost::asio::post(handle->socket.get_executor(), [handle]{
                handle->response.prepare_payload();
                handle->finish(nullptr);
            });
        });
        break;
    case boost::beast::http::verb::delete_: boost::asio::post(pool, [this, handle, tail]{
            NOTOJS_LOG_THIS_THREAD
            auto const part = parse_kv(tail);
            if(part[0].empty() && !part[1].empty()) try {
                auto tx = lmdb::txn::begin(get<Folder>().env());
                auto db = lmdb::dbi::open(tx, (std::string{prefix} + part[1]).c_str());
                db.drop(tx, true);
                tx.commit();
            } catch(...) {
                handle->response.result(boost::beast::http::status::internal_server_error);
            }
            else if(!part[1].empty()) try {
                auto tx = lmdb::txn::begin(get<Folder>().env());
                auto db = lmdb::dbi::open(tx, (std::string{prefix} + part[0]).c_str());
                lmdb::val lk{part[1].data(), part[1].size()};
                db.del(tx, lk);
                tx.commit();
            } catch(...) {
                handle->response.result(boost::beast::http::status::internal_server_error);
            }
            else handle->response.result(boost::beast::http::status::not_found);
            boost::asio::post(handle->socket.get_executor(), [handle]{
                handle->response.prepare_payload();
                handle->finish(nullptr);
            });
        });
        break;
    default:
        handle->response.result(boost::beast::http::status::method_not_allowed);
        return false;
    };
    return true;
}

void Router::window(std::string_view &&sv)
{
    window_ = std::move(sv);
}

void Router::rotate(std::function<std::string()> &&rf)
{
    rotate_ = std::move(rf);
}

void Router::route(std::shared_ptr<Handle> handle, Engine &engine, boost::asio::thread_pool &pool)
{
    std::string_view const *body = nullptr;

    handle->response.set(boost::beast::http::field::server, "boost:beast");
    if(auto url = boost::urls::parse_relative_ref(handle->parser.get().target()); !url)
    {
        handle->response.result(boost::beast::http::status::bad_request);
    }
    else
    {
        std::string tail;
        std::size_t const path = router.find(url->path(), tail);
        switch(path)
        {
        case router["/"]:
            if(!tail.empty()) handle->response.result(boost::beast::http::status::not_found);
            else switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::get:
                body = &window_;
                break;
            case boost::beast::http::verb::post:
                jsexec(std::move(handle), engine, pool, *url);
                return;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router["/api/v1/app/"]:
        case router["/a/"]:
            if(tail.empty()) handle->response.result(boost::beast::http::status::not_found);
            else
            {
                boost::asio::post(*get<Server>().disk, [this, &engine, &pool, tail=std::move(tail), handle=std::move(handle)]{
                    auto part = parse_kv(tail);
                    auto name = "app/" + part[0] + ".notojs";
                    if(auto version = get<Folder>().load<detail::Version>(name); !version)
                    {
                        handle->response.result(boost::beast::http::status::not_found);
                        handle->finish();
                    }
                    else
                    {
                        handle->response.result(boost::beast::http::status::method_not_allowed);
                        jsexec(std::move(handle), engine, pool, std::move(*version), std::move(name), "/" + part[1]);
                    }
                });
                return;
            }
            break;
        case router["/api/v1/folder/"]:
        case router["/f/"]:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::propfind:
                if(tail.size())
                {
                    handle->response.result(boost::beast::http::status::method_not_allowed);
                }
                else
                {
                    boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle)]{
                        handle->response.result(get<Folder>().list(handle->response.body()));
                        handle->response.set(boost::beast::http::field::content_type, "application/json");
                        handle->finish();
                    });
                    return;
                }
                break;
            case boost::beast::http::verb::get:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle), tail=std::move(tail)]{
                    handle->response.result(get<Folder>().load(handle->response.body(), tail));
                    handle->response.set(boost::beast::http::field::content_type, "application/json");
                    handle->finish();
                });
                return;
            case boost::beast::http::verb::move:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle), tail=std::move(tail)]{
                    handle->response.result(get<Folder>().move(tail, handle->parser.get().body()));
                    handle->finish();
                });
                return;
            case boost::beast::http::verb::put:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle), tail=std::move(tail)]{
                    handle->response.result(get<Folder>().save(handle->parser.get().body(), tail));
                    handle->response.set(boost::beast::http::field::content_type, "application/json");
                    handle->finish();
                });
                return;
            case boost::beast::http::verb::delete_:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle), tail=std::move(tail)]{
                    handle->response.result(get<Folder>().remove(tail));
                    handle->finish();
                });
                return;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router["/api/v1/kernel/"]:
        case router["/k/"]:
            if(get<Window>().kernel(handle, tail)) return;
            break;
        case router["/api/v1/keymap"]:
        case router["/m"]:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::get:
                if(auto it = handle->parser.get().find(boost::beast::http::field::if_none_match); it == handle->parser.get().end() || ETAG != it->value()) {
                    body = &KEYMAP_INI;
                } else {
                    handle->response.result(boost::beast::http::status::not_modified);
                }
                handle->response.set(boost::beast::http::field::content_type, "text/plain");
                handle->response.set(boost::beast::http::field::cache_control, "public, max-age=0, must-revalidate");
                handle->response.set(boost::beast::http::field::etag, ETAG);
                break;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router["/api/v1/packages"]:
        case router["/p"]:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::get:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle)]{
                    handle->response.result(get<Folder>().get_packages(handle->response.body()));
                    handle->response.set(boost::beast::http::field::content_type, "text/plain");
                    handle->finish();
                });
                return;
            case boost::beast::http::verb::put:
                boost::asio::post(*get<Server>().disk, [this, handle=std::move(handle)]{
                    std::string &source = handle->parser.get().body();
                    handle->response.result(get<Folder>().set_packages(source));
                    handle->response.set(boost::beast::http::field::content_type, "text/plain");
                    handle->response.body() = std::move(source);
                    handle->finish();
                });
                return;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router["/notojs.Render/"]:
            if(tail.empty()) handle->response.result(boost::beast::http::status::not_found);
            else
            {
                boost::asio::post(*get<Server>().disk, [this, &engine, tail=std::move(tail), handle=std::move(handle)]{
                    engine.render(handle->parser.get(), tail, handle->response);
                    handle->finish();
                });
                return;
            }
            break;
        case router["/api/v1/result/"]:
        case router["/r/"]:
            {
                auto it = handle->parser.get().find(boost::beast::http::field::accept);
                Flag flag{
                    .html = (it != handle->parser.get().end() && "text/html" == it->value()),
                    .json = (it == handle->parser.get().end() || "application/json" == it->value() || "*/*" == it->value())
                };
                switch(handle->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    if(!flag) handle->response.result(boost::beast::http::status::not_acceptable);
                    else
                    {
                        boost::asio::post(*get<Server>().disk, [this, flag, tail=std::move(tail), &engine, handle=std::move(handle)]{
                            handle->response.set(boost::beast::http::field::content_type, flag.html ? "text/html" : "application/json");
                            if(flag.html)
                            {
                                std::unordered_set<std::string> renderers;
                                handle->response.result(get<Folder>().eval(handle->response.body(), tail, renderers));
                                wrap(handle->response.body(), engine, renderers);
                            }
                            else
                            {
                                handle->response.result(get<Folder>().eval(handle->response.body(), tail));
                            }
                            handle->finish();
                        });
                        return;
                    }
                    break;
                case boost::beast::http::verb::put:
                    flag.name = tail;
                case boost::beast::http::verb::post:
                    if(!flag) handle->response.result(boost::beast::http::status::not_acceptable);
                    else
                    {
                        boost::asio::post(*get<Server>().disk, [this, flag, &pool, tail=std::move(tail), &engine, handle=std::move(handle)]{
                            if(auto code = get<Folder>().load<detail::Textcode>(tail); code)
                            {
                                jsexec(handle, engine, pool, std::move(*code), flag);
                            }
                            else
                            {
                                handle->response.result(boost::beast::http::status::not_found);
                                handle->finish();
                            }
                        });
                        return;
                    }
                    break;
                default:
                    handle->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
            }
            break;
        case router["/api/v1/storage/"]:
        case router["/s/"]:
            if(storage(std::move(handle), tail, *get<Server>().disk, *url)) return;
            break;
        case router["/api/v1/log/rotate"]:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::put:
                if(!rotate_) handle->response.result(boost::beast::http::status::method_not_allowed);
                else
                {
                    handle->response.set(boost::beast::http::field::content_type, "text/plain; charset=\"utf-8\"");
                    handle->response.body() = rotate_();
                }
                break;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router["/api/v1/window"]:
        case router["/w"]:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::get:
                get<Window>().create(handle);
                return;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
            break;
        case router.routes.size():
            handle->response.result(boost::beast::http::status::not_found);
            break;
        default:
            switch(handle->parser.get().method())
            {
            case boost::beast::http::verb::get:
                if(auto it = handle->parser.get().find(boost::beast::http::field::if_none_match); it == handle->parser.get().end() || ETAG != it->value()) {
                    body = std::get<0>(static_[path]);
                } else {
                    handle->response.result(boost::beast::http::status::not_modified);
                }
                handle->response.set(boost::beast::http::field::content_type, std::get<1>(static_[path]));
                handle->response.set(boost::beast::http::field::cache_control, "public, max-age=0, must-revalidate");
                handle->response.set(boost::beast::http::field::etag, ETAG);
                break;
            default:
                handle->response.result(boost::beast::http::status::method_not_allowed);
                break;
            }
        }
    }
    handle->response.prepare_payload();
    handle->finish(body);
}

} // namespace notojs
