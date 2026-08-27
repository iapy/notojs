struct SVGFEOffsetElement : bridge::Interface<SVGFEOffsetElement, dom::SVGElement, SVGElement>
{
    SVGFEOffsetElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEOffsetElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEOffsetElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEOffsetElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(dx),
    SVGLENGTH_ATTRIBUTE(dy),
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
};
