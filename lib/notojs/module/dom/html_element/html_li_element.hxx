struct HTMLLIElement : bridge::Interface<HTMLLIElement, dom::HTMLElement, HTMLElement>
{
    HTMLLIElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLLIElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue value(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"value"}))
        {
            std::string str{*value};
            return bridge::Number{ctx, std::int64_t{std::strtoll(str.c_str(), nullptr, 10)}};
        }
        return bridge::Number{ctx, std::int64_t{0}};
    }

    void set_value(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"value"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLLIElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLLIElement::funcs[] = {
    JS_CGETSET_DEF("value", &bridge::Getter<&HTMLLIElement::value>, &bridge::Setter<&HTMLLIElement::set_value>)
};
