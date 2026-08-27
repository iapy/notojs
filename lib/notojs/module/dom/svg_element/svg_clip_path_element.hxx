struct SVGClipPathElement : bridge::Interface<SVGClipPathElement, dom::SVGElement, SVGElement>
{
    SVGClipPathElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGClipPathElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGClipPathElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGClipPathElement::funcs[] = {
    REFLECTING_ATTRIBUTE(clipPathUnits),
};
