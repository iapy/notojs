struct SVGUseElement : bridge::Interface<SVGUseElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGUseElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGUseElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGUseElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGUseElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
