#include <bridge.hpp>

namespace {

struct Nonmovable : bridge::Number
{
    using bridge::Number::Number;

    Nonmovable(Nonmovable const &) = delete;
    Nonmovable(Nonmovable &&) = delete;
};

struct NonmovableTail : bridge::Tail<0, bridge::Number>
{
    using bridge::Tail<0, bridge::Number>::Tail;

    NonmovableTail(NonmovableTail const &) = delete;
    NonmovableTail(NonmovableTail &&) = delete;
};

JSValue number(JSContext *ctx, Nonmovable n)
{
    return JS_DupValue(ctx, n);
}

JSValue sum(JSContext *ctx, Nonmovable n, NonmovableTail tail)
{
    std::int64_t value = n;
    for(std::size_t i = 0; i < tail.size(); ++i)
        value += static_cast<std::int64_t>(tail[i]);
    return bridge::Number(ctx, value);
}

struct Vararg : bridge::Interface<Vararg, std::string>
{
    Vararg() = default;

    JSValue number(JSContext *ctx, Nonmovable n)
    {
        return JS_DupValue(ctx, n);
    }

    JSValue sum(JSContext *ctx, Nonmovable n, NonmovableTail tail) const
    {
        std::int64_t value = n;
        for(std::size_t i = 0; i < tail.size(); ++i)
            value += static_cast<std::int64_t>(tail[i]);
        return bridge::Number(ctx, value);
    }

    static JSValue count(Vararg &value, JSContext *ctx, NonmovableTail tail)
    {
        value.ref() = std::to_string(tail.size());
        return JS_UNDEFINED;
    }

    JSValue append_0(JSContext *ctx, bridge::Tail<1, bridge::String> tail)
    {
        if(ref().size()) ref().append(" ");
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            if(i) ref().append(" ");
            ref().append(tail[i]);
        }
        return JS_UNDEFINED;
    }

    JSValue append_1(JSContext *ctx, bridge::Number n, bridge::Tail<0, bridge::String> tail)
    {
        if(ref().size()) ref().append(" ");
        ref().append(std::to_string(static_cast<std::int64_t>(n)));
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            ref().append(" ");
            ref().append(tail[i]);
        }
        return JS_UNDEFINED;
    }

    JSValue cappend_0(JSContext *ctx, bridge::Tail<1, bridge::String> tail) const
    {
        std::string value = ref();
        if(value.size()) value.append(" ");
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            if(i) value.append(" ");
            value.append(tail[i]);
        }
        return bridge::String(ctx, value);
    }

    JSValue cappend_1(JSContext *ctx, bridge::Number n, bridge::Tail<0, bridge::String> tail) const
    {
        std::string value = ref();
        if(value.size()) value.append(" ");
        value.append(std::to_string(static_cast<std::int64_t>(n)));
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            value.append(" ");
            value.append(tail[i]);
        }
        return bridge::String(ctx, value);
    }

    JSValue eappend(JSContext *ctx, bridge::Tail<1, bridge::String, bridge::Number> tail)
    {
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            if(!ref().empty()) ref().append(" ");
            if(auto s = tail.get<bridge::String>(i); s)
                ref().append(*s);
            else if(auto n = tail.get<bridge::Number>(i); n)
                ref().append(std::to_string(static_cast<std::int64_t>(*n)));
        }
        return JS_UNDEFINED;
    }

    JSValue get_value(JSContext *ctx) const
    {
        return bridge::String(ctx, ref());
    }

    using append = bridge::Function
    <
        &Vararg::append_0,
        &Vararg::append_1
    >;

    using cappend = bridge::Function
    <
        &Vararg::cappend_0,
        &Vararg::cappend_1
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Vararg::funcs[] = {
    JS_CFUNC_DEF("number", 1, &bridge::Function<&Vararg::number>::invoke),
    JS_CFUNC_DEF("sum", 1, &bridge::Function<&Vararg::sum>::invoke),
    JS_CFUNC_DEF("count", 0, &bridge::Function<&Vararg::count>::invoke),
    JS_CFUNC_DEF("append", 1, &Vararg::append::invoke),
    JS_CFUNC_DEF("cappend", 1, &Vararg::cappend::invoke),
    JS_CFUNC_DEF("eappend", 1, &bridge::Function<&Vararg::eappend>::invoke),
    JS_CGETSET_DEF("value", &bridge::Getter<&Vararg::get_value>, NULL),
};

