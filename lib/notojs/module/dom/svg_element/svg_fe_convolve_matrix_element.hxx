struct SVGFEConvolveMatrixElement : bridge::Interface<SVGFEConvolveMatrixElement, dom::SVGElement, SVGElement>
{
    SVGFEConvolveMatrixElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEConvolveMatrixElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEConvolveMatrixElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEConvolveMatrixElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(order),
    REFLECTING_ATTRIBUTE(kernelMatrix),
    REFLECTING_ATTRIBUTE(divisor),
    REFLECTING_ATTRIBUTE(bias),
    REFLECTING_ATTRIBUTE(targetX),
    REFLECTING_ATTRIBUTE(targetY),
    REFLECTING_ATTRIBUTE(edgeMode),
    REFLECTING_ATTRIBUTE(kernelUnitLength),
    REFLECTING_ATTRIBUTE(preserveAlpha),
};
