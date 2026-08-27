struct SVGFEDistantLightElement : bridge::Interface<SVGFEDistantLightElement, dom::SVGElement, SVGElement>
{
    SVGFEDistantLightElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEDistantLightElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEDistantLightElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEDistantLightElement::funcs[] = {
    REFLECTING_ATTRIBUTE(azimuth),
    REFLECTING_ATTRIBUTE(elevation),
};
