struct CharacterData : bridge::Interface<CharacterData, dom::Node, Node>
{
    CharacterData(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CharacterData(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    JSValue data(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().doc->characterData(ref())};
    }

    void set_data(JSContext *, bridge::String data)
    {
        ref().doc->nodeValue(ref(), data);
    }

    JSValue length(JSContext *ctx) const
    {
        auto const data = ref().doc->characterData(ref());
        return bridge::Number{ctx, static_cast<std::uint64_t>(data.size())};
    }

    JSValue appendData(JSContext *, bridge::String data)
    {
        std::string text{ref().doc->characterData(ref())};
        auto const &value = static_cast<std::string_view const &>(data);
        text.append(value.data(), value.size());
        ref().doc->nodeValue(ref(), text);
        return JS_UNDEFINED;
    }

    JSValue deleteData(JSContext *, bridge::Number offset, bridge::Number count)
    {
        std::string text{ref().doc->characterData(ref())};
        auto const o = static_cast<std::uint64_t>(std::max<std::int64_t>(0, offset.operator std::int64_t()));
        auto const c = static_cast<std::uint64_t>(std::max<std::int64_t>(0, count.operator std::int64_t()));
        if(o < text.size()) text.erase(o, c);
        ref().doc->nodeValue(ref(), text);
        return JS_UNDEFINED;
    }

    JSValue insertData(JSContext *, bridge::Number offset, bridge::String data)
    {
        std::string text{ref().doc->characterData(ref())};
        auto const o = std::min<std::uint64_t>(static_cast<std::uint64_t>(std::max<std::int64_t>(0, offset.operator std::int64_t())), text.size());
        auto const &value = static_cast<std::string_view const &>(data);
        text.insert(o, value.data(), value.size());
        ref().doc->nodeValue(ref(), text);
        return JS_UNDEFINED;
    }

    JSValue replaceData(JSContext *, bridge::Number offset, bridge::Number count, bridge::String data)
    {
        std::string text{ref().doc->characterData(ref())};
        auto const o = std::min<std::uint64_t>(
            static_cast<std::uint64_t>(std::max<std::int64_t>(0, offset.operator std::int64_t())),
            text.size());
        auto const c = static_cast<std::uint64_t>(
            std::max<std::int64_t>(0, count.operator std::int64_t()));
        auto const &value = static_cast<std::string_view const &>(data);
        text.replace(o, c, value.data(), value.size());
        ref().doc->nodeValue(ref(), text);
        return JS_UNDEFINED;
    }

    JSValue substringData(JSContext *ctx, bridge::Number offset, bridge::Number count) const
    {
        auto const data = ref().doc->characterData(ref());
        auto const o = static_cast<std::uint64_t>(std::max<std::int64_t>(0, offset.operator std::int64_t()));
        auto const c = static_cast<std::uint64_t>(std::max<std::int64_t>(0, count.operator std::int64_t()));
        return bridge::String{ctx, o < data.size() ? data.substr(o, c) : std::string_view{}};
    }

    using ctor = bridge::Unconstructable<CharacterData>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const CharacterData::funcs[] = {
    JS_CGETSET_DEF("data", &bridge::Getter<&CharacterData::data>, &bridge::Setter<&CharacterData::set_data>),
    JS_CGETSET_DEF("length", &bridge::Getter<&CharacterData::length>, NULL),

    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<>>::invoke),
    JS_CFUNC_DEF("before", 1, &bridge::Function<&NodeMixin::before_t<>>::invoke),
    JS_CFUNC_DEF("remove", 0, &bridge::Function<&NodeMixin::remove>::invoke),
    JS_CFUNC_DEF("replaceWith", 1, &bridge::Function<&NodeMixin::replaceWith_t<>>::invoke),

    JS_CFUNC_DEF("appendData", 1, &bridge::Function<&CharacterData::appendData>::invoke),
    JS_CFUNC_DEF("deleteData", 2, &bridge::Function<&CharacterData::deleteData>::invoke),
    JS_CFUNC_DEF("insertData", 2, &bridge::Function<&CharacterData::insertData>::invoke),
    JS_CFUNC_DEF("replaceData", 3, &bridge::Function<&CharacterData::replaceData>::invoke),
    JS_CFUNC_DEF("substringData", 2, &bridge::Function<&CharacterData::substringData>::invoke)
};
