#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>

namespace {

JSValue html(JSContext *ctx, bridge::String n)
{
    return noto::core::html(ctx, n);
}

JSValue image_0(JSContext *ctx)
{
    std::vector<std::uint8_t> data{};
    return noto::core::image(ctx, data.data(), data.size());
}

JSValue image_1(JSContext *ctx, bridge::String url)
{
    auto const s = static_cast<std::string_view const &>(url);
    auto const u = noto::URL::parse(s.data());

    if(u) return noto::core::image(ctx, std::move(*u));
    return JS_NULL;
}

using image = bridge::Function<&image_0, &image_1>;

JSValue markdown(JSContext *ctx, bridge::String n)
{
    return noto::core::markdown(ctx, n);
}

JSValue svg_0(JSContext *ctx)
{
    constexpr std::string_view data{"<svg></svg>"};
    return noto::core::image(ctx, reinterpret_cast<std::uint8_t const *>(data.data()), data.size(), "image/svg+xml");
}

JSValue svg_1(JSContext *ctx, bridge::String n)
{
    return noto::core::svg(ctx, n);
}

using svg = bridge::Function<&svg_0, &svg_1>;

JSValue xml(JSContext *ctx, bridge::String n)
{
    return noto::core::xml(ctx, n);
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("html", 1, &bridge::Function<&html>::invoke),
    JS_CFUNC_DEF("image", 1, &image::invoke),
    JS_CFUNC_DEF("markdown", 1, &bridge::Function<&markdown>::invoke),
    JS_CFUNC_DEF("svg", 1, &svg::invoke),
    JS_CFUNC_DEF("xml", 1, &bridge::Function<&xml>::invoke)
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
