#include <bridge.hpp>

namespace {
using namespace std::string_view_literals;

struct Custom : bridge::Interface<Custom, bridge::Object>
{
    using Base::Base;

    JSValue toJSON(JSContext *ctx) const
    {
        bridge::Object json{ctx};
        json.set("type", bridge::String(ctx, "Custom"sv));
        json.set("data", (*this)["data"].release());
        return json;
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Custom::funcs[] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&Custom::toJSON>::invoke)
};

JSValue consume_0(JSContext *ctx, Custom c)
{
    return c["data"].release();
}

JSValue factory_0(JSContext *ctx)
{
    return Custom::ctor(ctx);
}

JSValue factory_1(JSContext *ctx, bridge::String data)
{
    bridge::Strong<bridge::Object> obj{ctx, bridge::Object{ctx}};
    obj.set("data", JS_DupValue(ctx, data));
    return Custom::ctor(ctx, std::move(obj));
}

using consume = bridge::Function<&consume_0>;
using factory = bridge::Function<&factory_0, &factory_1>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("consume", 1, &consume::invoke),
    JS_CFUNC_DEF("factory", 0, &factory::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Custom::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Custom::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
