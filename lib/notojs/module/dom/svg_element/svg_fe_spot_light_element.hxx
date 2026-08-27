struct SVGFESpotLightElement : bridge::Interface<SVGFESpotLightElement, dom::SVGElement, SVGElement>
{
    SVGFESpotLightElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFESpotLightElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFESpotLightElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFESpotLightElement::funcs[] = {
    REFLECTING_ATTRIBUTE(x),
    REFLECTING_ATTRIBUTE(y),
    REFLECTING_ATTRIBUTE(z),
    REFLECTING_ATTRIBUTE(pointsAtX),
    REFLECTING_ATTRIBUTE(pointsAtY),
    REFLECTING_ATTRIBUTE(pointsAtZ),
    REFLECTING_ATTRIBUTE(specularExponent),
    REFLECTING_ATTRIBUTE(limitingConeAngle),
};
