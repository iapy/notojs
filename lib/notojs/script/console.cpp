#include <notojs/script/console.hpp>
#include <notojs/global.hpp>
#include <global.hpp>

namespace notojs {
namespace {

JSValue clog(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    return facade::clog(ctx, argc, argv);
}

JSValue log(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    return facade::print(ctx, argc, argv);
}

JSValue init(JSContext *ctx, bool cleanup)
{
    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue impl = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, impl, "log", JS_NewCFunction(ctx, &log, "log", 1));
    JS_SetPropertyStr(ctx, impl, "clog", JS_NewCFunction(ctx, &clog, "clog", 1));
    JS_SetPropertyStr(ctx, glob, "console", impl);
    JS_FreeValue(ctx, glob);
    if(cleanup)
        Global::Context::ptr(ctx)->cleanup.insert("console");
    return JS_UNDEFINED;
}

} // namespace

JSValue notojs_init_console(JSContext *ctx)
{
    return init(ctx, false);
}

JSValue notojs_init_console(JSContext *ctx, ScriptConfig cfg)
{
    auto scope = cfg.get<ScriptConfig::Scope>("scope");
    return init(ctx, (scope && "cell" == static_cast<std::string_view const &>(*scope)));
}

} // namespace notojs
