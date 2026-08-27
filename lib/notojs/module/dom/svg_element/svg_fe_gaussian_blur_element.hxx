struct SVGFEGaussianBlurElement : bridge::Interface<SVGFEGaussianBlurElement, dom::SVGElement, SVGElement>
{
    SVGFEGaussianBlurElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEGaussianBlurElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEGaussianBlurElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEGaussianBlurElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(stdDeviation),
};
