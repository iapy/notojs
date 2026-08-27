struct DocumentType : bridge::Interface<DocumentType, dom::Node, Node>
{
    DocumentType(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    DocumentType(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue get_name(JSContext *ctx) const
    {
        return ref().doc->documentTypeName(ref());
    }

    JSValue namespaceURI(JSContext *) const
    {
        return JS_NULL;
    }

    JSValue publicId(JSContext *ctx) const
    {
        return ref().doc->documentTypePublicId(ref());
    }

    JSValue systemId(JSContext *ctx) const
    {
        return ref().doc->documentTypeSystemId(ref());
    }

    using ctor = bridge::Unconstructable<DocumentType>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const DocumentType::funcs[] = {
    JS_CGETSET_DEF("name", &bridge::Getter<&DocumentType::get_name>, NULL),
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&DocumentType::namespaceURI>, NULL),
    JS_CGETSET_DEF("publicId", &bridge::Getter<&DocumentType::publicId>, NULL),
    JS_CGETSET_DEF("systemId", &bridge::Getter<&DocumentType::systemId>, NULL),

    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<>>::invoke),
    JS_CFUNC_DEF("before", 1, &bridge::Function<&NodeMixin::before_t<>>::invoke),
    JS_CFUNC_DEF("remove", 0, &bridge::Function<&NodeMixin::remove>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<>>::invoke),

    JS_CFUNC_DEF("appendChild", 1, &Node::throwHierarchyRequestError),
    JS_CFUNC_DEF("insertBefore", 2, &Node::throwHierarchyRequestError),
    JS_CFUNC_DEF("replaceChild", 2, &Node::throwHierarchyRequestError)
};

struct HTMLDocumentType : bridge::Interface<HTMLDocumentType, dom::Node, DocumentType>
{
    HTMLDocumentType(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLDocumentType(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<HTMLDocumentType>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLDocumentType::funcs[] = {
    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("before", 1, &bridge::Function<&NodeMixin::before_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke)
};
