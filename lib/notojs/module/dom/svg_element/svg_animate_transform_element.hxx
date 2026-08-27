struct SVGAnimateTransformElement : bridge::Interface<SVGAnimateTransformElement, dom::SVGElement, SVGAnimationElement>
{
    SVGAnimateTransformElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGAnimateTransformElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGAnimateTransformElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGAnimateTransformElement::funcs[] = {
    REFLECTING_ATTRIBUTE(type),
};
