struct SVGFEMergeElement : bridge::Interface<SVGFEMergeElement, dom::SVGElement, SVGElement>
{
    SVGFEMergeElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEMergeElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEMergeElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEMergeElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES,
};