struct Prefix : bridge::String
{
    using bridge::String::String;

    static bool valid(JSContext *ctx, JSValue *value, std::string &message)
    {
        if(static_cast<std::string_view>(bridge::String(ctx, *value)).empty())
        {
            message = "empty prefix";
            return false;
        }
        return true;
    }
};

struct OptionalTail : bridge::Tail<0, bridge::String>
{
    using bridge::Tail<0, bridge::String>::Tail;
};

using RequiredTail = bridge::Tail<1, bridge::String>;

template<typename Tail>
JSValue join(JSContext *ctx, std::string value, Tail tail)
{
    for(std::size_t i = 0; i < tail.size(); ++i)
    {
        if(!value.empty()) value.append(" ");
        value.append(tail[i]);
    }
    return bridge::String(ctx, value);
}

JSValue with_self(JSContext *ctx, JSValue value, JSValueConst self)
{
    JSValue result = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, result, 0, value);
    JS_SetPropertyUint32(ctx, result, 1, JS_DupValue(ctx, self));
    return result;
}

JSValue with_argv(JSContext *ctx, JSValue value, JSValue *argv, std::size_t count)
{
    JSValue result = with_self(ctx, value, argv[0]);
    JS_SetPropertyUint32(ctx, result, 2, JS_DupValue(ctx, argv[count - 1]));
    return result;
}

JSValue optional(JSContext *ctx, Prefix prefix, OptionalTail tail)
{
    return join(ctx, static_cast<std::string>(prefix), tail);
}

JSValue required(JSContext *ctx, RequiredTail tail)
{
    return join(ctx, "", tail);
}

JSValue argv_optional(JSContext *ctx, JSValue *argv, Prefix prefix, OptionalTail tail)
{
    return with_argv(ctx, optional(ctx, prefix, tail), argv, 1 + tail.size());
}

JSValue argv_required(JSContext *ctx, JSValue *argv, RequiredTail tail)
{
    return with_argv(ctx, required(ctx, tail), argv, tail.size());
}

JSValue self_optional(JSContext *ctx, JSValueConst self, Prefix prefix, OptionalTail tail)
{
    return with_self(ctx, optional(ctx, prefix, tail), self);
}

JSValue self_required(JSContext *ctx, JSValueConst self, RequiredTail tail)
{
    return with_self(ctx, required(ctx, tail), self);
}

JSValue fixed(JSContext *ctx, Prefix prefix)
{
    return JS_DupValue(ctx, prefix);
}

JSValue argv_fixed(JSContext *ctx, JSValue *argv, Prefix prefix)
{
    return with_argv(ctx, fixed(ctx, prefix), argv, 1);
}

JSValue self_fixed(JSContext *ctx, JSValueConst self, Prefix prefix)
{
    return with_self(ctx, fixed(ctx, prefix), self);
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("number", 1, &bridge::Function<&number>::invoke),
    JS_CFUNC_DEF("sum", 1, &bridge::Function<&sum>::invoke),
    JS_CFUNC_DEF("optional", 1, &bridge::Function<&optional>::invoke),
    JS_CFUNC_DEF("required", 1, &bridge::Function<&required>::invoke),
    JS_CFUNC_DEF("argvOptional", 1, &bridge::Function<&argv_optional>::invoke),
    JS_CFUNC_DEF("argvRequired", 1, &bridge::Function<&argv_required>::invoke),
    JS_CFUNC_DEF("selfOptional", 1, &bridge::Function<&self_optional>::invoke),
    JS_CFUNC_DEF("selfRequired", 1, &bridge::Function<&self_required>::invoke),
    JS_CFUNC_DEF("fixed", 1, &bridge::Function<&fixed>::invoke),
    JS_CFUNC_DEF("argvFixed", 1, &bridge::Function<&argv_fixed>::invoke),
    JS_CFUNC_DEF("selfFixed", 1, &bridge::Function<&self_fixed>::invoke),
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Vararg::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Vararg::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
