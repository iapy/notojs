struct HTMLLabelElement : bridge::Interface<HTMLLabelElement, dom::HTMLElement, HTMLElement>
{
    HTMLLabelElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLLabelElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    BOOST_FORCEINLINE static bool isLabelable(lxb_dom_node_t *node)
    {
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_BUTTON:
        case LXB_TAG_INPUT:
        case LXB_TAG_METER:
        case LXB_TAG_OUTPUT:
        case LXB_TAG_PROGRESS:
        case LXB_TAG_SELECT:
        case LXB_TAG_TEXTAREA:
            return true;
        default:
            return false;
        }
    }

    static lxb_dom_element_t *elementById(lxb_dom_node_t *root, std::string_view id)
    {
        std::size_t len{0};
        if(auto *value = lxb_dom_element_get_attribute(lxb_dom_interface_element(root), reinterpret_cast<lxb_char_t const *>("id"), 2, &len))
            if(id == std::string_view{reinterpret_cast<char const *>(value), len}) return lxb_dom_interface_element(root);
        return lxb_dom_element_by_id(lxb_dom_interface_element(root), reinterpret_cast<lxb_char_t const *>(id.data()), id.size());
    }

    static lxb_dom_node_t *firstDescendantLabelable(lxb_dom_node_t *root)
    {
        for(lxb_dom_node_t *node = root->first_child; node;)
        {
            if(isLabelable(node)) return node;
            if(node->first_child)
            {
                node = node->first_child;
                continue;
            }

            while(node && node != root && !node->next)
                node = node->parent;
            node = node && node != root ? node->next : nullptr;
        }
        return nullptr;
    }

    lxb_dom_node_t *control_() const
    {
        if(auto id = ref().getAttribute({"for"}); id && !id->empty())
        {
            if(auto *el = elementById(ref().root(), *id))
                if(auto *node = lxb_dom_interface_node(el); isLabelable(node)) return node;
            return nullptr;
        }
        return firstDescendantLabelable(static_cast<lxb_dom_node_t *>(ref()));
    }

    JSValue control(JSContext *) const
    {
        auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto *node = control_())
            return doc->make(node);
        return JS_NULL;
    }

    JSValue form(JSContext *ctx) const
    {
        auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto *node = control_();
        if(!node) return JS_NULL;

        dom::HTMLElement control{ref().doc, lxb_html_interface_element(node)};
        if(auto id = control.getAttribute({"form"}))
        {
            if(auto *el = elementById(control.root(), *id))
                if(LXB_TAG_FORM == lxb_dom_node_tag_id(lxb_dom_interface_node(el))) return doc->make(lxb_dom_interface_node(el));
            return JS_NULL;
        }

        std::optional<std::string> error;
        if(auto *form = control.closest(LXB_TAG_FORM); form)
            return doc->make(form);
        return JS_NULL;
    }

    JSValue htmlFor(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"for"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_htmlFor(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"for"}, value.toString());
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLLabelElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLLabelElement::funcs[] = {
    JS_CGETSET_DEF("control", &bridge::Getter<&HTMLLabelElement::control>, NULL),
    JS_CGETSET_DEF("form", &bridge::Getter<&HTMLLabelElement::form>, NULL),
    JS_CGETSET_DEF("htmlFor", &bridge::Getter<&HTMLLabelElement::htmlFor>, &bridge::Setter<&HTMLLabelElement::set_htmlFor>)
};
