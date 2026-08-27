struct SVGFEFloodElement : bridge::Interface<SVGFEFloodElement, dom::SVGElement, SVGElement>
{
    SVGFEFloodElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEFloodElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEFloodElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEFloodElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES,
};
