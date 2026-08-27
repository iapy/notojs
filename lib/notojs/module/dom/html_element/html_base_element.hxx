struct HTMLBaseElement : bridge::Interface<HTMLBaseElement, dom::HTMLElement, HTMLElement>
{
    HTMLBaseElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLBaseElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLBaseElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLBaseElement::funcs[] = {
    REFLECTING_ATTRIBUTE(href),
    REFLECTING_ATTRIBUTE(target)
};
