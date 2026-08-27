struct SVGTSpanElement : bridge::Interface<SVGTSpanElement, dom::SVGElement, SVGTextContentElement>
{
    SVGTSpanElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGTSpanElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGTSpanElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGTSpanElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(dx),
    SVGLENGTH_ATTRIBUTE(dy),
    SVGLENGTH_ATTRIBUTE(rotate),
};
