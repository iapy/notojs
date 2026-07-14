struct Attr;
struct Node : bridge::Interface<Node, dom::Node>
{
    Node(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Node(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue _document(JSContext *ctx) const
    {
        if(ref().doc->self) return JS_DupValue(ctx, *ref().doc->self);
        return JS_NULL;
    }

    JSValue appendChild(JSContext *ctx, Node n)
    {
        if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
        return ref().doc->appendChild(ref(), n.ref());
    }

    JSValue baseURI(JSContext *ctx) const
    {
        static constexpr std::string_view blank{"about:blank"};
        return bridge::String{ctx, blank};
    }

    JSValue childNodes(JSContext *ctx) const
    {
        if(auto it = ref().doc->child_n.find(ref().node); it != std::end(ref().doc->child_n))
            return JS_DupValue(ctx, it->second);
        return ref().doc->child_n[ref().node] = NodeList::from(ctx, NodeList::Wrapped{dom::Node{ref().doc, ref().node}});
    }

    JSValue cloneNode_0(JSContext *) const
    {
        return ref().doc->cloneNode(ref(), false);
    }

    JSValue cloneNode_1(JSContext *, bridge::Boolean deep) const
    {
        return ref().doc->cloneNode(ref(), deep);
    }

    using cloneNode = bridge::Function
    <
        &Node::cloneNode_0,
        &Node::cloneNode_1
    >;

    JSValue compareDocumentPosition_0(JSContext *, Attr) const;
    JSValue compareDocumentPosition_1(JSContext *, Node n) const
    {
        return ref().doc->compareDocumentPosition(ref(), n.ref());
    }

    using compareDocumentPosition = bridge::Function
    <
        &Node::compareDocumentPosition_0,
        &Node::compareDocumentPosition_1
    >;

    JSValue contains_0(JSContext *, bridge::Null)
    {
        return JS_FALSE;
    }

    JSValue contains_1(JSContext *, Attr);
    JSValue contains_2(JSContext *, Node n)
    {
        return ref().doc->contains(ref(), n.ref()) ? JS_TRUE : JS_FALSE;
    }

    using contains = bridge::Function
    <
        &Node::contains_0,
        &Node::contains_1,
        &Node::contains_2
    >;

    JSValue dispatchEvent(JSContext *, Event)
    {
        return JS_UNDEFINED;
    }

    JSValue firstChild(JSContext *) const
    {
        return ref().doc->firstChild(ref());
    }

    JSValue getRootNode(JSContext *) const
    {
        return ref().doc->getRootNode(ref());
    }

    JSValue hasChildNodes(JSContext *) const
    {
        return ref().doc->hasChildNodes(ref());
    }

    JSValue isConnected(JSContext *) const
    {
        return ref().doc->isConnected(ref());
    }

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
        return ref().doc->isEqualNode(ref(), n.ref());
    }

    using isEqualNode = bridge::Function
    <
        &Node::isEqualNode_0,
        &Node::isEqualNode_1,
        &Node::isEqualNode_2
    >;

    JSValue insertBefore_0(JSContext *ctx, Node n, bridge::Null)
    {
        return appendChild(ctx, std::move(n));
    }

    JSValue insertBefore_1(JSContext *ctx, Node n, Node r)
    {
        if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(ref().doc != r.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
        if(!ref().doc->isChild(ref(), r.ref())) return DOMException::throwNotFoundError(ctx);
        return ref().doc->insertBefore(ref(), n.ref(), r.ref());
    }

    using insertBefore = bridge::Function
    <
        &Node::insertBefore_0,
        &Node::insertBefore_1
    >;

    JSValue isSameNode_0(JSContext *, bridge::Null) const
    {
        return JS_FALSE;
    }

    JSValue isSameNode_1(JSContext *, Attr) const;
    JSValue isSameNode_2(JSContext *, Node n) const
    {
        return ref().node == n.ref().node ? JS_TRUE : JS_FALSE;
    }

    using isSameNode = bridge::Function
    <
        &Node::isSameNode_0,
        &Node::isSameNode_1,
        &Node::isSameNode_2
    >;

    JSValue lastChild(JSContext *) const
    {
        return ref().doc->lastChild(ref());
    }

    JSValue lookupPrefix(JSContext *, bridge::String)
    {
        return JS_NULL;
    }

    JSValue lookupNamespaceURI(JSContext *, bridge::String)
    {
        return JS_NULL;
    }

    JSValue namespaceURI(JSContext *) const
    {
        return ref().doc->namespaceURI(ref());
    }

    JSValue nextSibling(JSContext *) const
    {
        return ref().doc->nextSibling(ref());
    }

    JSValue nodeName(JSContext *) const
    {
        return ref().doc->nodeName(ref());
    }

    JSValue nodeType(JSContext *) const
    {
        return ref().doc->nodeType(ref());
    }

    JSValue normalize(JSContext *)
    {
        return ref().doc->normalize(ref()), JS_UNDEFINED;
    }

    JSValue nodeValue(JSContext *) const
    {
        return ref().doc->nodeValue(ref());
    }

    void set_nodeValue(JSContext *, bridge::String str)
    {
        ref().doc->nodeValue(ref(), str);
    }

    JSValue ownerDocument(JSContext *) const
    {
        return ref().doc->ownerDocument();
    }

    JSValue parentElement(JSContext *) const
    {
        return ref().doc->parentElement(ref());
    }

    JSValue parentNode(JSContext *) const
    {
        return ref().doc->parentNode(ref());
    }

    JSValue previousSibling(JSContext *) const
    {
        return ref().doc->previousSibling(ref());
    }

    JSValue removeChild(JSContext *ctx, Node n)
    {
        if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!ref().doc->isChild(ref(), n.ref())) return DOMException::throwNotFoundError(ctx);
        return ref().doc->remove(n.ref());
    }

    JSValue replaceChild(JSContext *ctx, Node n, Node o)
    {
        if(ref().doc != n.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(ref().doc != o.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!ref().doc->isChild(ref(), o.ref())) return DOMException::throwNotFoundError(ctx);
        if(ref().doc->contains(n.ref(), ref())) return DOMException::throwHierarchyRequestError(ctx);
        ref().doc->insertBeforeVoid(ref(), n.ref(), o.ref());
        return ref().doc->remove(o.ref());
    }

    JSValue get_textContent(JSContext *ctx) const
    {
        return ref().doc->textContent(ref());
    }

    void set_textContent_0(JSContext *ctx, bridge::Null)
    {
        ref().doc->textContent(ref(), std::nullopt);
    }

    void set_textContent_1(JSContext *ctx, bridge::Value value)
    {
        ref().doc->textContent(ref(), value.toString());
    }

    using set_textContent = bridge::Setters
    <
        &Node::set_textContent_0,
        &Node::set_textContent_1
    >;

    BOOST_FORCEINLINE static void free(dom::Node &self)
    {
        self.doc->free(self);
    }

    template<typename U>
    static void register_upcast();

    using ctor = bridge::Unconstructable<Node>;
    static JSCFunctionListEntry const funcs[];

    friend class Attr;
    friend class Element;
    friend class HTMLElement;
};

JSCFunctionListEntry const Node::funcs[] = {
    // Documented interface
    JS_CGETSET_DEF("baseURI", &bridge::Getter<&Node::baseURI>, NULL),
    JS_CGETSET_DEF("childNodes", &bridge::Getter<&Node::childNodes>, NULL),
    JS_CGETSET_DEF("firstChild", &bridge::Getter<&Node::firstChild>, NULL),
    JS_CGETSET_DEF("isConnected", &bridge::Getter<&Node::isConnected>, NULL),
    JS_CGETSET_DEF("lastChild", &bridge::Getter<&Node::lastChild>, NULL),
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&Node::namespaceURI>, NULL),
    JS_CGETSET_DEF("nextSibling", &bridge::Getter<&Node::nextSibling>, NULL),
    JS_CGETSET_DEF("nodeName", &bridge::Getter<&Node::nodeName>, NULL),
    JS_CGETSET_DEF("nodeType", &bridge::Getter<&Node::nodeType>, NULL),
    JS_CGETSET_DEF("nodeValue", &bridge::Getter<&Node::nodeValue>, &bridge::Setter<&Node::set_nodeValue>),
    JS_CGETSET_DEF("ownerDocument", &bridge::Getter<&Node::ownerDocument>, NULL),
    JS_CGETSET_DEF("parentElement", &bridge::Getter<&Node::parentElement>, NULL),
    JS_CGETSET_DEF("parentNode", &bridge::Getter<&Node::parentNode>, NULL),
    JS_CGETSET_DEF("previousSibling", &bridge::Getter<&Node::previousSibling>, NULL),
    JS_CGETSET_DEF("textContent", &bridge::Getter<&Node::get_textContent>, &Node::set_textContent::invoke),

    JS_CFUNC_DEF("appendChild", 1, &bridge::Function<&Node::appendChild>::invoke),
    JS_CFUNC_DEF("cloneNode", 1, &Node::cloneNode::invoke),
    JS_CFUNC_DEF("compareDocumentPosition", 1, &Node::compareDocumentPosition::invoke),
    JS_CFUNC_DEF("contains", 1, &Node::contains::invoke),
    JS_CFUNC_DEF("dispatchEvent", 1, &bridge::Function<&Node::dispatchEvent>::invoke),
    JS_CFUNC_DEF("getRootNode", 0, &bridge::Function<&Node::getRootNode>::invoke),
    JS_CFUNC_DEF("hasChildNodes", 0, &bridge::Function<&Node::hasChildNodes>::invoke),
    JS_CFUNC_DEF("isDefaultNamespace", 1, &bridge::Function<&Node::isDefaultNamespace>::invoke),
    JS_CFUNC_DEF("isEqualNode", 1, &Node::isEqualNode::invoke),
    JS_CFUNC_DEF("insertBefore", 2, &Node::insertBefore::invoke),
    JS_CFUNC_DEF("isSameNode", 1, &Node::isSameNode::invoke),
    JS_CFUNC_DEF("lookupNamespaceURI", 1, &bridge::Function<&Node::lookupNamespaceURI>::invoke),
    JS_CFUNC_DEF("lookupPrefix", 1, &bridge::Function<&Node::lookupPrefix>::invoke),
    JS_CFUNC_DEF("normalize", 0, &bridge::Function<&Node::normalize>::invoke),
    JS_CFUNC_DEF("removeChild", 1, &bridge::Function<&Node::removeChild>::invoke),
    JS_CFUNC_DEF("replaceChild", 2, &bridge::Function<&Node::replaceChild>::invoke),

    // Integration interface
    JS_CGETSET_DEF("_document", &bridge::Getter<&Node::_document>, NULL)
};
