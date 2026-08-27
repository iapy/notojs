struct SVGFEDisplacementMapElement : bridge::Interface<SVGFEDisplacementMapElement, dom::SVGElement, SVGElement>
{
    SVGFEDisplacementMapElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEDisplacementMapElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEDisplacementMapElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEDisplacementMapElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(in2),
    REFLECTING_ATTRIBUTE(scale),
    REFLECTING_ATTRIBUTE(xChannelSelector),
    REFLECTING_ATTRIBUTE(yChannelSelector),
};
