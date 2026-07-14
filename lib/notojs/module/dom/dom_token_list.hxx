struct DOMTokenList : bridge::Interface<DOMTokenList, dom::DOMTokenList>
{
    DOMTokenList(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    DOMTokenList(std::reference_wrapper<dom::DOMTokenList> &&rw) : Base(std::move(rw)) {}

    BOOST_FORCEINLINE static std::optional<std::string_view> validate_(std::string_view const &sv)
    {
        return sv.empty() || std::find_if(std::begin(sv), std::end(sv), dom::DOMTokenList::isspace) != std::end(sv)
            ? std::nullopt : std::optional<std::string_view>(sv);
    }

    BOOST_FORCEINLINE static std::size_t find_(std::string_view const &s, std::string_view const &v)
    {
        if(v.empty()) return false;

        std::size_t p = s.find(v);
        while(p != std::string::npos)
        {
            if((p == 0 || dom::DOMTokenList::isspace(s[p - 1])) && (s.size() == p + v.size() || dom::DOMTokenList::isspace(s[p + v.size()]))) break;
            p = s.find(v, p + v.size());
        }
        return p;
    }

    JSValue add(JSContext *ctx, bridge::Tail<1, bridge::Value> values)
    {
        std::string attr;
        if(auto a = ref().getAttribute(ref().attr)) attr.assign(std::begin(*a), std::end(*a));

        for(std::size_t i = 0; i < values.size(); ++i)
        {
            auto str = values[i].toString();
            auto const &sv = static_cast<std::string_view const &>(str);
            if(auto const s = validate_(sv); s)
            {
                if(std::string::npos == find_(attr, *s))
                {
                    if(!attr.empty()) attr.append(" ");
                    attr.append(*s);
                }
            }
            else if(sv.empty()) return DOMException::throwSyntaxError(ctx);
            else return DOMException::throwInvalidCharacterError(ctx);
        }

        ref().setAttribute(ref().attr, attr);
        return JS_UNDEFINED;
    }

    JSValue contains(JSContext *ctx, bridge::Value value) const
    {
        if(auto c = ref().getAttribute(ref().attr))
        {
            auto str = value.toString();
            if(auto const sv = validate_(str); sv && std::string::npos != find_(*c, *sv))
                return JS_TRUE;
        }
        return JS_FALSE;
    }

    JSValue each_1(JSValue self, JSContext *ctx, bridge::Lambda lambda)
    {
        bridge::Strong<bridge::Array> arr{ctx, ref().values()};
        for(std::uint32_t i = 0; i < arr.size(); ++i)
        {
            bridge::Strong<void> j{ctx, bridge::Number{ctx, i}, false};
            if(auto result = lambda(std::array<JSValue, 3>{arr[i], j, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    JSValue each_2(JSValue self, JSContext *ctx, bridge::Lambda lambda, bridge::Value value)
    {
        bridge::Strong<bridge::Array> arr{ctx, ref().values()};
        for(std::uint32_t i = 0; i < arr.size(); ++i)
        {
            bridge::Strong<void> j{ctx, bridge::Number{ctx, i}, false};
            if(auto result = lambda(value, std::array<JSValue, 3>{arr[i], j, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    using each = bridge::Function
    <
        &DOMTokenList::each_1,
        &DOMTokenList::each_2
    >;

    JSValue entries(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> arr{ctx, ref().values()};
        for(std::uint32_t i = 0; i < arr.size(); ++i)
        {
            bridge::Array sub{ctx};
            sub.set(0, bridge::Number{ctx, i});
            sub.set(1, arr[i].release());
            arr.set(i, sub);
        }
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

    JSValue item(JSContext *, bridge::Number index) const
    {
        std::int64_t const n = static_cast<std::int64_t>(index);
        return n < 0 ? JS_NULL : ref().item(n, JS_NULL);
    }

    JSValue keys(JSContext *ctx) const
    {
        auto const len = ref().length();
        return bridge::Iterator<KeysIterator>::make(ctx, JS_NULL, KeysIterator::B{0, len}, KeysIterator::B{len, len});
    }

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref().length()};
    }

    JSValue get_property(JSContext *ctx, const char *name) const
    {
        std::int64_t const n = u64(name);
        return n < 0 ? JS_UNDEFINED : ref().item(n, JS_UNDEFINED);
    }

    JSValue remove(JSContext *ctx, bridge::Tail<1, bridge::Value> values)
    {
        std::string attr;
        if(auto a = ref().getAttribute(ref().attr)) attr.assign(std::begin(*a), std::end(*a));
        else return JS_UNDEFINED;

        for(std::size_t i = 0; i < values.size(); ++i)
        {
            auto str = values[i].toString();
            auto const &sv = static_cast<std::string_view const &>(str);
            if(auto const s = validate_(sv); s)
            {
                if(auto p = find_(attr, *s); std::string::npos != p)
                {
                    std::size_t n = p + s->size();
                    while(n < attr.size() && dom::DOMTokenList::isspace(attr[n])) ++n;
                    if(n == attr.size())
                        while(p && dom::DOMTokenList::isspace(attr[p - 1])) --p;
                    attr.erase(p, n - p);
                }
            }
            else if(sv.empty()) return DOMException::throwSyntaxError(ctx);
            else return DOMException::throwInvalidCharacterError(ctx);
        }

        if(attr.empty())
        {
            if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
                NamedNodeMap::remove(it->second, ref().attr);
            ref().removeAttribute(ref().attr);
        }
        else ref().setAttribute(ref().attr, attr);
        return JS_UNDEFINED;
    }

    JSValue replace(JSContext *ctx, bridge::Value o, bridge::Value n)
    {
        std::string attr;
        if(auto a = ref().getAttribute(ref().attr)) attr.assign(std::begin(*a), std::end(*a));
        else return JS_FALSE;

        auto ns = n.toString();
        auto const &nv = static_cast<std::string_view const &>(ns);

        auto os = o.toString();
        auto const &ov = static_cast<std::string_view const &>(os);

        if(auto const nss = validate_(nv); nss)
        {
            if(auto const oss = validate_(ov); oss)
            {
                if(auto p = find_(attr, *oss); std::string::npos != p)
                {
                    attr.replace(p, oss->size(), *nss);
                    ref().setAttribute(ref().attr, attr);
                    return JS_TRUE;
                }
            }
            else if(ov.empty()) return DOMException::throwSyntaxError(ctx);
            else return DOMException::throwInvalidCharacterError(ctx);
        }
        else if(nv.empty()) return DOMException::throwSyntaxError(ctx);
        else return DOMException::throwInvalidCharacterError(ctx);

        return JS_FALSE;
    }

    JSValue toggle_0(JSContext *ctx, bridge::Value value)
    {
        auto str = value.toString();
        auto const &sv = static_cast<std::string_view const &>(str);
        if(auto const s = validate_(sv); s)
        {
            std::string attr;
            if(auto a = ref().getAttribute(ref().attr))
            {
                attr.assign(std::begin(*a), std::end(*a));
                if(auto p = find_(attr, *s); std::string::npos != p)
                {
                    std::size_t n = p + s->size();
                    while(n < attr.size() && dom::DOMTokenList::isspace(attr[n])) ++n;
                    if(n == attr.size())
                        while(p && dom::DOMTokenList::isspace(attr[p - 1])) --p;
                    attr.erase(p, n - p);
                }
                else
                {
                    if(!attr.empty()) attr.append(" ");
                    attr.append(std::begin(*s), std::end(*s));
                }
            }
            else attr.assign(std::begin(*s), std::end(*s));

            ref().setAttribute(ref().attr, attr);
            return JS_TRUE;
        }
        else if(sv.empty()) return DOMException::throwSyntaxError(ctx);
        else return DOMException::throwInvalidCharacterError(ctx);

        return JS_FALSE;
    }

    JSValue toggle_1(JSContext *ctx, bridge::Value value, bridge::Boolean force)
    {
        auto str = value.toString();
        auto const &sv = static_cast<std::string_view const &>(str);
        if(auto const s = validate_(sv); s)
        {
            std::string attr;
            if(auto a = ref().getAttribute(ref().attr))
            {
                attr.assign(std::begin(*a), std::end(*a));
                if(auto p = find_(attr, *s); std::string::npos != p)
                {
                    if(force) return JS_FALSE;
                    std::size_t n = p + s->size();
                    while(n < attr.size() && dom::DOMTokenList::isspace(attr[n])) ++n;
                    if(n == attr.size())
                        while(p && dom::DOMTokenList::isspace(attr[p - 1])) --p;
                    attr.erase(p, n - p);
                }
                else if(force)
                {
                    if(!attr.empty()) attr.append(" ");
                    attr.append(std::begin(*s), std::end(*s));
                }
                else return JS_FALSE;
            }
            else if(force) attr.assign(std::begin(*s), std::end(*s));
            else return JS_FALSE;

            if(attr.empty())
            {
                if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
                    NamedNodeMap::remove(it->second, ref().attr);
                ref().removeAttribute(ref().attr);
            }
            else ref().setAttribute(ref().attr, attr);

            return JS_TRUE;
        }
        else if(sv.empty()) return DOMException::throwSyntaxError(ctx);
        else return DOMException::throwInvalidCharacterError(ctx);

        return JS_FALSE;
    }

    using toggle = bridge::Function
    <
        &DOMTokenList::toggle_0,
        &DOMTokenList::toggle_1
    >;

    JSValue value(JSContext *ctx) const
    {
        if(auto c = ref().getAttribute(ref().attr))
            return bridge::String{ctx, *c};
        return bridge::String{ctx, std::string_view{""}};
    }

    void set_value(JSContext *ctx, bridge::Value value)
    {
        ref().setAttribute(ref().attr, value.toString());
    }

    JSValue values(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> arr{ctx, ref().values()};
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

    BOOST_FORCEINLINE static void free(dom::HTMLElement &self)
    {
        dynamic_cast<dom::HTMLBackend *>(self.doc.get())->classes.erase(self);
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<DOMTokenList>;
    using priv = bridge::Private
    <
        bridge::Iterator<KeysIterator>
    >;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods DOMTokenList::exoticMethods = {
    .get_property = &bridge::get_property<DOMTokenList>
};

JSCFunctionListEntry const DOMTokenList::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&DOMTokenList::length>, NULL),
    JS_CGETSET_DEF("value", &bridge::Getter<&DOMTokenList::value>, &bridge::Setter<&DOMTokenList::set_value>),

    JS_CFUNC_DEF("add", 1, &bridge::Function<&DOMTokenList::add>::invoke),
    JS_CFUNC_DEF("contains", 1, &bridge::Function<&DOMTokenList::contains>::invoke),
    JS_CFUNC_DEF("forEach", 1, &DOMTokenList::each::invoke),
    JS_CFUNC_DEF("entries", 1, &bridge::Function<&DOMTokenList::entries>::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&DOMTokenList::item>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&DOMTokenList::keys>::invoke),
    JS_CFUNC_DEF("remove", 1, &bridge::Function<&DOMTokenList::remove>::invoke),
    JS_CFUNC_DEF("replace", 2, &bridge::Function<&DOMTokenList::replace>::invoke),
    JS_CFUNC_DEF("toggle", 1, &DOMTokenList::toggle::invoke),
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&DOMTokenList::value>::invoke),
    JS_CFUNC_DEF("values", 1, &bridge::Function<&DOMTokenList::values>::invoke)
};
