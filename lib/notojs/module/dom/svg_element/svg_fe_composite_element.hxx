struct SVGFECompositeElement : bridge::Interface<SVGFECompositeElement, dom::SVGElement, SVGElement>
{
    SVGFECompositeElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFECompositeElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

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
    using ctor = bridge::Unconstructable<SVGFECompositeElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFECompositeElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN,
    REFLECTING_ATTRIBUTE(in2),
    REFLECTING_ATTRIBUTE(k1),
    REFLECTING_ATTRIBUTE(k2),
    REFLECTING_ATTRIBUTE(k3),
    REFLECTING_ATTRIBUTE(k4),

    JS_CGETSET_DEF("operator", &bridge::Getter<&SVGFECompositeElement::get_operator>, &bridge::Setter<&SVGFECompositeElement::set_operator>),
};
