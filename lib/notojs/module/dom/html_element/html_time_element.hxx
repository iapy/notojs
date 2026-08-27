struct HTMLTimeElement : bridge::Interface<HTMLTimeElement, dom::HTMLElement, HTMLElement>
{
    HTMLTimeElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTimeElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue dateTime(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"datetime"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_dateTime(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"datetime"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTimeElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTimeElement::funcs[] = {
    JS_CGETSET_DEF("dateTime", &bridge::Getter<&HTMLTimeElement::dateTime>, &bridge::Setter<&HTMLTimeElement::set_dateTime>)
};
