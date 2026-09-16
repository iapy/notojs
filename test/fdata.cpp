#include <bridge.hpp>
#include <climits>

namespace {

JSValue one(JSContext *ctx, bridge::String x, bridge::String y)
{
    auto xs = static_cast<std::string>(x);
    auto ys = static_cast<std::string>(y);
    return bridge::String{ctx, xs + ys};
}

JSValue two(JSContext *ctx, bridge::String x, bridge::Number y)
{
    auto xs = static_cast<std::string>(x);
    auto ys = std::to_string(static_cast<std::int64_t>(y));
    return bridge::String{ctx, xs + ys};
}

JSValue bind(JSContext *ctx, bridge::String x)
{
    return bridge::FunctionData<&one, &two>::bind(ctx, x);
}

JSValue one_self(JSContext *ctx, JSValue self, bridge::String x, bridge::String y)
{
    bridge::Object result{ctx};
    result.set("self", JS_DupValue(ctx, self));
    result.set("value", one(ctx, x, y));
    return result;
}

JSValue two_self(JSContext *ctx, JSValue self, bridge::String x, bridge::Number y)
{
    bridge::Object result{ctx};
    result.set("self", JS_DupValue(ctx, self));
    result.set("value", two(ctx, x, y));
    return result;
}

JSValue bind_self_0(JSContext *ctx)
{
    return bridge::FunctionData<&one_self, &two_self>::bind(ctx);
}

JSValue bind_self_1(JSContext *ctx, bridge::String x)
{
    return bridge::FunctionData<&one_self, &two_self>::bind(ctx, x);
}

JSValue bind_self_2(JSContext *ctx, bridge::String x, bridge::String y)
{
    return bridge::FunctionData<&one_self>::bind(ctx, x, y);
}

using bind_self = bridge::Function<&bind_self_0, &bind_self_1, &bind_self_2>;

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

struct NonmovableNumber : bridge::Number
{
    using bridge::Number::Number;
    NonmovableNumber(NonmovableNumber const &) = delete;
    NonmovableNumber(NonmovableNumber &&) = delete;
};

struct OptionalTail : bridge::Tail<0, bridge::String>
{
    using bridge::Tail<0, bridge::String>::Tail;
    OptionalTail(OptionalTail const &) = delete;
    OptionalTail(OptionalTail &&) = delete;
};

using RequiredTail = bridge::Tail<1, bridge::String>;

// Report both tail endpoints and its size without copying nonmovable arguments.
template<typename Tail>
JSValue describe(JSContext *ctx, Prefix const &prefix, NonmovableNumber const &number, Tail const &tail)
{
    std::string value = static_cast<std::string>(prefix)
        + std::to_string(static_cast<std::int64_t>(number))
        + ":" + std::to_string(tail.size());
    if(tail.size())
    {
        value += ":" + static_cast<std::string>(tail[0]);
        value += ":" + static_cast<std::string>(bridge::String(ctx, tail.data()[tail.size() - 1]));
    }
    return bridge::String(ctx, value);
}

JSValue required(JSContext *ctx, Prefix prefix, NonmovableNumber number, RequiredTail tail)
{
    return describe(ctx, prefix, number, tail);
}

JSValue optional(JSContext *ctx, Prefix prefix, NonmovableNumber number, OptionalTail tail)
{
    return describe(ctx, prefix, number, tail);
}

template<typename Tail>
JSValue tail_self(JSContext *ctx, JSValueConst self, Prefix prefix, NonmovableNumber number, Tail tail)
{
    bridge::Object result{ctx};
    result.set("self", JS_DupValue(ctx, self));
    result.set("value", describe(ctx, prefix, number, tail));
    return result;
}

JSValue fixed(JSContext *ctx, bridge::String, bridge::Number, bridge::Number)
{
    return bridge::String(ctx, std::string_view{"fixed"});
}

template<auto... Fs>
JSValue capture_0(JSContext *ctx)
{
    return bridge::FunctionData<Fs...>::bind(ctx);
}

template<auto... Fs>
JSValue capture_1(JSContext *ctx, bridge::String prefix)
{
    return bridge::FunctionData<Fs...>::bind(ctx, prefix);
}

template<auto... Fs>
JSValue capture_2(JSContext *ctx, bridge::String prefix, bridge::Number number)
{
    return bridge::FunctionData<Fs...>::bind(ctx, prefix, number);
}

template<auto... Fs>
using Capture = bridge::Function<&capture_0<Fs...>, &capture_1<Fs...>, &capture_2<Fs...>>;

using BindRequired = Capture<&required>;
using BindOptional = Capture<&optional>;
using BindRequiredSelf = Capture<&tail_self<RequiredTail>>;
using BindOptionalSelf = Capture<&tail_self<OptionalTail>>;
using BindMixed = Capture<&fixed, &required>;

static_assert(bridge::FunctionData<&required>::arity() == INT_MAX);
static_assert(bridge::FunctionData<&required>::length() == 2);
static_assert(bridge::FunctionData<&optional>::length() == 2);
static_assert(bridge::FunctionData<&tail_self<OptionalTail>>::arity() == INT_MAX);
static_assert(bridge::FunctionData<&tail_self<RequiredTail>>::length() == 2);
static_assert(bridge::FunctionData<&fixed, &required>::arity() == INT_MAX);
static_assert(bridge::FunctionData<&fixed, &required>::length() == 3);
static_assert(bridge::FunctionData<&required, &fixed>::length() == 3);

JSValue rejects_invalid_captures(JSContext *ctx)
{
    using Plain = bridge::detail::function_data<&required>;
    using Self = bridge::detail::function_data<&tail_self<RequiredTail>>;
    using Optional = bridge::detail::function_data<&optional>;
    // Invalid metadata/counts must reject before touching any context or values.
    return JS_NewBool(ctx,
        !Plain::check(nullptr, JS_UNDEFINED, 3, nullptr, -1, nullptr)
        && !Plain::check(nullptr, JS_UNDEFINED, 1, nullptr, 3, nullptr)
        && !Plain::check(nullptr, JS_UNDEFINED, 2, nullptr, 0, nullptr)
        && !Plain::check(nullptr, JS_UNDEFINED, 1, nullptr, 1, nullptr)
        && !Plain::check(nullptr, JS_UNDEFINED, 0, nullptr, 2, nullptr)
        && !Self::check(nullptr, JS_UNDEFINED, 3, nullptr, -1, nullptr)
        && !Self::check(nullptr, JS_UNDEFINED, 1, nullptr, 3, nullptr)
        && !Self::check(nullptr, JS_UNDEFINED, 0, nullptr, 2, nullptr)
        && !Optional::check(nullptr, JS_UNDEFINED, 0, nullptr, 3, nullptr)
        && !Optional::check(nullptr, JS_UNDEFINED, -1, nullptr, 2, nullptr));
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("bind", 1, &bridge::Function<&bind>::invoke),
    JS_CFUNC_DEF("bindSelf", 2, &bind_self::invoke),
    JS_CFUNC_DEF("bindRequired", 2, &BindRequired::invoke),
    JS_CFUNC_DEF("bindOptional", 2, &BindOptional::invoke),
    JS_CFUNC_DEF("bindRequiredSelf", 2, &BindRequiredSelf::invoke),
    JS_CFUNC_DEF("bindOptionalSelf", 2, &BindOptionalSelf::invoke),
    JS_CFUNC_DEF("bindMixed", 2, &BindMixed::invoke),
    JS_CFUNC_DEF("rejectsInvalidCaptures", 0, &bridge::Function<&rejects_invalid_captures>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
