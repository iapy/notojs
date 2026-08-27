struct SVGFEColorMatrixElement : bridge::Interface<SVGFEColorMatrixElement, dom::SVGElement, SVGElement>
{
    SVGFEColorMatrixElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEColorMatrixElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEColorMatrixElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEColorMatrixElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(type),
    REFLECTING_ATTRIBUTE(values),
};
