struct CSSRule : bridge::Interface<CSSRule, std::pair<std::string, std::uint16_t>>
{
    CSSRule(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSRule(std::reference_wrapper<std::pair<std::string, std::uint16_t>> &&rw) : Base(std::move(rw)) {}

    JSValue cssText(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().first};
    }

    JSValue type(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref().second};
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<CSSRule>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const CSSRule::funcs[] = {
    JS_CGETSET_DEF("cssText", &bridge::Getter<&CSSRule::cssText>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&CSSRule::type>, NULL)
};
