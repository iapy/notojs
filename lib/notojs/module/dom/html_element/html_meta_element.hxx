struct HTMLMetaElement : bridge::Interface<HTMLMetaElement, dom::HTMLElement, HTMLElement>
{
    HTMLMetaElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLMetaElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue get_httpEquiv(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"http-equiv"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_httpEquiv(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"http-equiv"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLMetaElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLMetaElement::funcs[] = {
    REFLECTING_ATTRIBUTE(content),
    REFLECTING_ATTRIBUTE(media),
    REFLECTING_ATTRIBUTE(name),

    JS_CGETSET_DEF("httpEquiv", &bridge::Getter<&HTMLMetaElement::get_httpEquiv>, &bridge::Setter<&HTMLMetaElement::set_httpEquiv>),
};
