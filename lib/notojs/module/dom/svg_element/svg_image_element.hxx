struct SVGImageElement : bridge::Interface<SVGImageElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGImageElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGImageElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGImageElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGImageElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
