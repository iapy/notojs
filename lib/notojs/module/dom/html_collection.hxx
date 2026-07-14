struct HTMLCollection : bridge::Interface<Collection<HTMLCollection, dom::HTMLCollection>, std::variant<dom::Node, dom::HTMLCollection>>
{
    HTMLCollection(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLCollection(std::reference_wrapper<std::variant<dom::Node, dom::HTMLCollection>> &&rw) : Base(std::move(rw)) {}

    BOOST_FORCEINLINE static JSValue collection(dom::Node const &node)
    {
        return node.doc->childElements(node);
    }

    BOOST_FORCEINLINE static JSValue collection_item(dom::Node const &node, std::int64_t i, JSValue d)
    {
        return node.doc->childElement(node, i, d);
    }

    BOOST_FORCEINLINE static std::uint64_t collection_size(dom::Node const &node)
    {
        return node.doc->childElementCount(node);
    }

    BOOST_FORCEINLINE static void free(std::variant<dom::Node, dom::HTMLCollection> &self)
    {
        std::visit(boost::hana::overload_linearly(
            [](dom::HTMLCollection &list) {
                list.free();
            },
            [](dom::Node &node) {
                node.doc->child_e.erase(node.node);
            }), self);
    }

    JSValue namedItem(JSContext *, bridge::String name)
    {
        return std::visit(boost::hana::overload_linearly(
            [&name](dom::Node const &node) -> JSValue {
                return node.doc->childElement(node, name);
            },
            [&name](dom::HTMLCollection const &list) -> JSValue {
                return JS_UNDEFINED;
            }), Base::ref());
    }

    using C = Collection<HTMLCollection, dom::HTMLCollection>;

    using ctor = bridge::Unconstructable<HTMLCollection>;
    using priv = bridge::Private
    <
        bridge::Iterator<KeysIterator>
    >;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods HTMLCollection::exoticMethods = {
    .delete_property = &bridge::del_property<HTMLCollection::C>,
    .get_property = &bridge::get_property<HTMLCollection::C>,
    .set_property = &bridge::set_property<HTMLCollection::C>
};

JSCFunctionListEntry const HTMLCollection::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&HTMLCollection::C::length>, NULL),

    JS_CFUNC_DEF("entries", 0, &bridge::Function<&HTMLCollection::C::entries>::invoke),
    JS_CFUNC_DEF("forEach", 1, &HTMLCollection::C::each::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&HTMLCollection::C::item>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&HTMLCollection::C::keys>::invoke),
    JS_CFUNC_DEF("namedItem", 1, &bridge::Function<&HTMLCollection::namedItem>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&HTMLCollection::C::values>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&HTMLCollection::C::values>::invoke)
};
