struct SVGPolylineElement : bridge::Interface<SVGPolylineElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGPolylineElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGPolylineElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGPolylineElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGPolylineElement::funcs[] = {
    REFLECTING_ATTRIBUTE(points),
};
