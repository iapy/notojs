struct HTMLDataElement : bridge::Interface<HTMLDataElement, dom::HTMLElement, HTMLElement>
{
    HTMLDataElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLDataElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLDataElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLDataElement::funcs[] = {
    REFLECTING_ATTRIBUTE(value)
};
