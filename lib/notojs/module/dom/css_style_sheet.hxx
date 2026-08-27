struct CSSStyleSheet : bridge::Interface<CSSStyleSheet, dom::CSSStyleSheet>
{
    CSSStyleSheet(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSStyleSheet(std::reference_wrapper<dom::CSSStyleSheet> &&rw) : Base(std::move(rw)) {}

    JSValue cssRules(JSContext *ctx, JSValue self)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->cssrules.find(ptr); it != std::end(doc->cssrules))
            return JS_DupValue(ctx, it->second);
        return doc->cssrules[ptr] = CSSRuleList::from(ctx, dom::CSSRuleList{ref()}, self);
    }

    static JSValue mutationError(JSContext *ctx, dom::CSSStyleSheet::MutationError error)
    {
        switch(error)
        {
        case dom::CSSStyleSheet::MutationError::index_size:
            return DOMException::throwIndexSizeError(ctx);
        case dom::CSSStyleSheet::MutationError::syntax:
            return DOMException::throwSyntaxError(ctx);
        default:
            return JS_UNDEFINED;
        }
    }

    JSValue deleteRule(JSContext *ctx, bridge::Number index)
    {
        return mutationError(ctx, ref().deleteRule(css_rule_index(index)));
    }

    JSValue insertRule_0(JSContext *ctx, bridge::String rule)
    {
        auto const [error, index] = ref().insertRule(0, rule);
        if(dom::CSSStyleSheet::MutationError::none != error)
            return mutationError(ctx, error);
        return bridge::Number{ctx, index};
    }

    JSValue insertRule_1(JSContext *ctx, bridge::String rule, bridge::Number index)
    {
        auto const [error, inserted] = ref().insertRule(css_rule_index(index), rule);
        if(dom::CSSStyleSheet::MutationError::none != error)
            return mutationError(ctx, error);
        return bridge::Number{ctx, inserted};
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

        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);
        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        JSValue settled = JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &self);
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        if(JS_IsException(settled))
        {
            JS_FreeValue(ctx, promise);
            return settled;
        }
        JS_FreeValue(ctx, settled);
        return promise;
    }

    JSValue replaceSync(JSContext *ctx, bridge::String text)
    {
        ref().replace(text);
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
