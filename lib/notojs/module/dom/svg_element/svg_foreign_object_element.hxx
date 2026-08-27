struct SVGForeignObjectElement : bridge::Interface<SVGForeignObjectElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGForeignObjectElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGForeignObjectElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGForeignObjectElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGForeignObjectElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
};
