#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>

namespace {

JSValue make(JSContext *ctx, bridge::String n)
{
    return noto::fs::Path::make(ctx, std::filesystem::path(n));
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("make", 1, &bridge::Function<&make>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    notojs::fs::init(ctx);
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
