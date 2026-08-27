struct SVGLineElement : bridge::Interface<SVGLineElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGLineElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGLineElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGLineElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGLineElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x1),
    SVGLENGTH_ATTRIBUTE(x2),
    SVGLENGTH_ATTRIBUTE(y1),
    SVGLENGTH_ATTRIBUTE(y2),
};
