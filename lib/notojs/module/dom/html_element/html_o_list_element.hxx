struct HTMLOListElement : bridge::Interface<HTMLOListElement, dom::HTMLElement, HTMLElement>
{
    HTMLOListElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLOListElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue reversed(JSContext *) const
    {
        return ref().hasAttribute({"reversed"}) ? JS_TRUE : JS_FALSE;
    }

    void set_reversed(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"reversed"}, "");
        else ref().removeAttribute({"reversed"});
    }

    JSValue start(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"start"}))
        {
            std::string str{*value};
            return bridge::Number{ctx, std::int64_t{std::strtoll(str.c_str(), nullptr, 10)}};
        }
        return bridge::Number{ctx, std::int64_t{1}};
    }

    void set_start(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"start"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLOListElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLOListElement::funcs[] = {
    REFLECTING_ATTRIBUTE(type),

    JS_CGETSET_DEF("reversed", &bridge::Getter<&HTMLOListElement::reversed>, &bridge::Setter<&HTMLOListElement::set_reversed>),
    JS_CGETSET_DEF("start", &bridge::Getter<&HTMLOListElement::start>, &bridge::Setter<&HTMLOListElement::set_start>),
};
