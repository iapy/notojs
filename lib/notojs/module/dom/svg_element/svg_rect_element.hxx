struct SVGRectElement : bridge::Interface<SVGRectElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGRectElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGRectElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGRectElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGRectElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
    SVGLENGTH_ATTRIBUTE(rx),
    SVGLENGTH_ATTRIBUTE(ry),
};
