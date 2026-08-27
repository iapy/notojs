struct HTMLOptionElement : bridge::Interface<HTMLOptionElement, dom::HTMLElement, HTMLElement>
{
    HTMLOptionElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLOptionElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    lxb_dom_node_t *select() const
    {
        for(auto *node = static_cast<lxb_dom_node_t *>(ref())->parent; node; node = node->parent)
            if(LXB_TAG_SELECT == lxb_dom_node_tag_id(node)) return node;
        return nullptr;
    }

    std::vector<void*> options(lxb_dom_node_t *select) const
    {
        std::optional<std::string> error;
        return ref().doc->querySelectorAll(dom::Node{ref().doc, select}, "option", error);
    }

    std::string text_() const
    {
        return dom::lexbor::get_text(static_cast<lxb_dom_node_t *>(ref()));
    }

    JSValue defaultSelected(JSContext *) const
    {
        return ref().hasAttribute({"selected"}) ? JS_TRUE : JS_FALSE;
    }

    void set_defaultSelected(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"selected"}, "");
        else ref().removeAttribute({"selected"});
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

    JSValue index(JSContext *ctx) const
    {
        auto *self = static_cast<lxb_dom_node_t *>(ref());
        if(auto *sel = select())
        {
            auto opts = options(sel);
            for(std::size_t i = 0; i < opts.size(); ++i)
                if(opts[i] == self) return bridge::Number{ctx, static_cast<std::int64_t>(i)};
        }
        return bridge::Number{ctx, std::int64_t{-1}};
    }

    JSValue label(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"label"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx, text_()};
    }

    void set_label(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"label"}, value.toString());
    }

    JSValue selected(JSContext *) const
    {
        return ref().hasAttribute({"selected"}) ? JS_TRUE : JS_FALSE;
    }

    void set_selected(JSContext *, bridge::Value value)
    {
        bool const selected = JS_ToBool(value, value);
        if(selected)
        {
            if(auto *sel = select())
            {
                dom::HTMLElement select{ref().doc, lxb_html_interface_element(sel)};
                if(!select.hasAttribute({"multiple"}))
                    for(void *node: options(sel))
                        dom::HTMLElement{ref().doc, lxb_html_interface_element(static_cast<lxb_dom_node_t *>(node))}.removeAttribute({"selected"});
            }
            ref().setAttribute({"selected"}, "");
        }
        else
        {
            ref().removeAttribute({"selected"});
        }
    }

    JSValue text(JSContext *ctx) const
    {
        return bridge::String{ctx, text_()};
    }

    void set_text(JSContext *, bridge::Value value)
    {
        auto str = value.toString();
        ref().doc->textContent(ref(), static_cast<std::string_view const &>(str));
    }

    JSValue value(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"value"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx, text_()};
    }

    void set_value(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"value"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLOptionElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLOptionElement::funcs[] = {
    JS_CGETSET_DEF("defaultSelected", &bridge::Getter<&HTMLOptionElement::defaultSelected>, &bridge::Setter<&HTMLOptionElement::set_defaultSelected>),
    JS_CGETSET_DEF("disabled", &bridge::Getter<&HTMLOptionElement::disabled>, &bridge::Setter<&HTMLOptionElement::set_disabled>),
    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("index", &bridge::Getter<&HTMLOptionElement::index>, NULL),
    JS_CGETSET_DEF("label", &bridge::Getter<&HTMLOptionElement::label>, &bridge::Setter<&HTMLOptionElement::set_label>),
    JS_CGETSET_DEF("selected", &bridge::Getter<&HTMLOptionElement::selected>, &bridge::Setter<&HTMLOptionElement::set_selected>),
    JS_CGETSET_DEF("text", &bridge::Getter<&HTMLOptionElement::text>, &bridge::Setter<&HTMLOptionElement::set_text>),
    JS_CGETSET_DEF("value", &bridge::Getter<&HTMLOptionElement::value>, &bridge::Setter<&HTMLOptionElement::set_value>)
};
