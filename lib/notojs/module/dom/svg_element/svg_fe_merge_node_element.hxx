struct SVGFEMergeNodeElement : bridge::Interface<SVGFEMergeNodeElement, dom::SVGElement, SVGElement>
{
    SVGFEMergeNodeElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEMergeNodeElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEMergeNodeElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEMergeNodeElement::funcs[] = {
    JS_CGETSET_DEF("in", &bridge::Getter<&SVGElement::get_in>, &bridge::Setter<&SVGElement::set_in>),
};
