struct SVGFEMorphologyElement : bridge::Interface<SVGFEMorphologyElement, dom::SVGElement, SVGElement>
{
    SVGFEMorphologyElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEMorphologyElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue get_operator(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"operator"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_operator(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"operator"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEMorphologyElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEMorphologyElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(radius),

    JS_CGETSET_DEF("operator", &bridge::Getter<&SVGFEMorphologyElement::get_operator>, &bridge::Setter<&SVGFEMorphologyElement::set_operator>),
};
