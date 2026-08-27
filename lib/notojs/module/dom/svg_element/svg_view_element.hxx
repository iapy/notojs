struct SVGViewElement : bridge::Interface<SVGViewElement, dom::SVGElement, SVGElement>
{
    SVGViewElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGViewElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGViewElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGViewElement::funcs[] = {
    REFLECTING_ATTRIBUTE(preserveAspectRatio),

    JS_CGETSET_DEF("viewBox", &bridge::Getter<&SVGElement::viewBox>, NULL),
};
