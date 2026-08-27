struct SVGMaskElement : bridge::Interface<SVGMaskElement, dom::SVGElement, SVGElement>
{
    SVGMaskElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGMaskElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGMaskElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGMaskElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
    REFLECTING_ATTRIBUTE(maskUnits),
    REFLECTING_ATTRIBUTE(maskContentUnits),
};
