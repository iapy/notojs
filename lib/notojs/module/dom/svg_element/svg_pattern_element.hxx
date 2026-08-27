struct SVGPatternElement : bridge::Interface<SVGPatternElement, dom::SVGElement, SVGElement>
{
    SVGPatternElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGPatternElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGPatternElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGPatternElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
    REFLECTING_ATTRIBUTE(patternUnits),
    REFLECTING_ATTRIBUTE(patternContentUnits),
    REFLECTING_ATTRIBUTE(preserveAspectRatio),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
    JS_CGETSET_DEF("viewBox", &bridge::Getter<&SVGElement::viewBox>, NULL),
};
