struct HTMLFieldSetElement : bridge::Interface<HTMLFieldSetElement, dom::HTMLElement, HTMLElement>
{
    HTMLFieldSetElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLFieldSetElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    static constexpr std::string_view const tags = "button, fieldset, input, object, output, select, textarea";

    std::vector<void*> elements_() const
    {
        std::optional<std::string> error;
        return ref().doc->querySelectorAll(ref(), tags, error);
    }

    JSValue disabled(JSContext *) const
    {
        return ref().hasAttribute({"disabled"}) ? JS_TRUE : JS_FALSE;
    }

    void set_disabled(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"disabled"}, "");
        else ref().removeAttribute({"disabled"});
    }

    JSValue elements(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, HTMLCollection::Wrapped{
            std::in_place_type_t<dom::HTMLCollection>{}, dom::Node{ref().doc, ref().node}, elements_()
        });
    }

    JSValue type(JSContext *ctx) const
    {
        return bridge::String{ctx, std::string_view{"fieldset"}};
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLFieldSetElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLFieldSetElement::funcs[] = {
    REFLECTING_ATTRIBUTE(name),

    JS_CGETSET_DEF("disabled", &bridge::Getter<&HTMLFieldSetElement::disabled>, &bridge::Setter<&HTMLFieldSetElement::set_disabled>),
    JS_CGETSET_DEF("elements", &bridge::Getter<&HTMLFieldSetElement::elements>, NULL),
    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&HTMLFieldSetElement::type>, NULL)
};
