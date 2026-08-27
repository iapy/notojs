#include <bridge.hpp>

namespace {

struct Enum : bridge::Interface<Enum, int>
{
    using Base::Base;

    static void sprop(JSContext *ctx, JSValue ctor)
    {
        JS_DefinePropertyValueStr(ctx, ctor, "ONE", bridge::Number{ctx, 1}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "TWO", bridge::Number{ctx, 2}, JS_PROP_ENUMERABLE);
    }
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Enum::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Enum::name());
    return mod;
}

} // extern "C"
