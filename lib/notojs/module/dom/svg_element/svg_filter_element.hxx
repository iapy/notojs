struct SVGFilterElement : bridge::Interface<SVGFilterElement, dom::SVGElement, SVGElement>
{
    SVGFilterElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFilterElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFilterElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFilterElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
    REFLECTING_ATTRIBUTE(filterUnits),
    REFLECTING_ATTRIBUTE(primitiveUnits),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
