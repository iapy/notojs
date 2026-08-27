struct DocumentFragment : bridge::Interface<DocumentFragment, dom::DocumentFragment, Node>
{
    DocumentFragment(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    DocumentFragment(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using appendChild = bridge::Function
    <
        &Node::appendChild,
        &HTMLNodeMixin::appendChild_t<Image>,
        &HTMLNodeMixin::appendChild_t<SVG>
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

    JSValue namespaceURI(JSContext *) const
    {
        return JS_NULL;
    }

    using replaceChild = bridge::Function
    <
        &Node::replaceChild,
        &HTMLNodeMixin::replaceChild_t<HTML>,
        &HTMLNodeMixin::replaceChild_t<Image>,
        &HTMLNodeMixin::replaceChild_t<SVG>
    >;

    using ctor = bridge::Unconstructable<DocumentFragment>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const DocumentFragment::funcs[] = {
    JS_CGETSET_DEF("childElementCount", &bridge::Getter<&NodeMixin::childElementCount>, NULL),
    JS_CGETSET_DEF("children", &bridge::Getter<&NodeMixin::children>, NULL),
    JS_CGETSET_DEF("firstElementChild", &bridge::Getter<&NodeMixin::firstElementChild>, NULL),
    JS_CGETSET_DEF("lastElementChild", &bridge::Getter<&NodeMixin::lastElementChild>, NULL),
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&DocumentFragment::namespaceURI>, NULL),

    JS_CFUNC_DEF("append", 1, &bridge::Function<&NodeMixin::append_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("appendChild", 1, &DocumentFragment::appendChild::invoke),
    JS_CFUNC_DEF("insertBefore", 2, &DocumentFragment::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 0, &bridge::Function<&NodeMixin::prepend_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("querySelector", 1, &bridge::Function<&NodeMixin::querySelector>::invoke),
    JS_CFUNC_DEF("querySelectorAll", 1, &bridge::Function<&NodeMixin::querySelectorAll>::invoke),
    JS_CFUNC_DEF("replaceChild", 2, &DocumentFragment::replaceChild::invoke),
    JS_CFUNC_DEF("replaceChildren", 1, &bridge::Function<&NodeMixin::replaceChildren_t<HTMLNodeMixin>>::invoke)
};
