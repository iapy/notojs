#include <bridge.hpp>
#include <global.hpp>

namespace {

JSValue make(JSContext *ctx, bridge::Number n)
{
    std::vector<std::uint8_t> data;
    for(std::int64_t i = 0; i < static_cast<std::int64_t>(n); ++i)
        data.push_back(static_cast<std::uint8_t>(i));
    return noto::Blob::make(ctx, std::move(data));
}

JSValue load(JSContext *ctx, bridge::String s)
{
    auto const &sv = static_cast<std::string_view const &>(s);
    if(auto u = noto::URL::parse(sv.data()); u)
    {
        boost::beast::http::request<boost::beast::http::string_body> request{
            boost::beast::http::verb::get, u->path(), 11
        };
        return noto::fetch(ctx, std::move(request), std::move(*u),
            +[](JSContext *ctx, JSValue response, boost::beast::http::response<boost::beast::http::string_body> const &)
            {
                return JS_DupValue(ctx, response);
            });
    }
    return JS_NULL;
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("load", 1, &bridge::Function<&load>::invoke),
    JS_CFUNC_DEF("make", 1, &bridge::Function<&make>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
