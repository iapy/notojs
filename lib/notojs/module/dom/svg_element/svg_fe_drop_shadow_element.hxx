struct SVGFEDropShadowElement : bridge::Interface<SVGFEDropShadowElement, dom::SVGElement, SVGElement>
{
    SVGFEDropShadowElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEDropShadowElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEDropShadowElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEDropShadowElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(dx),
    SVGLENGTH_ATTRIBUTE(dy),
    SVG_FILTER_PRIMITIVE_ATTRIBUTES,
    REFLECTING_ATTRIBUTE(stdDeviation),
};
