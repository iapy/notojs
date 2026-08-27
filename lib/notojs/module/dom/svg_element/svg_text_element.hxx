struct SVGTextElement : bridge::Interface<SVGTextElement, dom::SVGElement, SVGTextContentElement>
{
    SVGTextElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGTextElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGTextElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGTextElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(dx),
    SVGLENGTH_ATTRIBUTE(dy),
    SVGLENGTH_ATTRIBUTE(rotate),
};
