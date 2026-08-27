#include <notojs/script/dom.hpp>
#include <notojs/global.hpp>
#include <notojs/module.hpp>
#include <global.hpp>
#include <bridge.hpp>

namespace notojs {
namespace {

JSValue init(JSContext *ctx, bool cleanup)
{
    JSValue dom = Global::Context::ptr(ctx)->load(ctx, "noto:dom");
    if(JS_IsException(dom) || JS_IsError(ctx, dom)) return dom;

    JSValue document = JS_GetPropertyStr(ctx, dom, "Document");
    JSValue html = JS_GetPropertyStr(ctx, document, "html");
    JSValue glob = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, glob, "window", JS_GetPropertyStr(ctx, dom, "window"));
    JS_SetPropertyStr(ctx, glob, "document", JS_Call(ctx, html, document, 0, NULL));
    JS_FreeValue(ctx, glob);
    JS_FreeValue(ctx, html);
    JS_FreeValue(ctx, document);

    JS_FreeValue(ctx, dom);
    if(cleanup)
    {
        Global::Context::ptr(ctx)->cleanup.insert("window");
        Global::Context::ptr(ctx)->cleanup.insert("document");
    }
    return JS_UNDEFINED;
}

} // namespace

JSValue notojs_init_dom(JSContext *ctx)
{
    return init(ctx, false);
}

JSValue notojs_init_dom(JSContext *ctx, ScriptConfig cfg)
{
    auto scope = cfg.get<ScriptConfig::Scope>("scope");
    return init(ctx, (scope && "cell" == static_cast<std::string_view const &>(*scope)));
}

} // namespace notojs
