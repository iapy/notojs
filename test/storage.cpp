#include <global.hpp>

namespace {

JSValue set_string_0(JSContext *ctx, bridge::String ns, bridge::String key, bridge::String value)
{
    noto::Storage::type storage{static_cast<std::string>(ns)};
    storage.set(ctx, key, value);
    return JS_UNDEFINED;
}

JSValue set_string_1(JSContext *ctx, noto::Storage st, bridge::String key, bridge::String value)
{
    st->get().set(ctx, key, value);
    return JS_UNDEFINED;
}

using set_string = bridge::Function<&set_string_0, &set_string_1>;

JSValue set_value_0(JSContext *ctx, bridge::String ns, bridge::String key, bridge::Value value)
{
    noto::Storage::type storage{static_cast<std::string>(ns)};
    storage.set(ctx, key, value);
    return JS_UNDEFINED;
}

JSValue set_value_1(JSContext *ctx, noto::Storage st, bridge::String key, bridge::Value value)
{
    st->get().set(ctx, key, value);
    return JS_UNDEFINED;
}

using set_value = bridge::Function<&set_value_0, &set_value_1>;

JSValue get_0(JSContext *ctx, bridge::String ns, bridge::String key)
{
    noto::Storage::type storage{static_cast<std::string>(ns)};
    return storage.get(ctx, key);
}

JSValue get_1(JSContext *ctx, noto::Storage st, bridge::String key)
{
    return st->get().get(ctx, key);
}

using get = bridge::Function<&get_0, &get_1>;

JSValue remove_0(JSContext *ctx, bridge::String ns, bridge::String key)
{
    noto::Storage::type storage{static_cast<std::string>(ns)};
    storage.remove(ctx, key);
    return JS_UNDEFINED;
}

JSValue remove_1(JSContext *ctx, noto::Storage st, bridge::String key)
{
    st->get().remove(ctx, key);
    return JS_UNDEFINED;
}

using remove = bridge::Function<&remove_0, &remove_1>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("setString", 3, &set_string::invoke),
    JS_CFUNC_DEF("setValue", 3, &set_value::invoke),
    JS_CFUNC_DEF("get", 2, &get::invoke),
    JS_CFUNC_DEF("remove", 2, &remove::invoke)
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
