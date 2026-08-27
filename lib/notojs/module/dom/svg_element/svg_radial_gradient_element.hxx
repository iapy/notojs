struct SVGRadialGradientElement : bridge::Interface<SVGRadialGradientElement, dom::SVGElement, SVGElement>
{
    SVGRadialGradientElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGRadialGradientElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGRadialGradientElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGRadialGradientElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(cx),
    SVGLENGTH_ATTRIBUTE(cy),
    SVGLENGTH_ATTRIBUTE(r),
    SVGLENGTH_ATTRIBUTE(fx),
    SVGLENGTH_ATTRIBUTE(fy),
    SVGLENGTH_ATTRIBUTE(fr),
    REFLECTING_ATTRIBUTE(gradientUnits),
    REFLECTING_ATTRIBUTE(spreadMethod),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
