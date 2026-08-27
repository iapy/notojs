struct HTMLInputElement : bridge::Interface<HTMLInputElement, dom::HTMLElement, HTMLElement>
{
    HTMLInputElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLInputElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue get_type(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"type"}))
        {
            std::string type{*value};
            std::transform(std::begin(type), std::end(type), std::begin(type), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if(types.count(type)) return bridge::String{ctx, type};
        }
        return bridge::String{ctx, std::string_view{"text"}};
    }

    void set_type(JSContext *ctx, bridge::Value value)
    {
        ref().setAttribute({"type"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLInputElement>;
    static JSCFunctionListEntry const funcs[];
    static std::unordered_set<std::string_view> const types;
};

std::unordered_set<std::string_view> const HTMLInputElement::types{
    "button", "checkbox", "color", "date", "datetime-local", "email", "file", "hidden", "image", "month", "number",
    "password", "radio", "range", "reset", "search", "submit", "tel", "text", "time", "url", "week"
};

JSCFunctionListEntry const HTMLInputElement::funcs[] = {
    REFLECTING_ATTRIBUTE(name),
    REFLECTING_ATTRIBUTE(value),

    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&HTMLInputElement::get_type>, &bridge::Setter<&HTMLInputElement::set_type>),
};
