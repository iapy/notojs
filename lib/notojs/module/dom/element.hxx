struct Element : bridge::Interface<Element, dom::Element, Node>
{
    Element(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Element(std::reference_wrapper<dom::Element> &&rw) : Base(std::move(rw)) {}

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

    JSValue closest(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto node = ref().doc->closest(ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return node;
    }

    JSValue getAttribute(JSContext *ctx, Attr::Name name)
    {
        if(auto value = ref().getAttribute({name.get(ref())}))
            return bridge::String{ctx, *value};
        return JS_NULL;
    }

    JSValue getAttributeNode(JSContext *ctx, Attr::Name name)
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

    JSValue hasAttribute(JSContext *ctx, Attr::Name name)
    {
        return ref().hasAttribute({name.get(ref())}) ? JS_TRUE : JS_FALSE;
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

    JSValue matches(JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto m = ref().doc->matches(ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return m ? JS_TRUE : JS_FALSE;
    }

    JSValue nextElementSibling(JSContext *) const
    {
        return ref().doc->nextElementSibling(ref());
    }

    JSValue previousElementSibling(JSContext *) const
    {
        return ref().doc->previousElementSibling(ref());
    }

    JSValue removeAttribute(JSContext *ctx, Attr::Name name)
    {
        auto const key = dom::Attr::Name{name.get(ref())};
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            NamedNodeMap::remove(it->second, key);
        return ref().removeAttribute(key), JS_UNDEFINED;
    }

    JSValue removeAttributeNode(JSContext *ctx, Attr attr)
    {
        if(ref().node != attr.ref().node) return DOMException::throwNotFoundError(ctx);

        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        bridge::Strong<Attr::Name> n{ctx, attr.nodeName(ctx)};
        return (-m).removeNamedItem(m, ctx, n);
    }

    JSValue setAttribute(JSContext *ctx, Attr::Name name, bridge::Value value)
    {
        return ref().setAttribute({name.get(ref())}, value.toString()), JS_UNDEFINED;
    }

    JSValue setAttributeNode(JSContext *ctx, bridge::Object attr)
    {
        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        return (-m).setNamedItem(m, ctx, attr);
    }

    JSValue toggleAttribute_0(JSContext *ctx, Attr::Name name)
    {
        return ref().toggleAttribute({name.get(ref())}) ? JS_TRUE : JS_FALSE;
    }

    JSValue toggleAttribute_1(JSContext *ctx, Attr::Name name, bridge::Boolean force)
    {
        return ref().toggleAttribute({name.get(ref())}, force) ? JS_TRUE : JS_FALSE;
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
    JS_CGETSET_DEF("childElementCount", &bridge::Getter<&NodeMixin::childElementCount>, NULL),
    JS_CGETSET_DEF("children", &bridge::Getter<&NodeMixin::children>, NULL),
    JS_CGETSET_DEF("firstElementChild", &bridge::Getter<&NodeMixin::firstElementChild>, NULL),
    JS_CGETSET_DEF("lastElementChild", &bridge::Getter<&NodeMixin::lastElementChild>, NULL),
    JS_CGETSET_DEF("nextElementSibling", &bridge::Getter<&Element::nextElementSibling>, NULL),
    JS_CGETSET_DEF("previousElementSibling", &bridge::Getter<&Element::previousElementSibling>, NULL),
    JS_CGETSET_DEF("tagName", &bridge::Getter<&Node::nodeName>, NULL),

    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<>>::invoke),
    JS_CFUNC_DEF("append", 1, &bridge::Function<&NodeMixin::append_t<>>::invoke),
    JS_CFUNC_DEF("before", 1, &bridge::Function<&NodeMixin::before_t<>>::invoke),
    JS_CFUNC_DEF("closest", 1, &bridge::Function<&Element::closest>::invoke),
    JS_CFUNC_DEF("getAttribute", 1, &bridge::Function<&Element::getAttribute>::invoke),
    JS_CFUNC_DEF("getAttributeNode", 1, &bridge::Function<&Element::getAttributeNode>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&Element::getElementsByTagName>::invoke),
    JS_CFUNC_DEF("hasAttribute", 1, &bridge::Function<&Element::hasAttribute>::invoke),
    JS_CFUNC_DEF("insertAdjacentElement", 2, &bridge::Function<&Element::insertAdjacentElement>::invoke),
    JS_CFUNC_DEF("insertAdjacentText", 2, &bridge::Function<&Element::insertAdjacentText>::invoke),
    JS_CFUNC_DEF("matches", 1, &bridge::Function<&Element::matches>::invoke),
    JS_CFUNC_DEF("moveBefore", 2, &Node::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 0, &bridge::Function<&NodeMixin::prepend_t<>>::invoke),
    JS_CFUNC_DEF("querySelector", 1, &bridge::Function<&NodeMixin::querySelector>::invoke),
    JS_CFUNC_DEF("querySelectorAll", 1, &bridge::Function<&NodeMixin::querySelectorAll>::invoke),
    JS_CFUNC_DEF("setAttribute", 2, &bridge::Function<&Element::setAttribute>::invoke),
    JS_CFUNC_DEF("setAttributeNode", 1, &bridge::Function<&Element::setAttributeNode>::invoke),
    JS_CFUNC_DEF("remove", 0, &bridge::Function<&NodeMixin::remove>::invoke),
    JS_CFUNC_DEF("removeAttribute", 1, &bridge::Function<&Element::removeAttribute>::invoke),
    JS_CFUNC_DEF("removeAttributeNode", 1, &bridge::Function<&Element::removeAttributeNode>::invoke),
    JS_CFUNC_DEF("replaceChildren", 1,  &bridge::Function<&NodeMixin::replaceChildren_t<>>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<>>::invoke),
    JS_CFUNC_DEF("toggleAttribute", 1, &Element::toggleAttribute::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&Element::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Element>::toJSON)
};
