struct Attr : bridge::Interface<Attr, dom::Attr, Node>
{
    Attr(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Attr(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue childNodes(JSContext *ctx) const
    {
        return NodeList::from(ctx, NodeList::Wrapped{dom::NodeList{ref().doc->shared_from_this(), std::vector<void*>{}}});
    }

    JSValue cloneNode(JSContext *ctx) const;

    JSValue compareDocumentPosition_0(JSContext *ctx, Attr a)
    {
        if(!ref().node || !a.ref().node) return bridge::Number{ctx, 1};
        if(ref().node == a.ref().node) return ref().name < a.ref().name
            ? bridge::Number{ctx, 2} // PRECEEDING
            : a.ref().name < ref().name
                ? bridge::Number{ctx, 4} // FOLLOWING
                : bridge::Number{ctx, 0};
        return ref().doc->compareDocumentPosition(ref(), a.ref());
    }

    JSValue compareDocumentPosition_1(JSContext *ctx, Node n)
    {
        if(!ref().node || !ref().node) return bridge::Number{ctx, 1};
        if(ref().node == n.ref().node) return bridge::Number{ctx, 16 | 4}; // CONTAINED_BY
        return ref().doc->compareDocumentPosition(ref(), n.ref());
    }

    using compareDocumentPosition = bridge::Function
    <
        &Attr::compareDocumentPosition_0,
        &Attr::compareDocumentPosition_1
    >;

    JSValue contains_0(JSContext *, bridge::Null)
    {
        return JS_FALSE;
    }

    JSValue contains_1(JSContext *, Node n)
    {
        return JS_FALSE;
    }

    JSValue contains_2(JSContext *, Attr a)
    {
        return JS_FALSE;
    }

    using contains = bridge::Function
    <
        &Attr::contains_0,
        &Attr::contains_1,
        &Attr::contains_2
    >;

    JSValue dispatchEvent(JSContext *, Event)
    {
        return JS_UNDEFINED;
    }

    JSValue getNull(JSContext *) const
    {
        return JS_NULL;
    }

    JSValue getRootNode(JSContext *) const
    {
        if(!ref().node) return JS_NULL;
        return ref().doc->getRootNode(ref());
    }

    JSValue hasChildNodes(JSContext *) const
    {
        return JS_FALSE;
    }

    JSValue isConnected(JSContext *ctx) const;
    JSValue isDefaultNamespace(JSContext *, bridge::String ns)
    {
        return ref().doc->isDefaultNamespace(ref(), ns);
    }

    JSValue isEqualNode_0(JSContext *, bridge::Null) const
    {
        return JS_FALSE;
    }

    JSValue isEqualNode_1(JSContext *, Attr) const;
    JSValue isEqualNode_2(JSContext *, Node n) const
    {
        return JS_FALSE;
    }

    using isEqualNode = bridge::Function
    <
        &Attr::isEqualNode_0,
        &Attr::isEqualNode_1,
        &Attr::isEqualNode_2
    >;

    JSValue isSameNode_0(JSContext *, bridge::Null) const
    {
        return JS_FALSE;
    }

    JSValue isSameNode_1(JSContext *, Attr n) const
    {
        return ref().node == n.ref().node && ref().name == n.ref().name ? JS_TRUE : JS_FALSE;
    }

    JSValue isSameNode_2(JSContext *, Node n) const
    {
        return JS_FALSE;
    }

    using isSameNode = bridge::Function
    <
        &Attr::isSameNode_0,
        &Attr::isSameNode_1,
        &Attr::isSameNode_2
    >;

    JSValue namespaceURI(JSContext *ctx) const;
    JSValue nodeName(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().name.name};
    }
    JSValue nodeType(JSContext *ctx) const
    {
        return bridge::Number{ctx, std::uint64_t{2}};
    }

    JSValue nodeValue(JSContext *) const;
    void set_nodeValue(JSContext *, bridge::String str);

    JSValue noop(JSContext *)
    {
        return JS_UNDEFINED;
    }

    JSValue ownerDocument(JSContext *) const
    {
        return ref().doc->ownerDocument();
    }

    JSValue ownerElement(JSContext *) const;

    JSValue throwHierarchyRequestError(JSContext *ctx)
    {
        return DOMException::throwHierarchyRequestError(ctx);
    }

    static void free(dom::Attr &self);

    using ctor = bridge::Unconstructable<Attr>;
    friend class Node;
    friend class Element;
    friend class HTMLElement;
    friend class NamedNodeMap;
    friend class HTMLNamedNodeMap;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Attr::funcs[] = {
    JS_CGETSET_DEF("childNodes", &bridge::Getter<&Attr::childNodes>, NULL),
    JS_CGETSET_DEF("firstChild", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("isConnected", &bridge::Getter<&Attr::isConnected>, NULL),
    JS_CGETSET_DEF("lastChild", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&Attr::namespaceURI>, NULL),
    JS_CGETSET_DEF("nextSibling", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("nodeName", &bridge::Getter<&Attr::nodeName>, NULL),
    JS_CGETSET_DEF("nodeType", &bridge::Getter<&Attr::nodeType>, NULL),
    JS_CGETSET_DEF("nodeValue", &bridge::Getter<&Attr::nodeValue>, &bridge::Setter<&Attr::set_nodeValue>),
    JS_CGETSET_DEF("ownerDocument", &bridge::Getter<&Attr::ownerDocument>, NULL),
    JS_CGETSET_DEF("ownerElement", &bridge::Getter<&Attr::ownerElement>, NULL),
    JS_CGETSET_DEF("parentElement", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("parentNode", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("previousSibling", &bridge::Getter<&Attr::getNull>, NULL),
    JS_CGETSET_DEF("textContent", &bridge::Getter<&Attr::nodeValue>, &bridge::Setter<&Attr::set_nodeValue>),
    JS_CGETSET_DEF("value", &bridge::Getter<&Attr::nodeValue>, &bridge::Setter<&Attr::set_nodeValue>),

    JS_CFUNC_DEF("appendChild", 1, &bridge::Function<&Attr::throwHierarchyRequestError>::invoke),
    JS_CFUNC_DEF("cloneNode", 1, &bridge::Function<&Attr::cloneNode>::invoke),
    JS_CFUNC_DEF("contains", 1, &Attr::contains::invoke),
    JS_CFUNC_DEF("compareDocumentPosition", 1, &Attr::compareDocumentPosition::invoke),
    JS_CFUNC_DEF("dispatchEvent", 1, &bridge::Function<&Attr::dispatchEvent>::invoke),
    JS_CFUNC_DEF("getRootNode", 0, &bridge::Function<&Attr::getRootNode>::invoke),
    JS_CFUNC_DEF("hasChildNodes", 0, &bridge::Function<&Attr::hasChildNodes>::invoke),
    JS_CFUNC_DEF("removeChild", 1, &bridge::Function<&Attr::throwHierarchyRequestError>::invoke),
    JS_CFUNC_DEF("insertBefore", 2, &bridge::Function<&Attr::throwHierarchyRequestError>::invoke),
    JS_CFUNC_DEF("isDefaultNamespace", 1, &bridge::Function<&Attr::isDefaultNamespace>::invoke),
    JS_CFUNC_DEF("isEqualNode", 1, &Attr::isEqualNode::invoke),
    JS_CFUNC_DEF("isSameNode", 1, &Attr::isSameNode::invoke),
    JS_CFUNC_DEF("lookupNamespaceURI", 1, &bridge::Function<&Node::lookupNamespaceURI>::invoke),
    JS_CFUNC_DEF("lookupPrefix", 1, &bridge::Function<&Node::lookupPrefix>::invoke),
    JS_CFUNC_DEF("normalize", 0, &bridge::Function<&Attr::noop>::invoke),
    JS_CFUNC_DEF("replaceChild", 1, &bridge::Function<&Attr::throwHierarchyRequestError>::invoke),

    // Integration interface
    JS_CGETSET_DEF("_document", &bridge::Getter<&Node::_document>, NULL)
};
