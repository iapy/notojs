struct SVGStopElement : bridge::Interface<SVGStopElement, dom::SVGElement, SVGElement>
{
    SVGStopElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGStopElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue offset(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"offset"}))
            return JS_NewFloat64(ctx, std::atof(value->data()));
        return JS_NewFloat64(ctx, 0);
    }

    void set_offset(JSContext *, bridge::Number n)
    {
        auto const s = std::to_string(n.as_double());
        ref().setAttribute({"offset"}, {s.c_str(), s.size()});
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGStopElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGStopElement::funcs[] = {
    JS_CGETSET_DEF("offset", &bridge::Getter<&SVGStopElement::offset>, &bridge::Setter<&SVGStopElement::set_offset>),
};
