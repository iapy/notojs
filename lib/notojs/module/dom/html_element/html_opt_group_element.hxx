struct HTMLOptGroupElement : bridge::Interface<HTMLOptGroupElement, dom::HTMLElement, HTMLElement>
{
    HTMLOptGroupElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLOptGroupElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue disabled(JSContext *) const
    {
        return ref().hasAttribute({"disabled"}) ? JS_TRUE : JS_FALSE;
    }

    void set_disabled(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"disabled"}, "");
        else ref().removeAttribute({"disabled"});
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLOptGroupElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLOptGroupElement::funcs[] = {
    REFLECTING_ATTRIBUTE(label),

    JS_CGETSET_DEF("disabled", &bridge::Getter<&HTMLOptGroupElement::disabled>, &bridge::Setter<&HTMLOptGroupElement::set_disabled>),
};
