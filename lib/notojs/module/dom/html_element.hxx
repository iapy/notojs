struct HTMLElement : bridge::Interface<HTMLElement, dom::HTMLElement, Element>
{
    HTMLElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    template<typename T>
    JSValue appendChild_t(JSContext *ctx, T object)
    {
        std::optional<std::string> error;
        if(auto *node = ref().appendChild(object, error); node)
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }

    using appendChild = bridge::Function
    <
        &Node::appendChild,
        &HTMLElement::appendChild_t<Image>,
        &HTMLElement::appendChild_t<SVG>
    >;

    template<typename T>
    JSValue insertBefore_t0(JSContext *ctx, T object, Node r)
    {
        if(ref().doc != r.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!ref().doc->isChild(ref(), r.ref())) return DOMException::throwNotFoundError(ctx);

        std::optional<std::string> error;
        if(auto *node = ref().insertBefore(object, r.ref(), error); node)
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }

    template<typename T>
    JSValue insertBefore_t1(JSContext *ctx, T object, bridge::Null)
    {
        return appendChild_t<T>(ctx, object);
    }

    using insertBefore = bridge::Function
    <
        &Node::insertBefore_0,
        &Node::insertBefore_1,
        &HTMLElement::insertBefore_t0<Image>,
        &HTMLElement::insertBefore_t0<SVG>,
        &HTMLElement::insertBefore_t1<Image>,
        &HTMLElement::insertBefore_t1<SVG>
    >;

    using Tail = bridge::Tail<1, Node, bridge::String, HTML, Image, SVG>;

    static std::optional<JSValue> after_f(JSContext *ctx, Tail &nodes, std::size_t j, dom::Node const &self,
        dom::Node const &parent, std::optional<dom::Node> const &next)
    {
        std::optional<std::string> error;
        if(auto h = nodes.template get<HTML>(j); h)
        {
            if(next) (void)dom::HTMLElement{parent}.insertBefore(*h, *next, error);
            else (void)dom::HTMLElement{parent}.appendChild(*h, error);
        }
        else if(auto i = nodes.template get<Image>(j); i)
        {
            if(next) (void)dom::HTMLElement{parent}.insertBefore(*i, *next, error);
            else (void)dom::HTMLElement{parent}.appendChild(*i, error);
        }
        else if(auto s = nodes.template get<SVG>(j); s)
        {
            if(next) (void)dom::HTMLElement{parent}.insertBefore(*s, *next, error);
            else (void)dom::HTMLElement{parent}.appendChild(*s, error);
        }
        if(error) DOMException::throwSyntaxError(ctx, std::move(*error));
        return std::nullopt;
    }
    using after = bridge::Function<&Element::after_t<HTMLElement, Node, bridge::String, HTML, Image, SVG>>;

    static std::optional<JSValue> append_f(JSContext *ctx, Tail &nodes, std::size_t j, dom::Node const &self)
    {
        std::optional<std::string> error;
        if(auto h = nodes.template get<HTML>(j); h)
        {
            (void)dom::HTMLElement{self}.appendChild(*h, error);
        }
        else if(auto i = nodes.template get<Image>(j); i)
        {
            (void)dom::HTMLElement{self}.appendChild(*i, error);
        }
        else if(auto s = nodes.template get<SVG>(j); s)
        {
            (void)dom::HTMLElement{self}.appendChild(*s, error);
        }
        if(error) DOMException::throwSyntaxError(ctx, std::move(*error));
        return std::nullopt;
    }
    using append = bridge::Function<&Element::append_t<HTMLElement, Node, bridge::String, HTML, Image, SVG>>;

    JSValue attributes_(JSContext *ctx)
    {
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            return JS_DupValue(ctx, it->second);
        return ref().doc->attributes[ref().node] = HTMLNamedNodeMap::from(ctx, dom::HTMLNamedNodeMap{ref()});
    }

    JSValue attributes(JSContext *ctx, JSValue)
    {
        return attributes_(ctx);
    }

    static std::optional<JSValue> before_f(JSContext *ctx, Tail &nodes, std::size_t j, dom::Node const &self,
        dom::Node const &parent)
    {
        std::optional<std::string> error;
        if(auto h = nodes.template get<HTML>(j); h)
        {
            (void)dom::HTMLElement{parent}.insertBefore(*h, self, error);
        }
        else if(auto i = nodes.template get<Image>(j); i)
        {
            (void)dom::HTMLElement{parent}.insertBefore(*i, self, error);
        }
        else if(auto s = nodes.template get<SVG>(j); s)
        {
            (void)dom::HTMLElement{parent}.insertBefore(*s, self, error);
        }
        if(error) DOMException::throwSyntaxError(ctx, std::move(*error));
        return std::nullopt;
    }
    using before = bridge::Function<&Element::before_t<HTMLElement, Node, bridge::String, HTML, Image, SVG>>;

    JSValue className(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"class"}))
            return bridge::String{ctx, *value};
        return JS_NULL;
    }

    JSValue classList(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->classes.find(ptr); it != std::end(doc->classes))
            return JS_DupValue(ctx, it->second);
        return doc->classes[ptr] = DOMTokenList::from(ctx, dom::DOMTokenList(ref(), dom::Attr::Name{"class"}));
    }

    void set_className(JSContext *ctx, bridge::Value value)
    {
        ref().setAttribute({"class"}, value.toString());
    }

    JSValue closest(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto *node = ref().closest(query, error); node)
            return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return error ? DOMException::throwSyntaxError(ctx, std::move(error)) : JS_NULL;
    }

    JSValue dataset(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->datasets.find(ptr); it != std::end(doc->datasets))
            return JS_DupValue(ctx, it->second);
        return doc->datasets[ptr] = DOMStringMap::from(ctx, dom::DOMStringMap(ref()));
    }

    JSValue getAttributeNode(JSContext *ctx, bridge::String name)
    {
        bridge::Strong<NamedNodeMap> a{ctx, attributes_(ctx)};
        return (-a).getNamedItem(a, ctx, name);
    }

    JSValue getAttributeNodeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        bridge::Strong<NamedNodeMap> a{ctx, attributes_(ctx)};
        return (-a).getNamedItem(a, ctx, name);
    }

    JSValue getAttributeNodeNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        bridge::Strong<HTMLNamedNodeMap> a{ctx, attributes_(ctx)};
        return (-a).getNamedItemNS(a, ctx, ns, name);
    }

    using getAttributeNodeNS = bridge::Function
    <
        &HTMLElement::getAttributeNodeNS_0,
        &HTMLElement::getAttributeNodeNS_1
    >;

    JSValue getAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        if(auto value = ref().getAttribute({name}))
            return bridge::String{ctx, *value};
        return JS_NULL;
    }

    JSValue getAttributeNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
        {
            if(auto value = ref().getAttribute({name, nsid}))
                return bridge::String{ctx, *value};
            return JS_NULL;
        }
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using getAttributeNS = bridge::Function
    <
        &HTMLElement::getAttributeNS_0,
        &HTMLElement::getAttributeNS_1
    >;

    JSValue hasAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return ref().hasAttribute({name}) ? JS_TRUE : JS_FALSE;
    }

    JSValue hasAttributeNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
            return ref().hasAttribute({name, nsid}) ? JS_TRUE : JS_FALSE;
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using hasAttributeNS = bridge::Function
    <
        &HTMLElement::hasAttributeNS_0,
        &HTMLElement::hasAttributeNS_1
    >;

    JSValue getElementsByTagName(JSContext *ctx, bridge::String name)
    {
        return dom::Element::isTagName(name)
            ? HTMLCollection::from(ctx, dom::HTMLCollection{
                dom::Node{ref().doc, ref().node}, name
            })
            : HTMLCollection::from(ctx, dom::HTMLCollection{
                dom::Node{ref().doc, ref().node}, std::vector<void*>{}
            });
    }

    JSValue getElementsByTagNameNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return getElementsByTagName(ctx, name);
    }

    JSValue getElementsByTagNameNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
            return dom::Element::isTagName(name)
                ? HTMLCollection::from(ctx, dom::HTMLCollection{
                    dom::Node{ref().doc, ref().node}, name, nsid
                })
                : HTMLCollection::from(ctx, dom::HTMLCollection{
                    dom::Node{ref().doc, ref().node}, std::vector<void*>{}
                });
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using getElementsByTagNameNS = bridge::Function
    <
        &HTMLElement::getElementsByTagNameNS_0,
        &HTMLElement::getElementsByTagNameNS_1
    >;

    JSValue getElementsByClassName(JSContext *ctx, bridge::String name)
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{
            dom::Node{ref().doc, ref().node}, dom::HTMLElement::className(name)
        });
    }

    ELEMENT_ATTRIBUTE_PROPERTY(id)

    JSValue innerHTML(JSContext *ctx) const
    {
        return bridge::String{ctx, std::move(ref().innerHTML())};
    }

    void set_innerHTML_0(JSContext *ctx, HTML value)
    {
        ref().innerHTML(value);
    }

    void set_innerHTML_1(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        ref().innerHTML(static_cast<std::string_view const &>(str));
    }

    using set_innerHTML = bridge::Setters
    <
        &HTMLElement::set_innerHTML_0,
        &HTMLElement::set_innerHTML_1
    >;

    JSValue innerText(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().innerText()};
    }

    void set_innerText_0(JSContext *ctx, bridge::Null)
    {
        ref().doc->textContent(ref(), std::nullopt);
    }

    void set_innerText_1(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        ref().innerText(static_cast<std::string_view const &>(str));
    }

    using set_innerText = bridge::Setters
    <
        &HTMLElement::set_innerText_0,
        &HTMLElement::set_innerText_1
    >;

    template<typename T, Element::Position::Value pos>
    BOOST_FORCEINLINE JSValue inertAdjacentElement_t(JSContext *ctx, T object)
    {
        if constexpr (Element::Position::Value::afterbegin == pos)
        {
            std::optional<std::string> error;

            auto first = ref().doc->first(ref());
            if(first)
            {
                if(auto *node = ref().insertBefore(object, *first, error); node)
                    return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
                return DOMException::throwSyntaxError(ctx, std::move(error));
            }

            if(auto *node = ref().appendChild(object, error); node)
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else if constexpr (Element::Position::Value::afterend == pos)
        {
            std::optional<std::string> error;

            auto parent = ref().doc->parent(ref());
            if(!parent)
                return DOMException::throwHierarchyRequestError(ctx);

            if(auto next = ref().doc->next(ref()))
            {
                if(auto *node = dom::HTMLElement{*parent}.insertBefore(object, *next, error); node)
                    return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
                return DOMException::throwSyntaxError(ctx, std::move(error));
            }
            if(auto *node = dom::HTMLElement{*parent}.appendChild(object, error); node)
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else if constexpr (Element::Position::Value::beforebegin == pos)
        {
            std::optional<std::string> error;

            auto parent = ref().doc->parent(ref());
            if(!parent)
                return DOMException::throwHierarchyRequestError(ctx);

            if(auto *node = dom::HTMLElement{*parent}.insertBefore(object, ref(), error); node)
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else if constexpr (Element::Position::Value::beforeend == pos)
        {
            std::optional<std::string> error;
            if(auto *node = ref().appendChild(object, error); node)
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else return JS_UNDEFINED;
    }

    template<typename T>
    JSValue insertAdjacentElement_t(JSContext *ctx, Element::Position pos, T object)
    {
        switch(static_cast<Element::Position::Value>(pos))
        {
        case Element::Position::Value::afterbegin:
            return inertAdjacentElement_t<T, Element::Position::Value::afterbegin>(ctx, object);
        case Element::Position::Value::afterend:
            return inertAdjacentElement_t<T, Element::Position::Value::afterend>(ctx, object);
        case Element::Position::Value::beforebegin:
            return inertAdjacentElement_t<T, Element::Position::Value::beforebegin>(ctx, object);
        case Element::Position::Value::beforeend:
            return inertAdjacentElement_t<T, Element::Position::Value::beforeend>(ctx, object);
        default:
            return JS_ThrowTypeError(ctx, "Wrong position argument [%s]", static_cast<std::string_view const &>(pos).data());
        }
    }

    using insertAdjacentElement = bridge::Function
    <
        &Element::insertAdjacentElement,
        &HTMLElement::insertAdjacentElement_t<Image>,
        &HTMLElement::insertAdjacentElement_t<SVG>
    >;

    JSValue insertAdjacentHTML_1(JSContext *ctx, Element::Position pos, bridge::String text)
    {
        std::optional<std::string> error;
        switch(static_cast<Element::Position::Value>(pos))
        {
        case Element::Position::Value::afterbegin:
            if(auto first = ref().doc->first(ref()))
            {
                if(auto *node = ref().insertBefore(text, *first, error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = ref().appendChild(text, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::afterend:
            if(auto parent = ref().doc->parent(ref()); !parent)
                return DOMException::throwHierarchyRequestError(ctx);
            else if(auto next = ref().doc->next(ref()))
            {
                if(auto *node = dom::HTMLElement{*parent}.insertBefore(text, *next, error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::HTMLElement{*parent}.appendChild(text, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforebegin:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else if(auto *node = dom::HTMLElement{*parent}.insertBefore(text, ref(), error))
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforeend:
            if(auto *node = ref().appendChild(text, error); node)
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        default:
            return JS_ThrowTypeError(ctx, "Wrong position argument [%s]", static_cast<std::string_view const &>(pos).data());
        }
    }

    JSValue insertAdjacentHTML_2(JSContext *ctx, Element::Position pos, HTML object)
    {
        std::optional<std::string> error;
        switch(static_cast<Element::Position::Value>(pos))
        {
        case Element::Position::Value::afterbegin:
            if(auto first = ref().doc->first(ref()))
            {
                if(auto *node = ref().insertBefore(object, *first, error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = ref().appendChild(object, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::afterend:
            if(auto parent = ref().doc->parent(ref()); !parent)
                return DOMException::throwHierarchyRequestError(ctx);
            else if(auto next = ref().doc->next(ref()))
            {
                if(auto *node = dom::HTMLElement{*parent}.insertBefore(object, *next, error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::HTMLElement{*parent}.appendChild(object, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforebegin:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else if(auto *node = dom::HTMLElement{*parent}.insertBefore(object, ref(), error))
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforeend:
            if(auto *node = ref().appendChild(object, error); node)
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        default:
            return JS_ThrowTypeError(ctx, "Wrong position argument [%s]", static_cast<std::string_view const &>(pos).data());
        }
    }

    using insertAdjacentHTML = bridge::Function
    <
        &HTMLElement::insertAdjacentHTML_1,
        &HTMLElement::insertAdjacentHTML_2
    >;

    JSValue matches(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto const result = ref().matches(query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return result ? JS_TRUE : JS_FALSE;
    }

    void set_outerHTML_0(JSContext *ctx, HTML value)
    {
        if(lxb_dom_node_t *node = ref(); node->parent)
            ref().outerHTML(value);
    }

    void set_outerHTML_1(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        if(lxb_dom_node_t *node = ref(); node->parent)
            ref().outerHTML(static_cast<std::string_view const &>(str));
    }

    using set_outerHTML = bridge::Setters
    <
        &HTMLElement::set_outerHTML_0,
        &HTMLElement::set_outerHTML_1
    >;

    void set_outerText(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        ref().outerText(static_cast<std::string_view const &>(str));
    }

    JSValue relList(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->rels.find(ptr); it != std::end(doc->rels))
            return JS_DupValue(ctx, it->second);
        return doc->rels[ptr] = DOMTokenList::from(ctx, dom::DOMTokenList{ref(), dom::Attr::Name{"rel"}});
    }

    using prepend = bridge::Function<&Element::prepend_t<HTMLElement, Node, bridge::String, HTML, Image, SVG>>;

    JSValue removeAttributeNode(JSContext *ctx, Attr attr)
    {
        if(ref().node != attr.ref().node) return DOMException::throwNotFoundError(ctx);

        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        bridge::Strong<bridge::String> n{ctx, attr.nodeName(ctx)};
        return (-m).removeNamedItem(m, ctx, n);
    }

    JSValue removeAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return ref().removeAttribute({name}), JS_UNDEFINED;
    }

    JSValue removeAttributeNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
        {
            auto key = dom::Attr::Name{name, nsid};
            if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
                NamedNodeMap::remove(it->second, key);
            return ref().removeAttribute(key), JS_UNDEFINED;
        }
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using removeAttributeNS = bridge::Function
    <
        &HTMLElement::removeAttributeNS_0,
        &HTMLElement::removeAttributeNS_1
    >;

    template<typename T>
    JSValue replaceChild_t(JSContext *ctx, T object, Node o)
    {
        if(ref().doc != o.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!ref().doc->isChild(ref(), o.ref())) return DOMException::throwNotFoundError(ctx);

        std::optional<std::string> error;
        if(auto *node = ref().insertBefore(object, o.ref(), error); node)
            return ref().doc->remove(o.ref());
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }

    using replaceChild = bridge::Function
    <
        &Node::replaceChild,
        &HTMLElement::replaceChild_t<HTML>,
        &HTMLElement::replaceChild_t<Image>,
        &HTMLElement::replaceChild_t<SVG>
    >;

    using replaceChildren = bridge::Function<&Element::replaceChildren_t<HTMLElement, Node, bridge::String, HTML, Image, SVG>>;

    JSValue setAttributeNode(JSContext *ctx, bridge::Object attr)
    {
        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        return (-m).setNamedItem(m, ctx, attr);
    }

    JSValue setAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name, bridge::Value value)
    {
        return ref().setAttribute({name}, value.toString()), JS_UNDEFINED;
    }

    JSValue setAttributeNS_1(JSContext *ctx, bridge::String ns, bridge::String name, bridge::Value value)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
            return ref().setAttribute({name, nsid}, value.toString()), JS_UNDEFINED;
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using setAttributeNS = bridge::Function
    <
        &HTMLElement::setAttributeNS_0,
        &HTMLElement::setAttributeNS_1
    >;

    JSValue style(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->styles.find(ptr); it != std::end(doc->styles))
            return JS_DupValue(ctx, it->second);
        return doc->styles[ptr] = CSSStyleProperties::from(ctx, dom::CSSStyleProperties(
            ref(), std::in_place_type<dom::CSSStyleProperties::Attr>));
    }

    JSValue toJSON(JSContext *ctx) const
    {
        if(lxb_tag_id_t tag = (lxb_tag_id_t)static_cast<lxb_dom_node_t *>(ref())->local_name;
                LXB_TAG_HTML == tag
             || LXB_TAG_HEAD == tag
             || LXB_TAG_BODY == tag
             || LXB_TAG_TITLE == tag
        ) return JS_ThrowTypeError(ctx, "<%s> cannot be serialized", dom::lexbor::get_name(static_cast<lxb_dom_element_t *>(ref())).data());
        return HTML{ctx, bridge::Strong<void>{ctx, HTML::data(ctx, toString(ctx)), false}}.toJSON(ctx);
    }

    JSValue toString(JSContext *ctx) const
    {
        return bridge::String{ctx, std::move(ref().toString())};
    }

    BOOST_FORCEINLINE static void free(dom::HTMLElement &self)
    {
        dynamic_cast<dom::HTMLBackend *>(self.doc.get())->nodes.erase(self);
    }

    struct I : Base::I<I, HTML::Interface>
    {
        using Base::Base;
        bool json() const override
        {
            lxb_tag_id_t tag = (lxb_tag_id_t)static_cast<lxb_dom_node_t *>(ref)->local_name;
            return LXB_TAG_HTML != tag && LXB_TAG_HEAD != tag && LXB_TAG_BODY != tag && LXB_TAG_TITLE != tag;
        }
        std::string get() const override
        {
            return ref.toString();
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<HTMLElement>;
    friend class Window;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLElement::funcs[] = {
    JS_CGETSET_DEF("attributes", &bridge::Getter<&HTMLElement::attributes>, NULL),
    JS_CGETSET_DEF("classList", &bridge::Getter<&HTMLElement::classList>, NULL),
    JS_CGETSET_DEF("className", &bridge::Getter<&HTMLElement::className>, &bridge::Setter<&HTMLElement::set_className>),
    JS_CGETSET_DEF("dataset", &bridge::Getter<&HTMLElement::dataset>, NULL),
    JS_CGETSET_DEF("id", &bridge::Getter<&HTMLElement::id>, &bridge::Setter<&HTMLElement::set_id>),
    JS_CGETSET_DEF("innerHTML", &bridge::Getter<&HTMLElement::innerHTML>, &HTMLElement::set_innerHTML::invoke),
    JS_CGETSET_DEF("innerText", &bridge::Getter<&HTMLElement::innerText>, &HTMLElement::set_innerText::invoke),
    JS_CGETSET_DEF("outerHTML", &bridge::Getter<&HTMLElement::toString>, &HTMLElement::set_outerHTML::invoke),
    JS_CGETSET_DEF("outerText", &bridge::Getter<&HTMLElement::innerText>, &bridge::Setter<&HTMLElement::set_outerText>),
    JS_CGETSET_DEF("style", &bridge::Getter<&HTMLElement::style>, NULL),

    JS_CFUNC_DEF("after", 1, &HTMLElement::after::invoke),
    JS_CFUNC_DEF("append", 1, &HTMLElement::append::invoke),
    JS_CFUNC_DEF("appendChild", 1, &HTMLElement::appendChild::invoke),
    JS_CFUNC_DEF("before", 1, &HTMLElement::before::invoke),
    JS_CFUNC_DEF("closest", 1, &bridge::Function<&HTMLElement::closest>::invoke),
    JS_CFUNC_DEF("getAttributeNode", 1, &bridge::Function<&HTMLElement::getAttributeNode>::invoke),
    JS_CFUNC_DEF("getAttributeNodeNS", 2, &HTMLElement::getAttributeNodeNS::invoke),
    JS_CFUNC_DEF("getAttributeNS", 2, &HTMLElement::getAttributeNS::invoke),
    JS_CFUNC_DEF("getElementsByClassName", 1, &bridge::Function<&HTMLElement::getElementsByClassName>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&HTMLElement::getElementsByTagName>::invoke),
    JS_CFUNC_DEF("getElementsByTagNameNS", 2, &HTMLElement::getElementsByTagNameNS::invoke),
    JS_CFUNC_DEF("getHTML", 0, &bridge::Function<&HTMLElement::innerHTML>::invoke),
    JS_CFUNC_DEF("hasAttributeNS", 2, &HTMLElement::hasAttributeNS::invoke),
    JS_CFUNC_DEF("insertAdjacentElement", 2, &HTMLElement::insertAdjacentElement::invoke),
    JS_CFUNC_DEF("insertAdjacentHTML", 2, &HTMLElement::insertAdjacentHTML::invoke),
    JS_CFUNC_DEF("insertBefore", 2, &HTMLElement::insertBefore::invoke),
    JS_CFUNC_DEF("matches", 1, &bridge::Function<&HTMLElement::matches>::invoke),
    JS_CFUNC_DEF("moveBefore", 2, HTMLElement::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 0, &HTMLElement::prepend::invoke),
    JS_CFUNC_DEF("removeAttributeNS", 2, &HTMLElement::removeAttributeNS::invoke),
    JS_CFUNC_DEF("removeAttributeNode", 1, &bridge::Function<&HTMLElement::removeAttributeNode>::invoke),
    JS_CFUNC_DEF("replaceChildren", 1, &HTMLElement::replaceChildren::invoke),
    JS_CFUNC_DEF("replaceChild", 2, &HTMLElement::replaceChild::invoke),
    JS_CFUNC_DEF("setAttributeNS", 3, &HTMLElement::setAttributeNS::invoke),
    JS_CFUNC_DEF("setAttributeNode", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),
    JS_CFUNC_DEF("setAttributeNodeNS", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&HTMLElement::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<HTMLElement>::toJSON)
};

struct HTMLAnchorElement : bridge::Interface<HTMLAnchorElement, dom::HTMLElement, HTMLElement>
{
    HTMLAnchorElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLAnchorElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    ELEMENT_ATTRIBUTE_PROPERTY(href)
    ELEMENT_ATTRIBUTE_PROPERTY(hreflang)
    ELEMENT_ATTRIBUTE_PROPERTY(rel)
    ELEMENT_ATTRIBUTE_PROPERTY(target)

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLAnchorElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLAnchorElement::funcs[] = {
    JS_CGETSET_DEF("href", &bridge::Getter<&HTMLAnchorElement::href>, &bridge::Setter<&HTMLAnchorElement::set_href>),
    JS_CGETSET_DEF("hreflang", &bridge::Getter<&HTMLAnchorElement::hreflang>, &bridge::Setter<&HTMLAnchorElement::set_hreflang>),
    JS_CGETSET_DEF("rel", &bridge::Getter<&HTMLAnchorElement::rel>, &bridge::Setter<&HTMLAnchorElement::set_rel>),
    JS_CGETSET_DEF("relList", &bridge::Getter<&HTMLElement::relList>, NULL),
    JS_CGETSET_DEF("target", &bridge::Getter<&HTMLAnchorElement::target>, &bridge::Setter<&HTMLAnchorElement::set_target>)
};

struct HTMLImageElement : bridge::Interface<HTMLImageElement, dom::HTMLElement, HTMLElement>
{
    HTMLImageElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLImageElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    ELEMENT_ATTRIBUTE_PROPERTY(alt)
    ELEMENT_ATTRIBUTE_PROPERTY(src)

    struct I : Base::I<I, Image::Interface>
    {
        using Base::Base;
        std::string get() const override
        {
            return std::string{ref.getAttribute({"src"}).value_or("")};
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<HTMLImageElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLImageElement::funcs[] = {
    JS_CGETSET_DEF("alt", &bridge::Getter<&HTMLImageElement::alt>, &bridge::Setter<&HTMLImageElement::set_alt>),
    JS_CGETSET_DEF("src", &bridge::Getter<&HTMLImageElement::src>, &bridge::Setter<&HTMLImageElement::set_src>)
};

struct HTMLLinkElement : bridge::Interface<HTMLLinkElement, dom::HTMLElement, HTMLElement>
{
    HTMLLinkElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLLinkElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    ELEMENT_ATTRIBUTE_PROPERTY(href)
    ELEMENT_ATTRIBUTE_PROPERTY(hreflang)
    ELEMENT_ATTRIBUTE_PROPERTY(rel)

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLLinkElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLLinkElement::funcs[] = {
    JS_CGETSET_DEF("href", &bridge::Getter<&HTMLLinkElement::href>, &bridge::Setter<&HTMLLinkElement::set_href>),
    JS_CGETSET_DEF("hreflang", &bridge::Getter<&HTMLLinkElement::hreflang>, &bridge::Setter<&HTMLLinkElement::set_hreflang>),
    JS_CGETSET_DEF("rel", &bridge::Getter<&HTMLLinkElement::rel>, &bridge::Setter<&HTMLLinkElement::set_rel>),
    JS_CGETSET_DEF("relList", &bridge::Getter<&HTMLElement::relList>, NULL)
};

struct HTMLStyleElement : bridge::Interface<HTMLStyleElement, dom::HTMLElement, HTMLElement>
{
    HTMLStyleElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLStyleElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue sheet(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->sheets.find(ptr); it != std::end(doc->sheets))
            return JS_DupValue(ctx, it->second);
        return doc->sheets[ptr] = CSSStyleSheet::from(ctx, dom::CSSStyleSheet{ref()});
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLStyleElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLStyleElement::funcs[] = {
    JS_CGETSET_DEF("sheet", &bridge::Getter<&HTMLStyleElement::sheet>, NULL)
};
