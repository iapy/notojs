struct SVGFEBlendElement : bridge::Interface<SVGFEBlendElement, dom::SVGElement, SVGElement>
{
    SVGFEBlendElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEBlendElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEBlendElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEBlendElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(in2),
    REFLECTING_ATTRIBUTE(mode),
};
