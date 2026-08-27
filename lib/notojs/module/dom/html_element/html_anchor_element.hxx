struct HTMLAnchorElement : bridge::Interface<HTMLAnchorElement, dom::HTMLElement, HTMLElement>
{
    HTMLAnchorElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLAnchorElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLAnchorElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLAnchorElement::funcs[] = {
    REFLECTING_ATTRIBUTE(href),
    REFLECTING_ATTRIBUTE(hreflang),
    REFLECTING_ATTRIBUTE(rel),
    REFLECTING_ATTRIBUTE(target),

    JS_CGETSET_DEF("relList", &bridge::Getter<&HTMLElement::relList>, NULL)
};
