struct SVGFESpecularLightingElement : bridge::Interface<SVGFESpecularLightingElement, dom::SVGElement, SVGElement>
{
    SVGFESpecularLightingElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFESpecularLightingElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFESpecularLightingElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFESpecularLightingElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(surfaceScale),
    REFLECTING_ATTRIBUTE(specularConstant),
    REFLECTING_ATTRIBUTE(specularExponent),
    REFLECTING_ATTRIBUTE(kernelUnitLength),
};
