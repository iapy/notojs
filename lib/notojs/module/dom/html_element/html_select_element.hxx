struct HTMLSelectElement : bridge::Interface<HTMLSelectElement, dom::HTMLElement, HTMLElement>
{
    HTMLSelectElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLSelectElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    std::vector<void*> optionNodes() const
    {
        std::optional<std::string> error;
        return ref().doc->querySelectorAll(ref(), "option", error);
    }

    dom::HTMLElement option(lxb_dom_node_t *node) const
    {
        return dom::HTMLElement{ref().doc, lxb_html_interface_element(node)};
    }

    std::int64_t selectedIndex_() const
    {
        auto options = optionNodes();
        for(std::size_t i = 0; i < options.size(); ++i)
        {
            auto opt = option(static_cast<lxb_dom_node_t *>(options[i]));
            if(opt.hasAttribute({"selected"}))
                return static_cast<std::int64_t>(i);
        }
        if(!ref().hasAttribute({"multiple"}) && !options.empty()) return 0;
        return -1;
    }

    std::string optionValue(lxb_dom_node_t *node) const
    {
        auto opt = option(node);
        if(auto value = opt.getAttribute({"value"}))
            return std::string{*value};
        return dom::lexbor::get_text(node);
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

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, static_cast<std::uint64_t>(optionNodes().size())};
    }

    void set_length(JSContext *, bridge::Number value)
    {
        auto options = optionNodes();
        auto length = static_cast<std::int64_t>(value);
        if(length < 0) length = 0;

        while(static_cast<std::int64_t>(options.size()) > length)
        {
            lxb_dom_node_remove(static_cast<lxb_dom_node_t *>(options.back()));
            options.pop_back();
        }

        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        while(static_cast<std::int64_t>(options.size()) < length)
        {
            auto *option = doc->createElement("option", 6);
            lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), option);
            options.push_back(option);
        }
    }

    JSValue multiple(JSContext *) const
    {
        return ref().hasAttribute({"multiple"}) ? JS_TRUE : JS_FALSE;
    }

    void set_multiple(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"multiple"}, "");
        else ref().removeAttribute({"multiple"});
    }

    JSValue options(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "option"});
    }

    JSValue required(JSContext *) const
    {
        return ref().hasAttribute({"required"}) ? JS_TRUE : JS_FALSE;
    }

    void set_required(JSContext *, bridge::Value value)
    {
        if(JS_ToBool(value, value)) ref().setAttribute({"required"}, "");
        else ref().removeAttribute({"required"});
    }

    JSValue selectedIndex(JSContext *ctx) const
    {
        return bridge::Number{ctx, selectedIndex_()};
    }

    void set_selectedIndex(JSContext *, bridge::Number value)
    {
        auto options = optionNodes();
        auto index = static_cast<std::int64_t>(value);
        for(void *node: options)
            option(static_cast<lxb_dom_node_t *>(node)).removeAttribute({"selected"});
        if(index >= 0 && index < static_cast<std::int64_t>(options.size()))
            option(static_cast<lxb_dom_node_t *>(options[static_cast<std::size_t>(index)])).setAttribute({"selected"}, "");
    }

    JSValue selectedOptions(JSContext *ctx) const
    {
        std::vector<void*> selected;
        auto options = optionNodes();
        for(void *node: options)
            if(option(static_cast<lxb_dom_node_t *>(node)).hasAttribute({"selected"}))
                selected.push_back(node);
        if(selected.empty() && !ref().hasAttribute({"multiple"}) && !options.empty())
            selected.push_back(options.front());
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, std::move(selected)});
    }

    JSValue size(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"size"}))
        {
            std::string str{*value};
            auto size = u64(str.c_str());
            if(size > 0) return bridge::Number{ctx, size};
        }
        return bridge::Number{ctx, std::int64_t{0}};
    }

    void set_size(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"size"}, value.toString());
    }

    JSValue type(JSContext *ctx) const
    {
        if(ref().hasAttribute({"multiple"}))
            return bridge::String{ctx, std::string_view{"select-multiple"}};
        return bridge::String{ctx, std::string_view{"select-one"}};
    }

    JSValue value(JSContext *ctx) const
    {
        auto options = optionNodes();
        auto index = selectedIndex_();
        if(index >= 0 && index < static_cast<std::int64_t>(options.size()))
            return bridge::String{ctx, optionValue(static_cast<lxb_dom_node_t *>(options[static_cast<std::size_t>(index)]))};
        return bridge::String{ctx};
    }

    void set_value(JSContext *, bridge::Value value)
    {
        auto str = value.toString();
        auto options = optionNodes();
        bool selected{false};
        for(void *node: options)
        {
            auto opt = option(static_cast<lxb_dom_node_t *>(node));
            if(!selected && optionValue(static_cast<lxb_dom_node_t *>(node)) == static_cast<std::string_view>(str))
            {
                opt.setAttribute({"selected"}, "");
                selected = true;
            }
            else
            {
                opt.removeAttribute({"selected"});
            }
        }
    }

    JSValue item(JSContext *, bridge::Number index) const
    {
        auto options = optionNodes();
        auto i = static_cast<std::int64_t>(index);
        if(i >= 0 && i < static_cast<std::int64_t>(options.size()))
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(static_cast<lxb_dom_node_t *>(options[static_cast<std::size_t>(i)]));
        return JS_NULL;
    }

    JSValue namedItem(JSContext *, bridge::String name) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        for(void *node: optionNodes())
        {
            auto opt = option(static_cast<lxb_dom_node_t *>(node));
            if(auto value = opt.getAttribute({"id"}); value && *value == static_cast<std::string_view>(name))
                return doc->make(static_cast<lxb_dom_node_t *>(node));
            if(auto value = opt.getAttribute({"name"}); value && *value == static_cast<std::string_view>(name))
                return doc->make(static_cast<lxb_dom_node_t *>(node));
        }
        return JS_NULL;
    }

    JSValue add(JSContext *ctx, HTMLElement element)
    {
        auto *node = static_cast<lxb_dom_node_t *>(element.ref());
        auto tag = lxb_dom_node_tag_id(node);
        if(LXB_TAG_OPTION != tag && LXB_TAG_OPTGROUP != tag)
            return JS_ThrowTypeError(ctx, "Element must be an HTMLOptionElement or HTMLOptGroupElement");
        lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(ref()), node);
        return JS_UNDEFINED;
    }

    JSValue remove_(JSContext *, bridge::Number index)
    {
        auto options = optionNodes();
        auto i = static_cast<std::int64_t>(index);
        if(i >= 0 && i < static_cast<std::int64_t>(options.size()))
            lxb_dom_node_remove(static_cast<lxb_dom_node_t *>(options[static_cast<std::size_t>(i)]));
        return JS_UNDEFINED;
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLSelectElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLSelectElement::funcs[] = {
    REFLECTING_ATTRIBUTE(autocomplete),
    REFLECTING_ATTRIBUTE(name),

    JS_CGETSET_DEF("disabled", &bridge::Getter<&HTMLSelectElement::disabled>, &bridge::Setter<&HTMLSelectElement::set_disabled>),
    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLElement::form>, NULL),
    JS_CGETSET_DEF("length", &bridge::Getter<&HTMLSelectElement::length>, &bridge::Setter<&HTMLSelectElement::set_length>),
    JS_CGETSET_DEF("multiple", &bridge::Getter<&HTMLSelectElement::multiple>, &bridge::Setter<&HTMLSelectElement::set_multiple>),
    JS_CGETSET_DEF("options", &bridge::Getter<&HTMLSelectElement::options>, NULL),
    JS_CGETSET_DEF("required", &bridge::Getter<&HTMLSelectElement::required>, &bridge::Setter<&HTMLSelectElement::set_required>),
    JS_CGETSET_DEF("selectedIndex", &bridge::Getter<&HTMLSelectElement::selectedIndex>, &bridge::Setter<&HTMLSelectElement::set_selectedIndex>),
    JS_CGETSET_DEF("selectedOptions", &bridge::Getter<&HTMLSelectElement::selectedOptions>, NULL),
    JS_CGETSET_DEF("size", &bridge::Getter<&HTMLSelectElement::size>, &bridge::Setter<&HTMLSelectElement::set_size>),
    JS_CGETSET_DEF("type", &bridge::Getter<&HTMLSelectElement::type>, NULL),
    JS_CGETSET_DEF("value", &bridge::Getter<&HTMLSelectElement::value>, &bridge::Setter<&HTMLSelectElement::set_value>),
    JS_CFUNC_DEF("add", 1, &bridge::Function<&HTMLSelectElement::add>::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&HTMLSelectElement::item>::invoke),
    JS_CFUNC_DEF("namedItem", 1, &bridge::Function<&HTMLSelectElement::namedItem>::invoke),
    JS_CFUNC_DEF("remove", 1, &bridge::Function<&HTMLSelectElement::remove_>::invoke)
};
