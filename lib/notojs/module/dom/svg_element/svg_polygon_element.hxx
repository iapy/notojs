struct SVGPolygonElement : bridge::Interface<SVGPolygonElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGPolygonElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGPolygonElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGPolygonElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGPolygonElement::funcs[] = {
    REFLECTING_ATTRIBUTE(points),
};
