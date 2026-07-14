struct NamedNodeMap : bridge::Interface<NamedNodeMap, dom::NamedNodeMap>
{
    NamedNodeMap(JSContext *ctx, JSValue self) : Base{ctx, self} {}

    JSValue get_property(JSContext *ctx, const char *name) const
    {
        std::int64_t const i = u64(name);
        return item_(ctx, ref().doc->attributes[ref().node], const_cast<dom::NamedNodeMap &>(ref()), i, JS_UNDEFINED);
    }

    JSValue getNamedItem(JSValue self, JSContext *ctx, bridge::String name)
    {
        auto const key = dom::Attr::Name{name};
        if(auto it = ref().attributes.find(key); it != std::end(ref().attributes))
            return JS_DupValue(ctx, it->second);
        else if(ref().hasAttribute(key))
            return ref().attributes[key] = Attr::from(ctx, dom::Attr{ref().doc, ref().node, key}, self);
        return JS_NULL;
    }

    static JSValue item_(JSContext *ctx, JSValue self, dom::NamedNodeMap &e, std::int64_t i, JSValue def)
    {
        if(auto key = e.getAttributeName(i); key)
        {
            if(auto it = e.attributes.find(*key); it != std::end(e.attributes))
                return JS_DupValue(ctx, it->second);
            JSValue attr = Attr::from(ctx, dom::Attr{e.doc, e.node, *key}, self);
            return e.attributes[std::move(*key)] = attr;
        }
        return def;
    }

    JSValue item(JSValue self, JSContext *ctx, bridge::Number index)
    {
        std::int64_t const i = static_cast<std::int64_t>(index);
        return item_(ctx, self, ref(), i, JS_NULL);
    }

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref().attributesCount()};
    }

    JSValue removeNamedItem(JSValue self, JSContext *ctx, bridge::String name)
    {
        auto item = getNamedItem(self, ctx, name);
        if(!JS_IsNull(item))
        {
            auto &a = Attr::get(item);
            a.value = ref().getAttribute(a.name).value_or("");
            a.node = nullptr;

            ref().attributes.erase(a.name);
            ref().removeAttribute(a.name);
        }
        return item;
    }

    JSValue setNamedItem(JSValue self, JSContext *ctx, bridge::Object a)
    {
        if(!Attr::check(ctx, +a)) return JS_ThrowTypeError(ctx, "No matching function overload found");

        auto attr = Attr(ctx, a);
        if(attr.ref().node == ref().node) return JS_DupValue(ctx, a);
        if(attr.ref().doc != ref().doc) return DOMException::throwWrongDocumentError(ctx);

        JSValue prev = JS_NULL;
        auto const &key = attr.ref().name;
        if(auto it = ref().attributes.find(key); it != std::end(ref().attributes))
            prev = JS_DupValue(ctx, it->second);
        else if(ref().hasAttribute(attr.ref().name))
            prev = Attr::from(ctx, dom::Attr{ref().doc, ref().node, key}, self);
        if(!JS_IsNull(prev))
        {
            auto &a = Attr::get(prev);
            a.value = ref().getAttribute(a.name).value_or("");
            a.node = nullptr;

            ref().attributes.erase(a.name);
            ref().removeAttribute(a.name);
        }

        if(ref().attributes[key] = a; attr.ref().value)
        {
            ref().setAttribute(key, *attr.ref().value);
            attr.ref().value.reset();
        }
        else if(auto it = attr.ref().doc->attributes.find(attr.ref().node); it != std::end(attr.ref().doc->attributes))
        {
            ref().setAttribute(key, bridge::Strong<bridge::String>(ctx, attr.nodeValue(ctx)));

            auto &map = NamedNodeMap::get(it->second);
            map.attributes.erase(key);
            map.removeAttribute(key);
        }
        bridge::detail::set_allocator(ctx, a, self);
        attr.ref().node = ref().node;
        return prev;
    }

    JSValue values(JSValue self, JSContext *ctx)
    {
        bridge::Strong<bridge::Array> arr{ctx, bridge::Array{ctx}};
        for(auto const &key: ref().attrs())
        {
            if(auto it = ref().attributes.find(key); it != std::end(ref().attributes))
                arr.append(JS_DupValue(ctx, it->second));
            else
                arr.append(ref().attributes[key] = Attr::from(ctx, dom::Attr{ref().doc, ref().node, key}, self));
        }
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

    static void remove(JSValue &value, dom::Attr::Name const &key)
    {
        auto &nnm = NamedNodeMap::get(value);
        if(auto jt = nnm.attributes.find(key); jt != std::end(nnm.attributes))
        {
            auto &a = Attr::get(jt->second);
            a.value = nnm.getAttribute(key).value_or("");
            a.node = nullptr;
            nnm.attributes.erase(jt);
        }
    }

    BOOST_FORCEINLINE static void free(dom::NamedNodeMap &self)
    {
        self.doc->attributes.erase(self.node);
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<NamedNodeMap>;
    friend class Attr;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods NamedNodeMap::exoticMethods = {
    .get_property = &bridge::get_property<NamedNodeMap>
};

JSCFunctionListEntry const NamedNodeMap::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&NamedNodeMap::length>, NULL),

    JS_CFUNC_DEF("getNamedItem", 1, &bridge::Function<&NamedNodeMap::getNamedItem>::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&NamedNodeMap::item>::invoke),
    JS_CFUNC_DEF("setNamedItem", 1, &bridge::Function<&NamedNodeMap::setNamedItem>::invoke),
    JS_CFUNC_DEF("removeNamedItem", 1, &bridge::Function<&NamedNodeMap::removeNamedItem>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&NamedNodeMap::values>::invoke)
};

