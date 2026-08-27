struct HTMLTextAreaElement : bridge::Interface<HTMLTextAreaElement, dom::HTMLElement, HTMLElement>
{
    HTMLTextAreaElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLTextAreaElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue value(JSContext *ctx) const
    {
        return ref().doc->textContent(ref());
    }

    void set_value(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        ref().doc->textContent(ref(), static_cast<std::string_view const &>(str));
    }

    JSValue defaultValue(JSContext *ctx) const
    {
        return value(ctx);
    }

    void set_defaultValue(JSContext *ctx, bridge::Value value)
    {
        set_value(ctx, value);
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLTextAreaElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLTextAreaElement::funcs[] = {
    JS_CGETSET_DEF("defaultValue", &bridge::Getter<&HTMLTextAreaElement::defaultValue>, &bridge::Setter<&HTMLTextAreaElement::set_defaultValue>),
    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("value", &bridge::Getter<&HTMLTextAreaElement::value>, &bridge::Setter<&HTMLTextAreaElement::set_value>)
};
