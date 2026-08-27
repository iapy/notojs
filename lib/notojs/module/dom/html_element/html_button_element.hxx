struct HTMLButtonElement : bridge::Interface<HTMLButtonElement, dom::HTMLElement, HTMLElement>
{
    HTMLButtonElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLButtonElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue get_type(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"type"}))
        {
            std::string type{*value};
            std::transform(std::begin(type), std::end(type), std::begin(type), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if(type == "button" || type == "reset" || type == "submit")
                return bridge::String{ctx, type};
        }
        return bridge::String{ctx, std::string_view{"submit"}};
    }

    void set_type(JSContext *ctx, bridge::Value value)
    {
        ref().setAttribute({"type"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLButtonElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLButtonElement::funcs[] = {
    REFLECTING_ATTRIBUTE(command),
    REFLECTING_ATTRIBUTE(name),
    REFLECTING_ATTRIBUTE(value),

    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&HTMLButtonElement::get_type>, &bridge::Setter<&HTMLButtonElement::set_type>),
};
