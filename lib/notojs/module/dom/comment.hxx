struct Comment : bridge::Interface<Comment, dom::Node, CharacterData>
{
    Comment(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Comment(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue namespaceURI(JSContext *) const
    {
        return JS_NULL;
    }

    using ctor = bridge::Unconstructable<Comment>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Comment::funcs[] = {
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&Comment::namespaceURI>, NULL)
};

struct HTMLComment : bridge::Interface<HTMLComment, dom::Node, Comment>
{
    HTMLComment(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLComment(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<HTMLComment>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLComment::funcs[] = {
    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("before", 1, &bridge::Function<&NodeMixin::before_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke)
};
