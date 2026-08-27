struct SVGFEImageElement : bridge::Interface<SVGFEImageElement, dom::SVGElement, SVGElement>
{
    SVGFEImageElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGFEImageElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue get_crossOrigin(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"crossorigin"}))
            return bridge::String{ctx, *value};
        if(auto value = ref().getAttribute({"crossOrigin"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_crossOrigin(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"crossorigin"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGFEImageElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGFEImageElement::funcs[] = {
    SVG_FILTER_PRIMITIVE_ATTRIBUTES,
    REFLECTING_ATTRIBUTE(preserveAspectRatio),

    JS_CGETSET_DEF("href", &bridge::Getter<&SVGElement::href>, &bridge::Setter<&SVGElement::set_href>),
    JS_CGETSET_DEF("crossOrigin", &bridge::Getter<&SVGFEImageElement::get_crossOrigin>, &bridge::Setter<&SVGFEImageElement::set_crossOrigin>),
};
