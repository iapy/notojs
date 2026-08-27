struct CSSRule : bridge::Interface<CSSRule, dom::CSSRule>
{
    CSSRule(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSRule(std::reference_wrapper<dom::CSSRule> &&rw) : Base(std::move(rw)) {}

    JSValue cssText(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().cssText()};
    }

    JSValue parentRule(JSContext *) const
    {
        return JS_NULL;
    }

    JSValue parentStyleSheet(JSContext *ctx) const;

    JSValue type(JSContext *ctx) const
    {
        return bridge::Number{ctx, static_cast<std::uint32_t>(ref().type())};
    }

    static void sprop(JSContext *ctx, JSValue ctor)
    {
        JS_DefinePropertyValueStr(ctx, ctor, "STYLE_RULE", bridge::Number{ctx, 1}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "CHARSET_RULE", bridge::Number{ctx, 2}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "IMPORT_RULE", bridge::Number{ctx, 3}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "MEDIA_RULE", bridge::Number{ctx, 4}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "FONT_FACE_RULE", bridge::Number{ctx, 5}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "PAGE_RULE", bridge::Number{ctx, 6}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "KEYFRAMES_RULE", bridge::Number{ctx, 7}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "KEYFRAME_RULE", bridge::Number{ctx, 8}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "MARGIN_RULE", bridge::Number{ctx, 9}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "NAMESPACE_RULE", bridge::Number{ctx, 10}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "COUNTER_STYLE_RULE", bridge::Number{ctx, 11}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "SUPPORTS_RULE", bridge::Number{ctx, 12}, JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, ctor, "FONT_FEATURE_VALUES_RULE", bridge::Number{ctx, 14}, JS_PROP_ENUMERABLE);
    }

    BOOST_FORCEINLINE static void free(dom::CSSRule &self)
    {
        self.free();
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<CSSRule>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const CSSRule::funcs[] = {
    JS_CGETSET_DEF("cssText", &bridge::Getter<&CSSRule::cssText>, NULL),
    JS_CGETSET_DEF("parentRule", &bridge::Getter<&CSSRule::parentRule>, NULL),
    JS_CGETSET_DEF("parentStyleSheet", &bridge::Getter<&CSSRule::parentStyleSheet>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&CSSRule::type>, NULL)
};
