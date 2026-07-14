#include "quickjs/quickjs.h"
#include <notojs/module/core.hpp>
#include <notojs/global.hpp>
#include <notojs/notojs.hpp>
#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>

#include <lmdbxx/lmdb++.h>
#include <notodb.hpp>
#include <stdexcept>

namespace notojs {
namespace {

constexpr std::string_view SVG_MIME{"image/svg+xml"};

JSValue image_1(JSContext *ctx, bridge::String url)
{
    auto const &u = static_cast<std::string_view const &>(url);
    if(auto p = boost::urls::parse_uri(u); p && (
            boost::urls::scheme::http == p->scheme_id()
        || boost::urls::scheme::https == p->scheme_id()
    ) || "data:" == u.substr(0, 5))
        return Image::data(ctx, JS_DupValue(ctx, url));
    return JS_ThrowTypeError(ctx, "Unsupported URL: %s", u.data());
}

JSValue image_2(JSContext *ctx, IURL::Interface::Impl u)
{
    return Image::data(ctx, bridge::String{ctx, u->href()});
}

JSValue image_3(JSContext *ctx, SVG::Interface::Impl s)
{
    return SVG::data(ctx, bridge::String{ctx, s->get()});
}

JSValue image_4(JSContext *ctx, Image::Interface::Impl i)
{
    return Image::data(ctx, bridge::String{ctx, i->get()});
}

JSValue image_5(JSContext *ctx, IBlob::Interface::Impl blob)
{
    auto type = blob->type();
    auto const [data, size] = blob->data();
    if(SVG_MIME == type)
        return SVG::data(ctx, bridge::String{ctx, std::string_view{reinterpret_cast<char const *>(data), size}});

    std::string result;
    result.append("data:");
    if(!type.empty())
        result.append(std::move(type));
    else
        result.append("application/octet-stream");
    result.append(";base64,");

    std::string base64;
    base64.resize(boost::beast::detail::base64::encoded_size(size));
    base64.resize(boost::beast::detail::base64::encode(&base64[0], data, size));
    result.append(std::move(base64));

    return Image::data(ctx, bridge::String{ctx, std::move(result)});
}

using image = bridge::Function<image_1, image_2, image_3, image_4, image_5>;

JSValue icon(JSContext *ctx, bridge::String name)
{
    std::string const curl{"https://api.iconify.design/" + static_cast<std::string>(name) + ".svg"};

    try {
        std::string data;
        auto [tx, db] = DB(ctx).http(DB::RO);
        lmdb::val k{curl.data(), curl.size()};
        if(lmdb::val v; db.get(tx, k, v))
            data.assign(v.data(), v.size());
        tx.abort();
        if(!data.empty())
            return SVG::data(ctx, bridge::String{ctx, std::move(data)});
    } catch(...) {}

    auto u = facade::URL::parse(curl.data());
    if(!u) return JS_ThrowTypeError(ctx, "icon: malformed url %s", curl.c_str());

    bridge::Strong<bridge::String> url{ctx, bridge::String{ctx, curl}};
    boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::get, u->path(), 11};

