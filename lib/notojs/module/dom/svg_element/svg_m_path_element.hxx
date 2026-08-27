struct SVGMPathElement : bridge::Interface<SVGMPathElement, dom::SVGElement, SVGElement>
{
    SVGMPathElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGMPathElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGMPathElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGMPathElement::funcs[] = {
    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
