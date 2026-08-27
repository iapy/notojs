struct SVGSymbolElement : bridge::Interface<SVGSymbolElement, dom::SVGElement, SVGElement>
{
    SVGSymbolElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGSymbolElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGSymbolElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGSymbolElement::funcs[] = {
    JS_CGETSET_DEF("viewBox", &bridge::Getter<&SVGElement::viewBox>, NULL),
};
