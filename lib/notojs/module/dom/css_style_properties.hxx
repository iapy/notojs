struct CSSStyleProperties : bridge::Interface<CSSStyleProperties, dom::CSSStyleProperties, CSSStyleDeclaration>
{
    CSSStyleProperties(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSStyleProperties(std::reference_wrapper<dom::CSSStyleDeclaration> &&rw) : Base(std::move(rw)) {}

    JSValue cssText(JSContext *ctx) const
    {
        if(!ref().impl.index()) return std::get<0>(ref().impl).cssText(ref());
        return JS_NULL;
    }

    void set_cssText(JSContext *ctx, bridge::String value)
    {
        if(!ref().impl.index()) ref().setAttribute({"style"}, value);
    }

    JSValue getPropertyPriority(JSContext *ctx, bridge::String name)
    {
        if(!ref().impl.index()) return bridge::String{ctx, std::get<0>(ref().impl).getPropertyPriority(ref(), name)};
        return bridge::String{ctx, std::string_view{}};
    }

    template<typename T, typename F>
    T invoke(dom::CSSStyleProperties::Comp const &impl, T def, F &&fn) const
    {
        T result = def;
        auto const *doc = dynamic_cast<dom::HTMLBackend*>(ref().doc.get());
        if(auto it = doc->styles.find(ref()); it != std::end(doc->styles))
            if(auto &props = CSSStyleProperties::get(it->second); !props.impl.index())
                result = fn(std::get<0>(props.impl));
        return result;
    }

    JSValue getPropertyValue(JSContext *ctx, bridge::String name)
    {
        return std::visit(boost::hana::overload_linearly(
            [this, &name](dom::CSSStyleProperties::Attr &impl){
                return impl.getPropertyValue(ref(), name);
            },
            [this, &name](dom::CSSStyleProperties::Comp &impl){
                return invoke(impl, JS_NULL, [this, &impl, &name](auto &props){
                    return impl.getPropertyValue(ref(), props, name);
                });
            }
        ), ref().impl);
    }

    JSValue item(JSContext *ctx, bridge::Number i)
    {
        return std::visit(boost::hana::overload_linearly(
            [this, i=static_cast<std::int32_t>(i)](dom::CSSStyleProperties::Attr &impl){
                return impl.item(ref(), i);
            },
            [this, i=static_cast<std::int32_t>(i)](dom::CSSStyleProperties::Comp &impl){
                return invoke(impl, JS_NULL, [this, &impl, i](auto &props){
                    return impl.item(ref(), props, i);
                });
            }
        ), ref().impl);
    }

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, std::visit(boost::hana::overload_linearly(
            [this](dom::CSSStyleProperties::Attr const &impl){
                return impl.length(ref());
            },
            [this](dom::CSSStyleProperties::Comp const &impl){
                return invoke(impl, std::uint32_t{0}, [this, &impl](auto &props){
                    return impl.length(ref(), props);
                });
            }
        ), ref().impl)};
    }

    JSValue names(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> arr{ctx, std::visit(boost::hana::overload_linearly(
            [this](dom::CSSStyleProperties::Attr const &impl){
                return impl.names(ref());
            },
            [this, ctx](dom::CSSStyleProperties::Comp const &impl){
                JSValue ret = invoke(impl, JS_NULL, [this, &impl](auto &props){
                    return impl.names(ref(), props);
                });
                if(!JS_IsNull(ret)) return ret;
                return static_cast<JSValue>(bridge::Array{ctx});
            }
        ), ref().impl) };
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

    JSValue removeProperty(JSContext *ctx, bridge::String name)
    {
        if(!ref().impl.index())
            return std::get<0>(ref().impl).removeProperty(ref(), name);
        return JS_NULL;
    }

    JSValue setProperty_0(JSContext *ctx, bridge::String name, bridge::String value)
    {
        if(!ref().impl.index()) std::get<0>(ref().impl).setProperty(ref(), name, value);
        return JS_UNDEFINED;
    }

    JSValue setProperty_1(JSContext *ctx, bridge::String name, bridge::String value, bridge::String priority)
    {
        if(auto sv = static_cast<std::string_view const &>(priority); sv.empty())
            return setProperty_0(ctx, name, value);
        else if(!ref().impl.index() && sv == "important")
            std::get<0>(ref().impl).setProperty(ref(), name, value, sv);
        return JS_UNDEFINED;
    }

    using setProperty = bridge::Function
    <
        &CSSStyleProperties::setProperty_0,
        &CSSStyleProperties::setProperty_1
    >;

    JSValue get_property(JSContext *ctx, char const *n) const
    {
        return std::visit(boost::hana::overload_linearly(
            [this, n](dom::CSSStyleProperties::Attr const &impl){
                return impl.get_property(ref(), n);
            },
            [this, n](dom::CSSStyleProperties::Comp const &impl){
                return invoke(impl, JS_UNDEFINED, [this, &impl, n](auto &props){
                    return impl.get_property(ref(), props, n);
                });
            }
        ), ref().impl);
    }

    bool has_property(JSContext *, char const *n) const
    {
        return std::visit(boost::hana::overload_linearly(
            [this, n](dom::CSSStyleProperties::Attr const &impl){
                return impl.has_property(ref(), n);
            },
            [this, n](dom::CSSStyleProperties::Comp const &impl){
                return invoke(impl, false, [this, &impl, n](auto &props){
                    return impl.has_property(ref(), props, n);
                });
            }
        ), ref().impl);
    }

    void set_property(JSContext *ctx, char const *n, JSValueConst value)
    {
        if(!ref().impl.index()) std::get<0>(ref().impl).set_property(ref(), n, bridge::Value{ctx, value}.toString());
    }

    BOOST_FORCEINLINE static void free(dom::CSSStyleProperties &self)
    {
        std::visit(boost::hana::overload_linearly([&self](auto &impl){
            impl.free(self);
        }), self.impl);
    }

    using ctor = bridge::Unconstructable<CSSStyleProperties>;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSCFunctionListEntry const CSSStyleProperties::funcs[] = {
    JS_CGETSET_DEF("cssText", &bridge::Getter<&CSSStyleProperties::cssText>, &bridge::Setter<&CSSStyleProperties::set_cssText>),
    JS_CGETSET_DEF("length", &bridge::Getter<&CSSStyleProperties::length>, NULL),

    JS_CFUNC_DEF("getPropertyPriority", 1, &bridge::Function<&CSSStyleProperties::getPropertyPriority>::invoke),
    JS_CFUNC_DEF("getPropertyValue", 1, &bridge::Function<&CSSStyleProperties::getPropertyValue>::invoke),
    JS_CFUNC_DEF("item", 1, &bridge::Function<&CSSStyleProperties::item>::invoke),
    JS_CFUNC_DEF("removeProperty", 1, &bridge::Function<&CSSStyleProperties::removeProperty>::invoke),
    JS_CFUNC_DEF("setProperty", 2, &CSSStyleProperties::setProperty::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&CSSStyleProperties::names>::invoke)
};

JSClassExoticMethods CSSStyleProperties::exoticMethods = {
    .has_property = &bridge::has_property<CSSStyleProperties>,
    .get_property = &bridge::get_property<CSSStyleProperties>,
    .set_property = &bridge::set_property<CSSStyleProperties>
};
