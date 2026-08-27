struct SVGFEComponentTransferElement : bridge::Interface<SVGFEComponentTransferElement, dom::SVGElement, SVGElement>
{
    SVGFEComponentTransferElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEComponentTransferElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEComponentTransferElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEComponentTransferElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
};
