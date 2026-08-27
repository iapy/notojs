struct SVGFETurbulenceElement : bridge::Interface<SVGFETurbulenceElement, dom::SVGElement, SVGElement>
{
    SVGFETurbulenceElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFETurbulenceElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFETurbulenceElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFETurbulenceElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES,
    REFLECTING_ATTRIBUTE(baseFrequency),
    REFLECTING_ATTRIBUTE(numOctaves),
    REFLECTING_ATTRIBUTE(seed),
    REFLECTING_ATTRIBUTE(stitchTiles),
    REFLECTING_ATTRIBUTE(type),
};
