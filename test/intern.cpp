#include <bridge.hpp>
namespace {

JSValue request_0(JSContext *ctx)
{
    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue fetch = JS_GetPropertyStr(ctx, glob, "fetch");

    bridge::Object config{ctx};
    config.set("body", bridge::String{ctx, std::string{"{\"foo\": 42}"}});
    config.set("method", bridge::String{ctx, std::string{"POST"}});

    JSValue args[2] = {bridge::String{ctx, std::string{"/json"}}, config};
    bridge::Strong<bridge::Promise> ret{ctx, JS_Call(ctx, fetch, glob, 2, &args[0])};

    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, fetch);
    JS_FreeValue(ctx, glob);

    return std::move(ret).wrap(
        [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            return JS_DupValue(ctx, argv[0]);
        },
        [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
            return JS_DupValue(ctx, argv[0]);
        }).release();
}

JSValue request_1(JSContext *ctx, bridge::String type, bridge::Object data)
{
    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue fetch = JS_GetPropertyStr(ctx, glob, "fetch");

    bridge::Object payload{ctx};
    payload.set("type", JS_DupValue(ctx, type));
    payload.set("data", JS_DupValue(ctx, data));

    bridge::Object config{ctx};
    config.set("body", payload);
    config.set("method", bridge::String{ctx, std::string{"POST"}});

    JSValue args[2] = {bridge::String{ctx, std::string{"/json"}}, config};
    bridge::Promise ret{ctx, JS_Call(ctx, fetch, glob, 2, &args[0])};

    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, fetch);
    JS_FreeValue(ctx, glob);

    return ret;
}

using request = bridge::Function<&request_0, request_1>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("request", 0, &request::invoke)
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
