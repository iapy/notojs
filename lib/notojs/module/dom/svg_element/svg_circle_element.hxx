struct SVGCircleElement : bridge::Interface<SVGCircleElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGCircleElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGCircleElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGCircleElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGCircleElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(cx),
    SVGLENGTH_ATTRIBUTE(cy),
    SVGLENGTH_ATTRIBUTE(r),
};
