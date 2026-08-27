struct HTMLElement : bridge::Interface<HTMLElement, dom::HTMLElement, Element>
{
    HTMLElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using appendChild = bridge::Function
    <
        &Node::appendChild,
        &HTMLNodeMixin::appendChild_t<Image>,
        &HTMLNodeMixin::appendChild_t<SVG>
    >;

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

    JSValue dataset(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->datasets.find(ptr); it != std::end(doc->datasets))
            return JS_DupValue(ctx, it->second);
        return doc->datasets[ptr] = DOMStringMap::from(ctx, dom::DOMStringMap(ref()));
    }

    JSValue getAttributeNode(JSContext *ctx, Attr::Name name)
    {
        bridge::Strong<NamedNodeMap> a{ctx, attributes_(ctx)};
        return (-a).getNamedItem(a, ctx, name);
    }

    JSValue getAttributeNodeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        bridge::Strong<NamedNodeMap> a{ctx, attributes_(ctx)};
        return (-a).getNamedItemExact(a, ctx, name);
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

    JSValue form(JSContext *ctx) const
    {
        auto *doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto id = ref().getAttribute({"form"}))
        {
            auto *el = lxb_dom_element_by_id(lxb_dom_interface_element(doc->doc.get()), reinterpret_cast<lxb_char_t const *>(id->data()), id->size());
            if(el && LXB_TAG_FORM == lxb_dom_node_tag_id(lxb_dom_interface_node(el)))
                return doc->make(lxb_dom_interface_node(el));
            return JS_NULL;
        }

        if(auto *node = ref().closest(LXB_TAG_FORM); node)
            return doc->make(node);
        return JS_NULL;
    }

    struct attributes
    {
        static constexpr char abbr[] = "abbr";
        static constexpr char action[] = "action";
        static constexpr char alt[] = "alt";
        static constexpr char amplitude[] = "amplitude";
        static constexpr char attributeName[] = "attributeName";
        static constexpr char autocomplete[] = "autocomplete";
        static constexpr char azimuth[] = "azimuth";
        static constexpr char baseFrequency[] = "baseFrequency";
        static constexpr char begin[] = "begin";
        static constexpr char bias[] = "bias";
        static constexpr char by[] = "by";
        static constexpr char clipPathUnits[] = "clipPathUnits";
        static constexpr char command[] = "command";
        static constexpr char content[] = "content";
        static constexpr char diffuseConstant[] = "diffuseConstant";
        static constexpr char divisor[] = "divisor";
        static constexpr char dur[] = "dur";
        static constexpr char edgeMode[] = "edgeMode";
        static constexpr char elevation[] = "elevation";
        static constexpr char end[] = "end";
        static constexpr char enctype[] = "enctype";
        static constexpr char exponent[] = "exponent";
        static constexpr char fill[] = "fill";
        static constexpr char filterUnits[] = "filterUnits";
        static constexpr char from[] = "from";
        static constexpr char gradientUnits[] = "gradientUnits";
        static constexpr char headers[] = "headers";
        static constexpr char href[] = "href";
        static constexpr char hreflang[] = "hreflang";
        static constexpr char id[] = "id";
        static constexpr char in2[] = "in2";
        static constexpr char intercept[] = "intercept";
        static constexpr char k1[] = "k1";
        static constexpr char k2[] = "k2";
        static constexpr char k3[] = "k3";
        static constexpr char k4[] = "k4";
        static constexpr char kernelMatrix[] = "kernelMatrix";
        static constexpr char kernelUnitLength[] = "kernelUnitLength";
        static constexpr char label[] = "label";
        static constexpr char limitingConeAngle[] = "limitingConeAngle";
        static constexpr char markerUnits[] = "markerUnits";
        static constexpr char maskContentUnits[] = "maskContentUnits";
        static constexpr char maskUnits[] = "maskUnits";
        static constexpr char media[] = "media";
        static constexpr char method[] = "method";
        static constexpr char mode[] = "mode";
        static constexpr char name[] = "name";
        static constexpr char numOctaves[] = "numOctaves";
        static constexpr char offset[] = "offset";
        static constexpr char order[] = "order";
        static constexpr char orient[] = "orient";
        static constexpr char patternContentUnits[] = "patternContentUnits";
        static constexpr char patternUnits[] = "patternUnits";
        static constexpr char points[] = "points";
        static constexpr char pointsAtX[] = "pointsAtX";
        static constexpr char pointsAtY[] = "pointsAtY";
        static constexpr char pointsAtZ[] = "pointsAtZ";
        static constexpr char preserveAlpha[] = "preserveAlpha";
        static constexpr char preserveAspectRatio[] = "preserveAspectRatio";
        static constexpr char primitiveUnits[] = "primitiveUnits";
        static constexpr char radius[] = "radius";
        static constexpr char rel[] = "rel";
        static constexpr char repeatCount[] = "repeatCount";
        static constexpr char result[] = "result";
        static constexpr char scale[] = "scale";
        static constexpr char scope[] = "scope";
        static constexpr char seed[] = "seed";
        static constexpr char slope[] = "slope";
        static constexpr char spacing[] = "spacing";
        static constexpr char specularConstant[] = "specularConstant";
        static constexpr char specularExponent[] = "specularExponent";
        static constexpr char spreadMethod[] = "spreadMethod";
        static constexpr char src[] = "src";
        static constexpr char stdDeviation[] = "stdDeviation";
        static constexpr char stitchTiles[] = "stitchTiles";
        static constexpr char surfaceScale[] = "surfaceScale";
        static constexpr char tableValues[] = "tableValues";
        static constexpr char target[] = "target";
        static constexpr char targetX[] = "targetX";
        static constexpr char targetY[] = "targetY";
        static constexpr char to[] = "to";
        static constexpr char type[] = "type";
        static constexpr char value[] = "value";
        static constexpr char values[] = "values";
        static constexpr char x[] = "x";
        static constexpr char xChannelSelector[] = "xChannelSelector";
        static constexpr char y[] = "y";
        static constexpr char yChannelSelector[] = "yChannelSelector";
        static constexpr char z[] = "z";
    };

    template<char const *Name>
    JSValue attribute(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({Name}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    template<char const *Name>
    void set_attribute(JSContext *, bridge::Value value)
    {
        ref().setAttribute({Name}, value.toString());
    }

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
                if(auto *node = dom::lexbor::insertBefore(ref(), object, static_cast<lxb_dom_node_t *>(first->node), error))
                    return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
                return DOMException::throwSyntaxError(ctx, std::move(error));
            }

            if(auto *node = dom::lexbor::appendChild(ref(), object, error))
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
                if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), object, static_cast<lxb_dom_node_t *>(next->node), error))
                    return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
                return DOMException::throwSyntaxError(ctx, std::move(error));
            }
            if(auto *node = dom::lexbor::appendChild(static_cast<lxb_dom_node_t *>(parent->node), object, error))
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else if constexpr (Element::Position::Value::beforebegin == pos)
        {
            std::optional<std::string> error;

            auto parent = ref().doc->parent(ref());
            if(!parent)
                return DOMException::throwHierarchyRequestError(ctx);

            if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), object, ref(), error))
                return dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
            return DOMException::throwSyntaxError(ctx, std::move(error));
        }
        else if constexpr (Element::Position::Value::beforeend == pos)
        {
            std::optional<std::string> error;
            if(auto *node = dom::lexbor::appendChild(ref(), object, error))
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
                if(auto *node = dom::lexbor::insertBefore(ref(), text, static_cast<lxb_dom_node_t *>(first->node), error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::lexbor::appendChild(ref(), text, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::afterend:
            if(auto parent = ref().doc->parent(ref()); !parent)
                return DOMException::throwHierarchyRequestError(ctx);
            else if(auto next = ref().doc->next(ref()))
            {
                if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), text, static_cast<lxb_dom_node_t *>(next->node), error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::lexbor::appendChild(static_cast<lxb_dom_node_t *>(parent->node), text, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforebegin:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), text, ref(), error))
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforeend:
            if(auto *node = dom::lexbor::appendChild(ref(), text, error))
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
                if(auto *node = dom::lexbor::insertBefore(ref(), object, static_cast<lxb_dom_node_t *>(first->node), error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::lexbor::appendChild(ref(), object, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::afterend:
            if(auto parent = ref().doc->parent(ref()); !parent)
                return DOMException::throwHierarchyRequestError(ctx);
            else if(auto next = ref().doc->next(ref()))
            {
                if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), object, static_cast<lxb_dom_node_t *>(next->node), error))
                    return JS_UNDEFINED;
            }
            else
            {
                if(auto *node = dom::lexbor::appendChild(static_cast<lxb_dom_node_t *>(parent->node), object, error))
                    return JS_UNDEFINED;
            }
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforebegin:
            if(auto parent = ref().doc->parent(ref()); !parent) return DOMException::throwHierarchyRequestError(ctx);
            else if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(parent->node), object, ref(), error))
                return JS_UNDEFINED;
            return DOMException::throwSyntaxError(ctx, std::move(error));
        case Element::Position::Value::beforeend:
            if(auto *node = dom::lexbor::appendChild(ref(), object, error))
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

    using insertBefore = bridge::Function
    <
        &Node::insertBefore_0,
        &Node::insertBefore_1,
        &HTMLNodeMixin::insertBefore_t0<Image>,
        &HTMLNodeMixin::insertBefore_t0<SVG>,
        &HTMLNodeMixin::insertBefore_t1<Image>,
        &HTMLNodeMixin::insertBefore_t1<SVG>
    >;

    JSValue set_outerHTML_0(JSContext *ctx, HTML value)
    {
        if(lxb_dom_node_t *node = ref(); node->parent)
        {
            if(LXB_DOM_NODE_TYPE_DOCUMENT == node->parent->type)
                return DOMException::throwNoModificationAllowedError(ctx);
            ref().outerHTML(value);
        }
        return JS_UNDEFINED;
    }

    JSValue set_outerHTML_1(JSContext *ctx, bridge::Value value)
    {
        if(lxb_dom_node_t *node = ref(); node->parent)
        {
            if(LXB_DOM_NODE_TYPE_DOCUMENT == node->parent->type)
                return DOMException::throwNoModificationAllowedError(ctx);
            auto str = value.toString();
            ref().outerHTML(static_cast<std::string_view const &>(str));
        }
        return JS_UNDEFINED;
    }

    using set_outerHTML = bridge::Setters
    <
        &HTMLElement::set_outerHTML_0,
        &HTMLElement::set_outerHTML_1
    >;

    JSValue set_outerText(JSContext *ctx, bridge::Value value)
    {
        lxb_dom_node_t *node = ref();
        if(!node->parent) return DOMException::throwNoModificationAllowedError(ctx);
        if(LXB_DOM_NODE_TYPE_DOCUMENT == node->parent->type)
            return DOMException::throwHierarchyRequestError(ctx);

        auto str = value.toString();
        ref().outerText(static_cast<std::string_view const &>(str));
        return JS_UNDEFINED;
    }

    JSValue relList(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->rels.find(ptr); it != std::end(doc->rels))
            return JS_DupValue(ctx, it->second);
        return doc->rels[ptr] = DOMTokenList::from(ctx, dom::DOMTokenList{ref(), dom::Attr::Name{"rel"}});
    }

    JSValue removeAttributeNode(JSContext *ctx, Attr attr)
    {
        if(ref().node != attr.ref().node) return DOMException::throwNotFoundError(ctx);

        bridge::Strong<NamedNodeMap> m{ctx, attributes_(ctx)};
        bridge::Strong<Attr::Name> n{ctx, attr.nodeName(ctx)};
        return (-m).removeNamedItem(m, ctx, n);
    }

    JSValue removeAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        auto const key = dom::Attr::Name{name};
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            NamedNodeMap::remove(it->second, key);
        return ref().removeAttribute(key), JS_UNDEFINED;
    }

    JSValue removeAttributeNS_1(JSContext *ctx, bridge::String ns, Attr::Name name)
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = doc->lookupNS(nsv); LXB_NS__UNDEF != nsid)
        {
            auto key = dom::Attr::Name{name.get(ref()), nsid};
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

    using replaceChild = bridge::Function
    <
        &Node::replaceChild,
        &HTMLNodeMixin::replaceChild_t<HTML>,
        &HTMLNodeMixin::replaceChild_t<Image>,
        &HTMLNodeMixin::replaceChild_t<SVG>
    >;

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
    friend class HTMLSelectElement;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLElement::funcs[] = {
    REFLECTING_ATTRIBUTE(id),

    JS_CGETSET_DEF("attributes", &bridge::Getter<&HTMLElement::attributes>, NULL),
    JS_CGETSET_DEF("classList", &bridge::Getter<&HTMLElement::classList>, NULL),
    JS_CGETSET_DEF("className", &bridge::Getter<&HTMLElement::className>, &bridge::Setter<&HTMLElement::set_className>),
    JS_CGETSET_DEF("dataset", &bridge::Getter<&HTMLElement::dataset>, NULL),
    JS_CGETSET_DEF("innerHTML", &bridge::Getter<&HTMLElement::innerHTML>, &HTMLElement::set_innerHTML::invoke),
    JS_CGETSET_DEF("innerText", &bridge::Getter<&HTMLElement::innerText>, &HTMLElement::set_innerText::invoke),
    JS_CGETSET_DEF("outerHTML", &bridge::Getter<&HTMLElement::toString>, &HTMLElement::set_outerHTML::invoke),
    JS_CGETSET_DEF("outerText", &bridge::Getter<&HTMLElement::innerText>, &bridge::Setter<&HTMLElement::set_outerText>),
    JS_CGETSET_DEF("style", &bridge::Getter<&HTMLElement::style>, NULL),

    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("append", 1,  &bridge::Function<&NodeMixin::append_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("appendChild", 1, &HTMLElement::appendChild::invoke),
    JS_CFUNC_DEF("before", 1,  &bridge::Function<&NodeMixin::before_t<HTMLNodeMixin>>::invoke),
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
    JS_CFUNC_DEF("moveBefore", 2, &HTMLElement::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 1, &bridge::Function<&NodeMixin::prepend_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("removeAttributeNS", 2, &HTMLElement::removeAttributeNS::invoke),
    JS_CFUNC_DEF("removeAttributeNode", 1, &bridge::Function<&HTMLElement::removeAttributeNode>::invoke),
    JS_CFUNC_DEF("replaceChildren", 1, &bridge::Function<&NodeMixin::replaceChildren_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceChild", 2, &HTMLElement::replaceChild::invoke),
    JS_CFUNC_DEF("replaceWith", 1,  &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("setAttributeNS", 3, &HTMLElement::setAttributeNS::invoke),
    JS_CFUNC_DEF("setAttributeNode", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),
    JS_CFUNC_DEF("setAttributeNodeNS", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&HTMLElement::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<HTMLElement>::toJSON)
};

#define HTML_ELEMENT_STUB(...) BOOST_PP_OVERLOAD(HTML_ELEMENT_STUB_, __VA_ARGS__)(__VA_ARGS__)
#define HTML_ELEMENT_STUB_1(name) HTML_ELEMENT_STUB_2(name, HTMLElement)
#define HTML_ELEMENT_STUB_2(name, base) \
struct name : bridge::Interface<name, dom::HTMLElement, base> \
{ \
    name(JSContext *ctx, JSValue self) : Base{ctx, self} {} \
    name(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {} \
 \
    using Base::Base; \
    using ctor = bridge::Unconstructable<name>; \
};

HTML_ELEMENT_STUB(HTMLAreaElement)
HTML_ELEMENT_STUB(HTMLBodyElement)
HTML_ELEMENT_STUB(HTMLBRElement)
HTML_ELEMENT_STUB(HTMLCanvasElement)
HTML_ELEMENT_STUB(HTMLDetailsElement)
HTML_ELEMENT_STUB(HTMLDialogElement)
HTML_ELEMENT_STUB(HTMLDivElement)
HTML_ELEMENT_STUB(HTMLEmbedElement)
HTML_ELEMENT_STUB(HTMLHeadElement)
HTML_ELEMENT_STUB(HTMLHeadingElement)
HTML_ELEMENT_STUB(HTMLHRElement)
HTML_ELEMENT_STUB(HTMLHtmlElement)
HTML_ELEMENT_STUB(HTMLIFrameElement)
HTML_ELEMENT_STUB(HTMLLegendElement)
HTML_ELEMENT_STUB(HTMLMapElement)
HTML_ELEMENT_STUB(HTMLMediaElement)
HTML_ELEMENT_STUB(HTMLAudioElement, HTMLMediaElement)
HTML_ELEMENT_STUB(HTMLMenuElement)
HTML_ELEMENT_STUB(HTMLMeterElement)
HTML_ELEMENT_STUB(HTMLModElement)
HTML_ELEMENT_STUB(HTMLObjectElement)
HTML_ELEMENT_STUB(HTMLOutputElement)
HTML_ELEMENT_STUB(HTMLParagraphElement)
HTML_ELEMENT_STUB(HTMLParamElement)
HTML_ELEMENT_STUB(HTMLPictureElement)
HTML_ELEMENT_STUB(HTMLPreElement)
HTML_ELEMENT_STUB(HTMLProgressElement)
HTML_ELEMENT_STUB(HTMLQuoteElement)
HTML_ELEMENT_STUB(HTMLScriptElement)
HTML_ELEMENT_STUB(HTMLSourceElement)
HTML_ELEMENT_STUB(HTMLSpanElement)
HTML_ELEMENT_STUB(HTMLTableCaptionElement)
HTML_ELEMENT_STUB(HTMLTableColElement)
HTML_ELEMENT_STUB(HTMLTitleElement)
HTML_ELEMENT_STUB(HTMLTrackElement)
HTML_ELEMENT_STUB(HTMLUListElement)
HTML_ELEMENT_STUB(HTMLVideoElement, HTMLMediaElement)

#undef HTML_ELEMENT_STUB_2
#undef HTML_ELEMENT_STUB_1
#undef HTML_ELEMENT_STUB

#include <notojs/module/dom/html_element/html_anchor_element.hxx>
#include <notojs/module/dom/html_element/html_base_element.hxx>
#include <notojs/module/dom/html_element/html_button_element.hxx>
#include <notojs/module/dom/html_element/html_data_element.hxx>
#include <notojs/module/dom/html_element/html_label_element.hxx>
#include <notojs/module/dom/html_element/html_form_element.hxx>
#include <notojs/module/dom/html_element/html_field_set_element.hxx>
#include <notojs/module/dom/html_element/html_input_element.hxx>
#include <notojs/module/dom/html_element/html_image_element.hxx>
#include <notojs/module/dom/html_element/html_li_element.hxx>
#include <notojs/module/dom/html_element/html_link_element.hxx>
#include <notojs/module/dom/html_element/html_meta_element.hxx>
#include <notojs/module/dom/html_element/html_style_element.hxx>
#include <notojs/module/dom/html_element/html_time_element.hxx>
#include <notojs/module/dom/html_element/html_table_element.hxx>
#include <notojs/module/dom/html_element/html_table_cell_element.hxx>
#include <notojs/module/dom/html_element/html_table_row_element.hxx>
#include <notojs/module/dom/html_element/html_table_section_element.hxx>
#include <notojs/module/dom/html_element/html_data_list_element.hxx>
#include <notojs/module/dom/html_element/html_o_list_element.hxx>
#include <notojs/module/dom/html_element/html_opt_group_element.hxx>
#include <notojs/module/dom/html_element/html_option_element.hxx>
#include <notojs/module/dom/html_element/html_select_element.hxx>
#include <notojs/module/dom/html_element/html_text_area_element.hxx>
