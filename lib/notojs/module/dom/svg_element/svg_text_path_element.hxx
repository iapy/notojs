struct SVGTextPathElement : bridge::Interface<SVGTextPathElement, dom::SVGElement, SVGTextContentElement>
{
    SVGTextPathElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGTextPathElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGTextPathElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGTextPathElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(startOffset),
    REFLECTING_ATTRIBUTE(method),
    REFLECTING_ATTRIBUTE(spacing),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