struct HTMLNamedNodeMap : bridge::Interface<HTMLNamedNodeMap, dom::HTMLNamedNodeMap, NamedNodeMap>
{
    HTMLNamedNodeMap(JSContext *ctx, JSValue self) : Base{ctx, self} {}

    JSValue get_property(JSContext *ctx, const char *name) const
    {
        std::int64_t const i = u64(name);
        return NamedNodeMap::item_(ctx, ref().doc->attributes[ref().node], const_cast<dom::HTMLNamedNodeMap &>(ref()), i, JS_UNDEFINED);
    }

    JSValue getNamedItemNS(JSValue self, JSContext *ctx, bridge::String ns, bridge::String name)
    {
        if(auto key = attr(name, ns); !key)
            return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + static_cast<std::string>(ns));
        else if(auto it = ref().attributes.find(*key); it != std::end(ref().attributes))
            return JS_DupValue(ctx, it->second);
        else if(ref().hasAttribute(*key))
            return ref().attributes[*key] = Attr::from(ctx, dom::Attr{ref().doc, ref().node, *key}, self);
        return JS_NULL;
    }

    JSValue removeNamedItemNS(JSValue self, JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto item = getNamedItemNS(self, ctx, ns, name);
        if(!JS_IsNull(item) && !JS_IsException(item))
        {
            auto &a = Attr::get(item);
            if(auto an = attr(name, ns))
            {
                a.value = ref().getAttribute(*an).value_or("");
                a.node = nullptr;

                ref().attributes.erase(*an);
                ref().removeAttribute(*an);
            }
        }
        return JS_IsNull(item) ? DOMException::throwNotFoundError(ctx) : item;
    }

    using ctor = bridge::Unconstructable<HTMLNamedNodeMap>;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;

private:
    std::optional<dom::Attr::Name> attr(std::string const &n, std::string_view const &ns) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        if(auto nsid = doc->lookupNS(n); LXB_NS__UNDEF != nsid)
            return dom::Attr::Name{n, nsid};
        return std::nullopt;
    }
};

JSClassExoticMethods HTMLNamedNodeMap::exoticMethods = {
    .get_property = &bridge::get_property<HTMLNamedNodeMap>
};

JSCFunctionListEntry const HTMLNamedNodeMap::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&NamedNodeMap::length>, NULL),

    JS_CFUNC_DEF("getNamedItem", 1, &bridge::Function<&NamedNodeMap::getNamedItem>::invoke),
    JS_CFUNC_DEF("getNamedItemNS", 1, &bridge::Function<&HTMLNamedNodeMap::getNamedItemNS>::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&NamedNodeMap::item>::invoke),
    JS_CFUNC_DEF("setNamedItem", 1, &bridge::Function<&NamedNodeMap::setNamedItem>::invoke),
    JS_CFUNC_DEF("setNamedItemNS", 1, &bridge::Function<&NamedNodeMap::setNamedItem>::invoke),
    JS_CFUNC_DEF("removeNamedItem", 1, &bridge::Function<&NamedNodeMap::removeNamedItem>::invoke),
    JS_CFUNC_DEF("removeNamedItemNS", 1, &bridge::Function<&HTMLNamedNodeMap::removeNamedItemNS>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&NamedNodeMap::values>::invoke)
};
