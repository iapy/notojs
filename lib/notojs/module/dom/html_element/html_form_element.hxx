struct HTMLFormElement : bridge::Interface<HTMLFormElement, dom::HTMLElement, HTMLElement>
{
    HTMLFormElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLFormElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    static constexpr std::string_view const tags = "button, fieldset, input, object, output, select, textarea";

    std::vector<void*> elements_() const
    {
        auto *form = static_cast<lxb_dom_node_t *>(ref());
        auto *root = ref().root();

        std::size_t len{0};
        auto *fid = lxb_dom_element_get_attribute(lxb_dom_interface_element(form), reinterpret_cast<lxb_char_t const *>("id"), 2, &len);

        std::optional<std::string> error;
        auto controls = ref().doc->querySelectorAll(dom::Node{ref().doc, root}, tags, error);

        controls.erase(std::remove_if(std::begin(controls), std::end(controls), [form, fid, len](void *ptr) {
            auto *node = static_cast<lxb_dom_node_t *>(ptr);
            std::size_t l{0};

            if(auto *id = lxb_dom_element_get_attribute(lxb_dom_interface_element(node), reinterpret_cast<lxb_char_t const *>("form"), 4, &l))
                return  !fid || len != l || std::strncmp((const char *)fid, (const char *)id, l);

            for(auto *n = node->parent; n; n = n->parent)
                if(LXB_TAG_FORM == lxb_dom_node_tag_id(n)) return n != form;
            return true;
        }), std::end(controls));

        return controls;
    }

    JSValue elements(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, HTMLCollection::Wrapped{
            std::in_place_type_t<dom::HTMLCollection>{}, dom::Node{ref().doc, ref().node}, elements_()
        });
    }

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, static_cast<std::int64_t>(elements_().size())};
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLFormElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLFormElement::funcs[] = {
    REFLECTING_ATTRIBUTE(action),
    REFLECTING_ATTRIBUTE(autocomplete),
    REFLECTING_ATTRIBUTE(enctype),
    REFLECTING_ATTRIBUTE(encoding, enctype),
    REFLECTING_ATTRIBUTE(method),
    REFLECTING_ATTRIBUTE(name),
    REFLECTING_ATTRIBUTE(rel),
    REFLECTING_ATTRIBUTE(target),

    JS_CGETSET_DEF("elements", &bridge::Getter<&HTMLFormElement::elements>, NULL),
    JS_CGETSET_DEF("length", &bridge::Getter<&HTMLFormElement::length>, NULL),
    JS_CGETSET_DEF("relList", &bridge::Getter<&HTMLElement::relList>, NULL),
};
