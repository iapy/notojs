struct NodeList : bridge::Interface<Collection<NodeList, dom::NodeList>, std::variant<dom::Node, dom::NodeList>>
{
    NodeList(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    NodeList(std::reference_wrapper<std::variant<dom::Node, dom::NodeList>> &&rw) : Base(std::move(rw)) {}

    BOOST_FORCEINLINE static JSValue collection(dom::Node const &node)
    {
        return node.doc->childNodes(node);
    }

    BOOST_FORCEINLINE static JSValue collection_item(dom::Node const &node, std::int64_t i, JSValue d)
    {
        return node.doc->childNode(node, i, d);
    }

    BOOST_FORCEINLINE static std::uint64_t collection_size(dom::Node const &node)
    {
        return node.doc->childNodeCount(node);
    }

    BOOST_FORCEINLINE static void free(std::variant<dom::Node, dom::NodeList> &self)
    {
        if(!self.index()) std::get<0>(self).doc->child_n.erase(std::get<0>(self).node);
    }

    using C = Collection<NodeList, dom::NodeList>;

    using ctor = bridge::Unconstructable<NodeList>;
    using priv = bridge::Private
    <
        bridge::Iterator<KeysIterator>
    >;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods NodeList::exoticMethods = {
    .delete_property = &bridge::del_property<NodeList::C>,
    .get_property = &bridge::get_property<NodeList::C>,
    .set_property = &bridge::set_property<NodeList::C>
};

JSCFunctionListEntry const NodeList::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&C::length>, NULL),

    JS_CFUNC_DEF("entries", 0, &bridge::Function<&NodeList::C::entries>::invoke),
    JS_CFUNC_DEF("forEach", 1, &NodeList::C::each::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&NodeList::C::item>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&NodeList::C::keys>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&NodeList::C::values>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&NodeList::C::values>::invoke)
};
