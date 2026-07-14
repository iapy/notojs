struct Element : bridge::Interface<Element, dom::Element, Node>
{
    Element(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Element(std::reference_wrapper<dom::Element> &&rw) : Base(std::move(rw)) {}

    template<typename Fallback, typename ...Ts>
    JSValue after_t(JSContext *ctx, bridge::Tail<1, Ts...> nodes)
    {
        auto parent = ref().doc->parent(ref());
        if(!parent) return DOMException::throwHierarchyRequestError(ctx);

        auto next = ref().doc->next(ref());
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(ref().doc != n->ref().doc) return DOMException::throwWrongDocumentError(ctx);
                if(ref().doc->contains(n->ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);

                if(next) ref().doc->insertBeforeVoid(*parent, n->ref(), *next);
                else ref().doc->appendChildVoid(*parent, n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                if(next) ref().doc->insertBeforeVoid(*parent, ref().doc->createTextNode(*s), *next);
                else ref().doc->appendChildVoid(*parent, ref().doc->createTextNode(*s));
            }
            else if(auto e = Fallback::after_f(ctx, nodes, i, ref(), *parent, next)) return *e;
        }
        return JS_UNDEFINED;
    }

    BOOST_FORCEINLINE std::optional<JSValue> after_f(JSContext *, bridge::Tail<1, Node, bridge::String> &, std::size_t, dom::Node const &, dom::Node const &, std::optional<dom::Node> const &)
    {
        return std::nullopt;
    }
    using after = bridge::Function<&Element::after_t<Element, Node, bridge::String>>;

    template<typename Fallback, typename ...Ts>
    JSValue append_t(JSContext *ctx, bridge::Tail<1, Ts...> nodes)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(ref().doc != n->ref().doc) return DOMException::throwWrongDocumentError(ctx);
                if(ref().doc->contains(n->ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
                ref().doc->appendChildVoid(ref(), n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                ref().doc->appendChildVoid(ref(), ref().doc->createTextNode(*s));
            }
            else if(auto e = Fallback::append_f(ctx, nodes, i, ref())) return *e;
        }
        return JS_UNDEFINED;
    }

    BOOST_FORCEINLINE std::optional<JSValue> append_f(JSContext *, bridge::Tail<1, Node, bridge::String> &, std::size_t, dom::Node const &)
    {
        return std::nullopt;
    }
    using append = bridge::Function<&Element::append_t<Element, Node, bridge::String>>;

    JSValue attributes_(JSContext *ctx)
    {
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            return JS_DupValue(ctx, it->second);
        return ref().doc->attributes[ref().node] = NamedNodeMap::from(ctx, dom::NamedNodeMap{ref()});
    }

    JSValue attributes(JSContext *ctx, JSValue)
    {
        return attributes_(ctx);
    }

    template<typename Fallback, typename ...Ts>
    JSValue before_t(JSContext *ctx, bridge::Tail<1, Ts...> nodes)
    {
        auto parent = ref().doc->parent(ref());
        if(!parent) return DOMException::throwHierarchyRequestError(ctx);

        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(ref().doc != n->ref().doc) return DOMException::throwWrongDocumentError(ctx);
                if(ref().doc->contains(n->ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
                ref().doc->insertBeforeVoid(*parent, n->ref(), ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                ref().doc->insertBeforeVoid(*parent, ref().doc->createTextNode(*s), ref());
            }
            else if(auto e = Fallback::before_f(ctx, nodes, i, ref(), *parent)) return *e;
        }
        return JS_UNDEFINED;
    }

    BOOST_FORCEINLINE std::optional<JSValue> before_f(JSContext *, bridge::Tail<1, Node, bridge::String> &, std::size_t, dom::Node const &, dom::Node const &)
    {
        return std::nullopt;
    }
    using before = bridge::Function<&Element::before_t<Element, Node, bridge::String>>;

    JSValue childElementCount(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref().doc->childElementCount(ref())};
    }

    JSValue children(JSContext *ctx) const
    {
        if(auto it = ref().doc->child_e.find(ref().node); it != std::end(ref().doc->child_e))
            return JS_DupValue(ctx, it->second);
        return ref().doc->child_e[ref().node] = HTMLCollection::from(ctx, HTMLCollection::Wrapped{dom::Node{ref().doc, ref().node}});
    }

    JSValue firstElementChild(JSContext *) const
    {
        return ref().doc->firstElementChild(ref());
    }

    JSValue getAttribute(JSContext *ctx, bridge::String name)
    {
        if(auto value = ref().getAttribute({name}))
            return bridge::String{ctx, *value};
        return JS_NULL;
    }

    JSValue getAttributeNode(JSContext *ctx, bridge::String name)
    {
        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        return (-m).getNamedItem(m, ctx, name);
    }

    JSValue getElementsByTagName(JSContext *ctx, bridge::String name)
    {
        std::optional<std::string> error;
        return HTMLCollection::from(ctx, dom::HTMLCollection{
            dom::Node{ref().doc, ref().node},
            dom::Element::isTagName(name)
                ? ref().doc->querySelectorAll(ref(), name, error)
                : std::vector<void*>()
        });
    }

    JSValue hasAttribute(JSContext *ctx, bridge::String name)
    {
        return ref().hasAttribute({name}) ? JS_TRUE : JS_FALSE;
    }

    struct Position : bridge::String
    {
        enum class Value : std::uint8_t
        {
            afterbegin,
            afterend,
            beforebegin,
            beforeend,
            undefined,
        };

        using bridge::String::String;

        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            return true;
        }

        BOOST_FORCEINLINE operator Value () const
        {
            if(auto sv = static_cast<std::string_view>(*this); sv == "afterbegin")
                return Value::afterbegin;
            else if(sv == "afterend")
                return Value::afterend;
            else if(sv == "beforebegin")
                return Value::beforebegin;
            else if(sv == "beforeend")
                return Value::beforeend;
            return Value::undefined;
        }
    };

    template<Element::Position::Value pos>
    BOOST_FORCEINLINE JSValue insertAdjacentElement_t(JSContext *ctx, Element &n)
    {
        if constexpr (Element::Position::Value::afterbegin == pos)
        {
            if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
            if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);

            if(auto first = ref().doc->first(ref())) return ref().doc->insertBefore(ref(), n.ref(), *first);
            else return ref().doc->appendChild(ref(), n.ref());
        }
        else if constexpr (Element::Position::Value::afterend == pos)
        {
            if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
            if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);

            auto parent = ref().doc->parent(ref());
            if(!parent) return DOMException::throwHierarchyRequestError(ctx);

            if(auto next = ref().doc->next(ref())) return ref().doc->insertBefore(*parent, n.ref(), *next);
            else return ref().doc->appendChild(*parent, n.ref());
        }
        else if constexpr (Element::Position::Value::beforebegin == pos)
        {
            if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
            if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);

            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else return ref().doc->insertBefore(*parent, n.ref(), ref());
        }
        else if constexpr (Element::Position::Value::beforeend == pos)
        {
            if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
            if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
            return ref().doc->appendChild(ref(), n.ref());
        }
    }

    JSValue insertAdjacentElement(JSContext *ctx, Position pos, Element n)
    {
        switch(static_cast<Position::Value>(pos))
        {
        case Position::Value::afterbegin:
            return insertAdjacentElement_t<Position::Value::afterbegin>(ctx, n);
        case Position::Value::afterend:
            return insertAdjacentElement_t<Position::Value::afterend>(ctx, n);
        case Position::Value::beforebegin:
            return insertAdjacentElement_t<Position::Value::beforebegin>(ctx, n);
        case Position::Value::beforeend:
            return insertAdjacentElement_t<Position::Value::beforeend>(ctx, n);
        default:
            return JS_ThrowTypeError(ctx, "Wrong position argument [%s]", static_cast<std::string_view const &>(pos).data());
        }
    }

    JSValue insertAdjacentText(JSContext *ctx, Position pos, bridge::String s)
    {
        switch(static_cast<Position::Value>(pos))
        {
        case Position::Value::afterbegin:
            if(auto first = ref().doc->first(ref())) return ref().doc->insertBefore(ref(), ref().doc->createTextNode(s), *first);
            else return ref().doc->appendChild(ref(), ref().doc->createTextNode(s));
        case Position::Value::afterend:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else if(auto next = ref().doc->next(ref())) return ref().doc->insertBefore(*parent, ref().doc->createTextNode(s), *next);
            else return ref().doc->appendChild(*parent, ref().doc->createTextNode(s));
        case Position::Value::beforebegin:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else return ref().doc->insertBefore(*parent, ref().doc->createTextNode(s), ref());
        case Position::Value::beforeend:
            return ref().doc->appendChild(ref(), ref().doc->createTextNode(s));
        default:
            return JS_ThrowTypeError(ctx, "Wrong position argument [%s]", static_cast<std::string_view const &>(pos).data());
        }
    }

    JSValue lastElementChild(JSContext *) const
    {
        return ref().doc->lastElementChild(ref());
    }

    JSValue nextElementSibling(JSContext *) const
    {
        return ref().doc->nextElementSibling(ref());
    }

    template<typename Fallback, typename ...Ts>
    JSValue prepend_t(JSContext *ctx, bridge::Tail<1, Ts...> nodes)
    {
        auto first = ref().doc->first(ref());
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(ref().doc != n->ref().doc) return DOMException::throwWrongDocumentError(ctx);
                if(ref().doc->contains(n->ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
                if(first) ref().doc->insertBeforeVoid(ref(), n->ref(), *first);
                else ref().doc->appendChildVoid(ref(), n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                if(first) ref().doc->insertBeforeVoid(ref(), ref().doc->createTextNode(*s), *first);
                else ref().doc->appendChildVoid(ref(), ref().doc->createTextNode(*s));
            }
            else if(first)
            {
                if(auto e = Fallback::before_f(ctx, nodes, i, *first, ref())) return *e;
            }
            else
            {
                if(auto e = Fallback::append_f(ctx, nodes, i, ref())) return *e;
            }
        }
        return JS_UNDEFINED;
    }

    using prepend = bridge::Function<&Element::prepend_t<Element, Node, bridge::String>>;

    JSValue querySelector(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto node = ref().doc->querySelector(ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return node;
    }

    JSValue querySelectorAll(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto result = ref().doc->querySelectorAll(ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return NodeList::from(ctx, dom::NodeList{ref().doc->shared_from_this(), std::move(result)});
    }

    JSValue previousElementSibling(JSContext *) const
    {
        return ref().doc->previousElementSibling(ref());
    }

    JSValue remove(JSContext *ctx)
    {
        return ref().doc->removeVoid(ref()), JS_UNDEFINED;
    }

    JSValue removeAttribute(JSContext *ctx, bridge::String name)
    {
        auto const key = dom::Attr::Name{name};
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            NamedNodeMap::remove(it->second, key);
        return ref().removeAttribute(key), JS_UNDEFINED;
    }

    JSValue removeAttributeNode(JSContext *ctx, Attr attr)
    {
        if(ref().node != attr.ref().node) return DOMException::throwNotFoundError(ctx);

        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        bridge::Strong<bridge::String> n{ctx, attr.nodeName(ctx)};
        return (-m).removeNamedItem(m, ctx, n);
    }

    template<typename Fallback, typename ...Ts>
    JSValue replaceChildren_t(JSContext *ctx, bridge::Tail<1, Ts...> nodes)
    {
        ref().clear();
        return append_t<Fallback, Ts...>(ctx, nodes);
    }
    using replaceChildren = bridge::Function<&Element::replaceChildren_t<Element, Node, bridge::String>>;

    JSValue setAttribute(JSContext *ctx, bridge::String name, bridge::Value value)
    {
        return ref().setAttribute({name}, value.toString()), JS_UNDEFINED;
    }

    JSValue setAttributeNode(JSContext *ctx, bridge::Object attr)
    {
        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        return (-m).setNamedItem(m, ctx, attr);
    }

    JSValue toggleAttribute_0(JSContext *ctx, bridge::String name)
    {
        return ref().toggleAttribute({name}) ? JS_TRUE : JS_FALSE;
    }

    JSValue toggleAttribute_1(JSContext *ctx, bridge::String name, bridge::Boolean force)
    {
        return ref().toggleAttribute({name}, force) ? JS_TRUE : JS_FALSE;
    }

    using toggleAttribute = bridge::Function
    <
        &Element::toggleAttribute_0,
        &Element::toggleAttribute_1
    >;

    JSValue toJSON(JSContext *ctx) const
    {
        return XML{ctx, bridge::Strong<void>{ctx, XML::data(ctx, toString(ctx)), false}}.toJSON(ctx);
    }

    JSValue toString(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().toString()};
    }

    BOOST_FORCEINLINE static void free(dom::Element &self)
    {
        dynamic_cast<dom::XMLBackend *>(self.doc.get())->nodes.erase(self);
    }

    struct I : Base::I<I, XML::Interface>
    {
        using Base::Base;
        std::string get() const override
        {
            if(!dynamic_cast<dom::XMLBackend*>(ref.doc.get()))
                throw std::runtime_error("not an XML document");
            return ref.toString();
        }
    };

    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<Element>;
    friend class NamedNodeMap;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Element::funcs[] = {
    JS_CGETSET_DEF("attributes", &bridge::Getter<&Element::attributes>, NULL),
    JS_CGETSET_DEF("childElementCount", &bridge::Getter<&Element::childElementCount>, NULL),
    JS_CGETSET_DEF("children", &bridge::Getter<&Element::children>, NULL),
    JS_CGETSET_DEF("firstElementChild", &bridge::Getter<&Element::firstElementChild>, NULL),
    JS_CGETSET_DEF("innerText", &bridge::Getter<&Node::get_textContent>, &Node::set_textContent::invoke),
    JS_CGETSET_DEF("lastElementChild", &bridge::Getter<&Element::lastElementChild>, NULL),
    JS_CGETSET_DEF("nextElementSibling", &bridge::Getter<&Element::nextElementSibling>, NULL),
    JS_CGETSET_DEF("previousElementSibling", &bridge::Getter<&Element::previousElementSibling>, NULL),
    JS_CGETSET_DEF("tagName", &bridge::Getter<&Node::nodeName>, NULL),

    JS_CFUNC_DEF("after", 1, &Element::after::invoke),
    JS_CFUNC_DEF("append", 1, &Element::append::invoke),
    JS_CFUNC_DEF("before", 1, &Element::before::invoke),
    JS_CFUNC_DEF("getAttribute", 1, &bridge::Function<&Element::getAttribute>::invoke),
    JS_CFUNC_DEF("getAttributeNode", 1, &bridge::Function<&Element::getAttributeNode>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&Element::getElementsByTagName>::invoke),
    JS_CFUNC_DEF("hasAttribute", 1, &bridge::Function<&Element::hasAttribute>::invoke),
    JS_CFUNC_DEF("insertAdjacentElement", 2, &bridge::Function<&Element::insertAdjacentElement>::invoke),
    JS_CFUNC_DEF("insertAdjacentText", 2, &bridge::Function<&Element::insertAdjacentText>::invoke),
    JS_CFUNC_DEF("moveBefore", 2, &Node::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 0, &Element::prepend::invoke),
    JS_CFUNC_DEF("querySelector", 1, &bridge::Function<&Element::querySelector>::invoke),
    JS_CFUNC_DEF("querySelectorAll", 1, &bridge::Function<&Element::querySelectorAll>::invoke),
    JS_CFUNC_DEF("setAttribute", 2, &bridge::Function<&Element::setAttribute>::invoke),
    JS_CFUNC_DEF("setAttributeNode", 1, &bridge::Function<&Element::setAttributeNode>::invoke),
    JS_CFUNC_DEF("remove", 0, &bridge::Function<&Element::remove>::invoke),
    JS_CFUNC_DEF("removeAttribute", 1, &bridge::Function<&Element::removeAttribute>::invoke),
    JS_CFUNC_DEF("removeAttributeNode", 1, &bridge::Function<&Element::removeAttributeNode>::invoke),
    JS_CFUNC_DEF("replaceChildren", 1, &Element::replaceChildren::invoke),
    JS_CFUNC_DEF("toggleAttribute", 1, &Element::toggleAttribute::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&Element::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Element>::toJSON)
};
