#include <notojs/script/storage.hpp>
#include <notojs/global.hpp>
#include <bridge.hpp>

namespace notojs {
namespace {

JSValue init(JSContext *ctx, bool cleanup, bridge::String ns)
{
    JSValue glob = JS_GetGlobalObject(ctx);
    bridge::Strong<bridge::Lambda> ctor{ctx, bridge::Lambda{ctx, JS_GetPropertyStr(ctx, glob, "Storage")}};

    JSValue value = JS_CallConstructor(ctx, ctor, 1, +ns);
    if(!bridge::Error::check(ctx, &value))
    {
        JS_SetPropertyStr(ctx, glob, "localStorage", value);
        value = JS_UNDEFINED;
        if(cleanup)
            Global::Context::ptr(ctx)->cleanup.insert("localStorage");
    }
    JS_FreeValue(ctx, glob);
    return value;
}

} // namespace

JSValue notojs_init_storage(JSContext *ctx, ScriptConfig cfg)
{
    auto ns = cfg.get<bridge::String>("ns");
    if(!ns) return JS_ThrowTypeError(ctx, "Storage namespace is not specified");

    auto scope = cfg.get<ScriptConfig::Scope>("scope");
    return init(ctx, (scope && "cell" == static_cast<std::string_view const &>(*scope)), *ns);
}

} // namespace notojs
