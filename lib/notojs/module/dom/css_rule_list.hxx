struct CSSRuleList : bridge::Interface<CSSRuleList, dom::CSSRuleList>
{
    CSSRuleList(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSRuleList(std::reference_wrapper<dom::CSSRuleList> &&rw) : Base(std::move(rw)) {}

    JSValue get_property(JSContext *, char const *n) const;
    JSValue make_rule(JSContext *, dom::CSSRule &&, JSValue owner) const;
    JSValue item(JSContext *ctx, bridge::Number) const;
    JSValue length(JSContext *ctx) const;

    BOOST_FORCEINLINE static void free(dom::CSSRuleList &self)
    {
        self.free();
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<CSSRuleList>;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods CSSRuleList::exoticMethods = {
    .get_property = &bridge::get_property<CSSRuleList>
};

JSCFunctionListEntry const CSSRuleList::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&CSSRuleList::length>, NULL),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&CSSRuleList::item>::invoke)
};
