struct ProcessingInstruction : bridge::Interface<ProcessingInstruction, dom::Node, CharacterData>
{
    ProcessingInstruction(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    ProcessingInstruction(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue sheet(JSContext *) const
    {
        return JS_NULL;
    }

    JSValue target(JSContext *) const
    {
        return ref().doc->nodeName(ref());
    }

    using ctor = bridge::Unconstructable<ProcessingInstruction>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const ProcessingInstruction::funcs[] = {
    JS_CGETSET_DEF("sheet", &bridge::Getter<&ProcessingInstruction::sheet>, NULL),
    JS_CGETSET_DEF("target", &bridge::Getter<&ProcessingInstruction::target>, NULL)
};

struct HTMLProcessingInstruction : bridge::Interface<HTMLProcessingInstruction, dom::Node, ProcessingInstruction>
{
    HTMLProcessingInstruction(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLProcessingInstruction(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<HTMLProcessingInstruction>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLProcessingInstruction::funcs[] = {
    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("append", 1, &bridge::Function<&NodeMixin::append_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke)
};
