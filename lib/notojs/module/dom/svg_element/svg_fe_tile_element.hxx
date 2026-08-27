struct SVGFETileElement : bridge::Interface<SVGFETileElement, dom::SVGElement, SVGElement>
{
    SVGFETileElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFETileElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFETileElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFETileElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
};
