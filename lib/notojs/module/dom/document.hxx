struct Document : bridge::Interface<Document, dom::Node, Node>
{
    Document(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Document(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

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

    static JSValue html_0(JSContext *);
    static JSValue html_1(JSContext *, bridge::String);

    using html = bridge::Function
    <
        &Document::html_0,
        &Document::html_1
    >;

    static JSValue xml(JSContext *, bridge::String);

    using ctor = bridge::Unconstructable<Document>;
    static JSCFunctionListEntry const sfunc[];
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Document::sfunc[] = {
    // Integration interface
    JS_CFUNC_DEF("html", 1, &Document::html::invoke),
    JS_CFUNC_DEF("xml", 1, &bridge::Function<&Document::xml>::invoke)
};

JSCFunctionListEntry const Document::funcs[] = {
    JS_CFUNC_DEF("querySelector", 1, &bridge::Function<&Document::querySelector>::invoke),
    JS_CFUNC_DEF("querySelectorAll", 1, &bridge::Function<&Document::querySelectorAll>::invoke)
};
