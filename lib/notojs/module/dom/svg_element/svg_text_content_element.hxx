struct SVGTextContentElement : bridge::Interface<SVGTextContentElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGTextContentElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGTextContentElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGTextContentElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGTextContentElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(textLength),
};
