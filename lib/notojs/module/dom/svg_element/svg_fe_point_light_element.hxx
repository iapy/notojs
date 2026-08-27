struct SVGFEPointLightElement : bridge::Interface<SVGFEPointLightElement, dom::SVGElement, SVGElement>
{
    SVGFEPointLightElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEPointLightElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEPointLightElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEPointLightElement::funcs[] = {
    REFLECTING_ATTRIBUTE(x),
    REFLECTING_ATTRIBUTE(y),
    REFLECTING_ATTRIBUTE(z),
};
