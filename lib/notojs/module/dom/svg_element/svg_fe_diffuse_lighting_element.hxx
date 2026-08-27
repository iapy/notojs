struct SVGFEDiffuseLightingElement : bridge::Interface<SVGFEDiffuseLightingElement, dom::SVGElement, SVGElement>
{
    SVGFEDiffuseLightingElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEDiffuseLightingElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEDiffuseLightingElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEDiffuseLightingElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(surfaceScale),
    REFLECTING_ATTRIBUTE(diffuseConstant),
    REFLECTING_ATTRIBUTE(kernelUnitLength),
};
