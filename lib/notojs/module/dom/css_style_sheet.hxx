struct CSSStyleSheet : bridge::Interface<CSSStyleSheet, dom::CSSStyleSheet>
{
    CSSStyleSheet(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSStyleSheet(std::reference_wrapper<dom::CSSStyleSheet> &&rw) : Base(std::move(rw)) {}

    JSValue cssRules(JSContext *ctx, JSValue self)
    {
        return CSSRuleList::from(ctx, dom::CSSRuleList{ref()}, self);
    }

    JSValue deleteRule(JSContext *ctx, bridge::Number index)
    {
        ref().deleteRule(index);
        return JS_UNDEFINED;
    }

    JSValue insertRule_0(JSContext *ctx, bridge::String rule)
    {
        return bridge::Number{ctx, ref().insertRule(0, rule)};
    }

    JSValue insertRule_1(JSContext *ctx, bridge::String rule, bridge::Number index)
    {
        return bridge::Number{ctx, ref().insertRule(index, rule)};
    }

    using insertRule = bridge::Function
    <
        &CSSStyleSheet::insertRule_0,
        &CSSStyleSheet::insertRule_1
    >;

    JSValue ownerRule(JSContext *ctx) const
    {
        return JS_NULL;
    }

    JSValue replace(JSValue self, JSContext *ctx, bridge::String text)
    {
        (void)replaceSync(ctx, text);

        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(!JS_IsException(promise))
        {
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &self));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue replaceSync(JSContext *ctx, bridge::String text)
    {
        ref().doc->textContent(ref(), text);
        return JS_UNDEFINED;
    }

    BOOST_FORCEINLINE static void free(dom::CSSStyleSheet &self)
    {
        self.free();
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<CSSStyleSheet>;
    static JSCFunctionListEntry const funcs[];
    friend class CSSRuleList;
};

JSCFunctionListEntry const CSSStyleSheet::funcs[] = {
    JS_CGETSET_DEF("cssRules", &bridge::Getter<&CSSStyleSheet::cssRules>, NULL),
    JS_CGETSET_DEF("ownerRule", &bridge::Getter<&CSSStyleSheet::ownerRule>, NULL),

    JS_CFUNC_DEF("deleteRule", 1, &bridge::Function<&CSSStyleSheet::deleteRule>::invoke),
    JS_CFUNC_DEF("insertRule", 1, &CSSStyleSheet::insertRule::invoke),
    JS_CFUNC_DEF("replace", 1, &bridge::Function<&CSSStyleSheet::replace>::invoke),
    JS_CFUNC_DEF("replaceSync", 1, &bridge::Function<&CSSStyleSheet::replaceSync>::invoke),
};
