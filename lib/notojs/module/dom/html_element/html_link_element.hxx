struct HTMLLinkElement : bridge::Interface<HTMLLinkElement, dom::HTMLElement, HTMLElement>
{
    HTMLLinkElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLLinkElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLLinkElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLLinkElement::funcs[] = {
    REFLECTING_ATTRIBUTE(href),
    REFLECTING_ATTRIBUTE(hreflang),
    REFLECTING_ATTRIBUTE(rel),

    JS_CGETSET_DEF("relList", &bridge::Getter<&HTMLElement::relList>, NULL)
};