    return bridge::Strong<bridge::Promise>{ctx, facade::fetch(ctx, std::move(request), std::move(*u),
        +[](JSContext *ctx, JSValue resp, boost::beast::http::response<boost::beast::http::string_body> const &response) -> JSValue {
            if(auto const it = response.find(boost::beast::http::field::content_type);
                boost::beast::http::status::ok == response.result()
                && it != std::end(response)
                && it->value().size() >= SVG_MIME.size()
                && !it->value().compare(0, SVG_MIME.size(), SVG_MIME))
            {
                return bridge::String{ctx, response.body()};
            }
            return JS_NULL;
        })
    }.wrap(
        [](JSContext *ctx, JSValueConst, int argc, JSValue *argv, int size, JSValue *data) -> JSValue
        {
            if(!bridge::String::check(ctx, argv)) return JS_DupValue(ctx, *argv);

            bridge::String body{ctx, *argv};
            std::string const &svg = static_cast<std::string const &>(body);

            bridge::String datb{ctx, *data};
            std::string const &url = static_cast<std::string const &>(datb);

            try {
                auto [tx, db] = DB(ctx).http(DB::RW);
                lmdb::val k{url.data(), url.size()};
                lmdb::val v{svg.data(), svg.size()};
                db.put(tx, k, v);
                tx.commit();
            } catch(...) {}

            return SVG::data(ctx, JS_DupValue(ctx, *argv));
        },
        [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) -> JSValue
        {
            return JS_DupValue(ctx, *argv);
        }, 1, +url
    ).release();
}

JSValue markdown(JSContext *ctx, bridge::String str)
{
    return __Markdown::data(ctx, JS_DupValue(ctx, str));
}

JSValue html_0(JSContext *ctx, bridge::String str)
{
    return HTML::data(ctx, JS_DupValue(ctx, str));
}

JSValue html_1(JSContext *ctx, HTML::Interface::Impl h)
{
    JSValue v = HTML::data(ctx, bridge::String(ctx, h->get()));
    if(!h->json()) JS_DefinePropertyValueStr(ctx, v, ".json", JS_FALSE, JS_PROP_CONFIGURABLE);
    return v;
}

using html = bridge::Function<html_0, html_1>;

JSValue xml_0(JSContext *ctx, bridge::String str)
{
    return XML::data(ctx, JS_DupValue(ctx, str));
}

JSValue xml_1(JSContext *ctx, XML::Interface::Impl h)
{
    try {
        std::string data = h->get();
        return XML::data(ctx, bridge::String(ctx, std::move(data)));
    } catch(std::runtime_error const &e) {
        return JS_ThrowTypeError(ctx, "Invalid XML: %s", e.what());
    }
}

using xml = bridge::Function<xml_0, xml_1>;

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("html", 1, &html::invoke),
    JS_CFUNC_DEF("icon", 1, &bridge::Function<icon>::invoke),
    JS_CFUNC_DEF("image", 1, &image::invoke),
    JS_CFUNC_DEF("markdown", 1, &bridge::Function<markdown>::invoke),
    JS_CFUNC_DEF("xml", 1, &xml::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    HTML::alias(ctx, m);
    Image::alias(ctx, m);
    __Markdown::alias(ctx, m, "Markdown");
    SVG::alias(ctx, m);
    XML::alias(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_core() {}
void notojs_init_core(JSRuntime *rt) {}
void notojs_init_core(boost::property_tree::ptree const &) {}

JSModuleDef *notojs_init_core(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    JS_AddModuleExport(ctx, mod, HTML::name());
    JS_AddModuleExport(ctx, mod, Image::name());
    JS_AddModuleExport(ctx, mod, "Markdown");
    JS_AddModuleExport(ctx, mod, SVG::name());
    JS_AddModuleExport(ctx, mod, XML::name());
    return mod;
}

JSValue core::facade::html(JSContext *ctx, std::string const &data)
{
    return HTML::data(ctx, bridge::String{ctx, data});
}

JSValue core::facade::image(JSContext *ctx, boost::urls::url const &url)
{
    std::string_view view = url.buffer();
    return Image::data(ctx, bridge::String{ctx, std::move(view)});
}

JSValue core::facade::image(JSContext *ctx, std::uint8_t const *data, std::size_t size, std::string const &mime)
{
    if(SVG_MIME == mime)
        return SVG::data(ctx, bridge::String{ctx, std::string_view{reinterpret_cast<char const *>(data), size}});

    std::string result;
    result.append("data:");
    if(!mime.empty())
        result.append(mime);
    else
        result.append("application/octet-stream");
    result.append(";base64,");

    std::string base64;
    base64.resize(boost::beast::detail::base64::encoded_size(size));
    base64.resize(boost::beast::detail::base64::encode(&base64[0], data, size));
    result.append(std::move(base64));

    return Image::data(ctx, bridge::String{ctx, std::move(result)});
}

JSValue core::facade::markdown(JSContext *ctx, std::string const &data)
{
    return __Markdown::data(ctx, bridge::String{ctx, data});
}

JSValue core::facade::svg(JSContext *ctx, std::string const &data)
{
    return SVG::data(ctx, bridge::String{ctx, data});
}

JSValue core::facade::xml(JSContext *ctx, std::string const &data)
{
    return XML::data(ctx, bridge::String{ctx, data});
}

} // namespace notojs
