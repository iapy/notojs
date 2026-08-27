struct SVGAElement : bridge::Interface<SVGAElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGAElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGAElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGAElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGAElement::funcs[] = {
    REFLECTING_ATTRIBUTE(target),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
};
