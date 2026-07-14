template<typename U>
void Node::register_upcast()
{
    // make Node methods inassessible in Attr
    if constexpr (!std::is_same_v<U, Attr::Base>)
        Base::template register_upcast<U>();
}

JSValue Node::compareDocumentPosition_0(JSContext *ctx, Attr a) const
{
    if(!ref().node || !a.ref().node) return bridge::Number{ctx, 1};
    if(ref().node == a.ref().node) return bridge::Number{ctx, 8 | 2}; // CONTAINS
    return ref().doc->compareDocumentPosition(ref(), a.ref());
}

JSValue Node::isEqualNode_1(JSContext *, Attr) const
{
    return JS_FALSE;
}

JSValue Node::isSameNode_1(JSContext *, Attr) const
{
    return JS_FALSE;
}

JSValue Node::contains_1(JSContext *, Attr a)
{
    return a.ref().node == ref().node || ref().doc->contains(ref(), a.ref()) ? JS_TRUE : JS_FALSE;
}

void Attr::free(dom::Attr &self)
{
    if(auto it = self.doc->attributes.find(self.node); it != std::end(self.doc->attributes))
        NamedNodeMap::get(it->second).attributes.erase(self.name);
}

JSValue Attr::cloneNode(JSContext *ctx) const
{
    std::optional<std::string> v;
    if(ref().value) v = ref().value;
    else if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
        v = NamedNodeMap::get(it->second).getAttribute(ref().name);
    return v ? Attr::from(ctx, dom::Attr{ref().doc->shared_from_this(), ref().name, std::move(*v)}) : JS_NULL;
}

JSValue Attr::isConnected(JSContext *ctx) const
{
    return ref().node ? JS_TRUE : JS_FALSE;
}

JSValue Attr::isEqualNode_1(JSContext *, Attr a) const
{
    std::optional<std::string> v1, v2;
    if(ref().value) v1 = ref().value;
    else if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
        v1 = NamedNodeMap::get(it->second).getAttribute(ref().name);
    if(a.ref().value) v2 = a.ref().value;
    else if(auto it = a.ref().doc->attributes.find(a.ref().node); it != std::end(a.ref().doc->attributes))
        v2 = NamedNodeMap::get(it->second).getAttribute(a.ref().name);
    return v1 == v2 ? JS_TRUE : JS_FALSE;
}

JSValue Attr::namespaceURI(JSContext *ctx) const
{
    if(auto *html = dynamic_cast<dom::HTMLBackend*>(ref().doc.get()); html)
        if(auto ns = html->lookupNS(ref().name.ns))
            return bridge::String(ctx, std::move(*ns));
    return JS_NULL;
}

JSValue Attr::nodeValue(JSContext *ctx) const
{
    if(ref().value)
        return bridge::String{ctx, *ref().value};
    if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
        if(auto value = NamedNodeMap::get(it->second).getAttribute(ref().name))
            return bridge::String{ctx, *value};
    return JS_NULL;
}

void Attr::set_nodeValue(JSContext *, bridge::String str)
{
    if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
        NamedNodeMap::get(it->second).setAttribute(ref().name, str);
    else ref().value = str;
}

JSValue Attr::ownerElement(JSContext *ctx) const
{
    if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
        return NamedNodeMap::get(it->second).ownerElement();
    return JS_NULL;
}
