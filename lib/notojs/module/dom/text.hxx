struct Text : bridge::Interface<Text, dom::Node, CharacterData>
{
    Text(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Text(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue namespaceURI(JSContext *) const
    {
        return JS_NULL;
    }

    using ctor = bridge::Unconstructable<Text>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Text::funcs[] = {
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&Text::namespaceURI>, NULL),
};

struct HTMLText : bridge::Interface<HTMLText, dom::Node, Text>
{
    HTMLText(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLText(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<HTMLText>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLText::funcs[] = {
    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("append", 1, &bridge::Function<&NodeMixin::append_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke)
};
