struct SVGEllipseElement : bridge::Interface<SVGEllipseElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGEllipseElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGEllipseElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGEllipseElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGEllipseElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(cx),
    SVGLENGTH_ATTRIBUTE(cy),
    SVGLENGTH_ATTRIBUTE(rx),
    SVGLENGTH_ATTRIBUTE(ry),
};
