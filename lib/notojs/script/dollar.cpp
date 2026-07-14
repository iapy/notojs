#include <notojs/script/dollar.hpp>
#include <bridge.hpp>
#include <global.hpp>

namespace notojs {
namespace {

struct Config : bridge::Struct<Config>
{
    BRIDGE_DEFINE_STRUCT(Config);
    static constexpr auto fields = bridge::fields(
        bridge::field<bridge::Dict<bridge::String>>("headers"),
        bridge::field<bridge::String>("contentType"),
        bridge::field<bridge::String>("dataType"),
        bridge::field<bridge::String>("method"),
        bridge::field<bridge::String>("data"),
        bridge::field<bridge::String>("url")
    );
    using bridge::Object::operator JSValue;
    using bridge::Object::set;
};

JSValue ajax(JSContext *ctx, Config config)
{
    auto u = config.get<bridge::String>("url");
    if(!u) return JS_ThrowTypeError(ctx, "$.ajax: url should be specified");

    auto url = facade::URL::parse(static_cast<std::string_view>(*u).data());
    if(!url) return JS_ThrowTypeError(ctx, "$.ajax: malformed url %s", static_cast<std::string_view>(*u).data());

    boost::beast::http::request<boost::beast::http::string_body> request{
        std::invoke([&config]{
            if(auto me = config.get<bridge::String>("method"); me)
                return boost::beast::http::string_to_verb(static_cast<std::string_view>(*me));
            return boost::beast::http::verb::get;
        }),
        std::invoke([&url]{
            std::string path = url->path();
            if(auto q = url->encoded_query(); !q.empty())
            {
                path.append("?", 1);
                path.append(q.data(), q.size());
            }
            return path;
        }),
        11
    };

    bool json = false;
    if(auto dt = config.get<bridge::String>("dataType"); dt)
        json = "json" == static_cast<std::string_view>(*dt);
    if(auto ct = config.get<bridge::String>("contentType"); ct)
        request.set(boost::beast::http::field::content_type, static_cast<std::string_view>(*ct));
    if(auto data = config.get<bridge::String>("data"); data)
        request.body() = static_cast<std::string_view>(*data);
    if(auto headers = config.get<bridge::Dict<bridge::String>>("headers"); headers)
        headers->each([&request](auto &&key, auto value){
            request.set(key, static_cast<std::string_view const &>(value));
        });

    return facade::fetch(ctx, std::move(request), std::move(*url),
        json
        ? +[](JSContext *ctx, JSValue, boost::beast::http::response<boost::beast::http::string_body> const &response)
        {
            return JS_ParseJSON(ctx, response.body().c_str(), response.body().size(), "<json>");
        }
        : +[](JSContext *ctx, JSValue, boost::beast::http::response<boost::beast::http::string_body> const &response) -> JSValue
        {
            return bridge::String(ctx, response.body());
        }
    );
}

} // namespace

JSValue notojs_init_dollar(JSContext *ctx)
{
    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue impl = JS_GetPropertyStr(ctx, glob, "$");
    JS_SetPropertyStr(ctx, impl, "ajax", JS_NewCFunction(ctx, &bridge::Function<&ajax>::invoke, "ajax", 1));
    JS_FreeValue(ctx, impl);
    JS_FreeValue(ctx, glob);
    return JS_UNDEFINED;
}

} // namespace notojs
