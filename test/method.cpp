#include <bridge.hpp>
#include <map>

namespace {

struct Headers : bridge::Interface<Headers, std::map<std::string, std::string>>
{
    Headers() = default;

    JSValue get(JSContext *ctx, bridge::String key)
    {
        if(auto it = ref().find(static_cast<std::string>(key)); it != std::end(ref()))
            return bridge::String(ctx, it->second);
        return JS_UNDEFINED;
    }

    JSValue has(JSContext *ctx, bridge::String key)
    {
        return ref().find(static_cast<std::string>(key)) == std::end(ref()) ? JS_FALSE : JS_TRUE;
    }

    JSValue set(JSContext *ctx, bridge::String key, bridge::String value)
    {
        ref()[static_cast<std::string>(key)] = static_cast<std::string>(value);
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Headers::funcs[] = {
    JS_CFUNC_DEF("get", 1, &bridge::Function<&Headers::get>::invoke),
    JS_CFUNC_DEF("has", 1, &bridge::Function<&Headers::has>::invoke),
    JS_CFUNC_DEF("set", 2, &bridge::Function<&Headers::set>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Headers::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Headers::name());
    return mod;
}

} // extern "C"
