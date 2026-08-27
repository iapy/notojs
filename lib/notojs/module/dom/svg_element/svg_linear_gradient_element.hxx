struct SVGLinearGradientElement : bridge::Interface<SVGLinearGradientElement, dom::SVGElement, SVGElement>
{
    SVGLinearGradientElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGLinearGradientElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGLinearGradientElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGLinearGradientElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x1),
    SVGLENGTH_ATTRIBUTE(y1),
    SVGLENGTH_ATTRIBUTE(x2),
    SVGLENGTH_ATTRIBUTE(y2),
    REFLECTING_ATTRIBUTE(gradientUnits),
    REFLECTING_ATTRIBUTE(spreadMethod),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
