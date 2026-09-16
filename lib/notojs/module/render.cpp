#include <notojs/module/render.hpp>
#include <notojs/notojs.hpp>
#include <notojs/global.hpp>
#include <bridge.hpp>

namespace notojs {
namespace {

JSValue render_call(JSContext *ctx, JSValueConst self, bridge::String name, bridge::Lambda fn, bridge::Tail<0, bridge::Value> args)
{
    JSValue result = JS_Call(ctx, fn, self, static_cast<int>(args.size()), args.data());
    if(JS_IsException(result)) return result;

    Global::Context::ptr(ctx)->renderers.insert(name);
    return Content::make(ctx, name, result);
}

JSValue render_view(JSContext *ctx, bridge::Lambda fn, bridge::String view, bridge::Tail<0, bridge::Value> args)
{
    bridge::Strong<void> result{ctx, JS_Call(ctx, fn, JS_UNDEFINED, static_cast<int>(args.size()), args.data()), false};
    if(JS_IsException(result)) return result.release();

    if(JS_DefinePropertyValueStr(ctx, result, "view", JS_DupValue(ctx, view), JS_PROP_C_W_E) < 0)
        return JS_EXCEPTION;
    return result.release();
}

JSValue render_get(JSContext *ctx, bridge::Object regex, bridge::Lambda fn, bridge::Value property, bridge::Value)
{
    if(!JS_IsString(property))
        return JS_ThrowTypeError(ctx, "render: expected a string");

    bridge::Strong<void> match{ctx, JS_GetPropertyStr(ctx, property, "match"), false};
    bridge::Strong<void> result{ctx, JS_Call(ctx, match, property, 1, +regex), false};
    if(JS_IsException(result)) return result.release();

    if(!JS_ToBool(ctx, result))
    {
        char const *view = JS_ToCString(ctx, property);
        if(!view) return JS_EXCEPTION;

        JS_ThrowTypeError(ctx, "render: invalid view %s", view);
        JS_FreeCString(ctx, view);
        return JS_EXCEPTION;
    }

    return bridge::FunctionData<&render_view>::bind(ctx, fn, property);
}

JSValue render_0(JSContext *ctx, bridge::String name, bridge::Lambda fn)
{
    return bridge::FunctionData<&render_call>::bind(ctx, name, fn);
}

JSValue render_1(JSContext *ctx, bridge::String name, bridge::Object regex, bridge::Lambda fn)
{
    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};
    auto regexp = glob["RegExp"];

    if(JS_IsException(regexp)) return regexp.release();
    int valid = JS_IsInstanceOf(ctx, regex, regexp);

    if(valid < 0) return JS_EXCEPTION;
    if(!valid) return JS_ThrowTypeError(ctx, "render: expected a RegExp");

    bridge::Strong<void> h{ctx, JS_NewObject(ctx), false};
    bridge::Strong<void> r{ctx, render_0(ctx, name, fn), false};
    if(JS_IsException(r)) return r.release();

    JSValue get = bridge::FunctionData<&render_get>::bind(ctx, regex);

    if(JS_IsException(get)) return get;
    if(JS_SetPropertyStr(ctx, h, "get", get) < 0) return JS_EXCEPTION;

    JSValue args[] = {r, h};
    return JS_CallConstructor(ctx, glob["Proxy"], 2, args);
}

using render = bridge::Function<&render_0, &render_1>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("render", 2, &render::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExport(ctx, m, "default", JS_NewCFunction(ctx, render::invoke, "render", 2));
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_render() {}
void notojs_init_render(JSRuntime *) {}
void notojs_init_render(detail::Config const &) {}

JSModuleDef *notojs_init_render(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "default");
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // namespace notojs
