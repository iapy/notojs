#pragma once
#include <boost/core/demangle.hpp>
#include <boost/config.hpp>
#include <boost/hana.hpp>

#include <quickjs/quickjs.h>
#include <type_traits>
#include <functional>
#include <optional>
#include <variant>
#include <cstdint>
#include <vector>
#include <limits>
#include <memory>
#include <array>

#define BRIDGE_DEFINE_STRUCT(type) \
    BOOST_FORCEINLINE type(JSContext *ctx, JSValue value) \
    : bridge::Struct<type>(ctx, value) {}

namespace bridge {

template<int I, typename ...Ts>
struct Tail
{
    static constexpr int count = std::enable_if_t<I >= 0, std::integral_constant<int, I>>::value;
    static constexpr bool check(JSContext *ctx, JSValue *value) { return true; }

    BOOST_FORCEINLINE Tail(JSContext *ctx, int argc, JSValue *argv)
    : val{argv}, ctx{ctx}, len{static_cast<std::size_t>(argc)} {}

    BOOST_FORCEINLINE std::size_t size() const { return len; }

    template<typename T>
    BOOST_FORCEINLINE std::optional<T> get(std::size_t i) const
    {
        if(!T::check(ctx, val + i))
            return std::nullopt;
        else
            return T{ctx, *(val + i)};
    }

private:
    JSValue *val;
    JSContext *ctx;
    std::size_t len;
};

template<int I, typename T>
struct Tail<I, T>
{
    static constexpr int count = std::enable_if_t<I >= 0, std::integral_constant<int, I>>::value;
    static constexpr bool check(JSContext *ctx, JSValue *value) { return true; }

    BOOST_FORCEINLINE Tail(JSContext *ctx, int argc, JSValue *argv)
    : val{argv}, ctx{ctx}, len{static_cast<std::size_t>(argc)} {}

    BOOST_FORCEINLINE std::size_t size() const { return len; }
    BOOST_FORCEINLINE T operator [](std::size_t i) const { return T{ctx, *(val + i)}; }

private:
    JSValue *val;
    JSContext *ctx;
    std::size_t len;
};

namespace detail {

BOOST_FORCEINLINE void set_allocator(JSContext *ctx, JSValue p, JSValue a)
{
    JS_DefinePropertyValueStr(ctx, p, ".allocator", JS_DupValue(ctx, a), JS_PROP_CONFIGURABLE);
}

template<typename, typename = void>
struct has_constructor : std::false_type {};

template<typename T>
struct has_constructor<T, std::void_t<typename T::ctor>> : std::true_type {};

template<typename T, typename N>
auto has_deleter_impl(T*) -> decltype(T::free(std::declval<N&>()), std::true_type{});

template<typename T, typename N>
std::false_type has_deleter_impl(...);

template<typename T, typename N>
using has_deleter = decltype(has_deleter_impl<T, N>(0));

template<typename ...Args, std::size_t ...Is>
BOOST_FORCEINLINE bool check(JSContext *ctx, JSValueConst *argv, std::index_sequence<Is...>)
{
    return true && ((Args::check(ctx, argv + Is)) && ...);
}

template<class, class = void>
struct get_exotic
{
    BOOST_FORCEINLINE static JSClassExoticMethods *get() { return nullptr; }
};

template<class T>
struct get_exotic<T, std::void_t<decltype(&T::exoticMethods)>>
{
    BOOST_FORCEINLINE static JSClassExoticMethods *get() { return &T::exoticMethods; }
};

template<std::size_t, typename ...>
struct varargs_impl;

template<std::size_t N>
struct varargs_impl<N>
{
    using type = void;
    static constexpr int arity = N;
};

template<std::size_t N, int I, typename ...Ts>
struct varargs_impl<N, Tail<I, Ts...>>
{
    using type = Tail<I, Ts...>;
    static constexpr int arity = std::numeric_limits<int>::max();

    static bool check(JSContext *ctx, int argc, JSValue *argv)
    {
        for(int i = 0; i < argc; ++i)
            if((!Ts::check(ctx, argv + i) && ...)) return false;
        return true;
    }
};

template<std::size_t N, typename Arg, typename ...Args>
struct varargs_impl<N, Arg, Args...> : varargs_impl<N + 1, Args...> {};

template<std::size_t I, typename Arg, typename ...Args>
struct varargs_nth : varargs_nth<I - 1, Args...> {};

template<typename Arg, typename ...Args>
struct varargs_nth<0, Arg, Args...>
{
    using type = Arg;
};

template<typename ...Args>
using varargs = varargs_impl<0, Args...>;

template<typename ...Args, std::size_t ...Is>
BOOST_FORCEINLINE std::optional<std::string> valid(JSContext *ctx, JSValueConst *argv, std::index_sequence<Is...>)
{
    std::string error;
    return true && ((Args::valid(ctx, argv + Is, error)) && ...) ? std::optional<std::string>{} : std::optional<std::string>(std::move(error));
}

template<auto f, typename ...Args>
struct function_alt
{
    using Is = std::make_index_sequence<sizeof ...(Args)>;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, int argc, JSValueConst *argv)
    {
        return (argc == sizeof ...(Args)) && detail::check<Args...>(ctx, argv, Is{});
    }

    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue, JSValue *argv, std::index_sequence<Is...> is)
    {
        if(auto error = valid<Args...>(ctx, argv, is); error)
            return JS_ThrowTypeError(ctx, "%s", error->c_str());
        return f(ctx, Args(ctx, argv[Is])...);
    }
};

template<auto f, typename ...Args>
struct function_alt<f, JSValue*, Args...>
{
    using Is = std::make_index_sequence<sizeof ...(Args)>;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, int argc, JSValueConst *argv)
    {
        return (argc == sizeof ...(Args)) && detail::check<Args...>(ctx, argv, Is{});
    }

    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue, JSValue *argv, std::index_sequence<Is...> is)
    {
        if(auto error = valid<Args...>(ctx, argv, is); error)
            return JS_ThrowTypeError(ctx, "%s", error->c_str());
        return f(ctx, argv, Args(ctx, argv[Is])...);
    }
};

template<auto f, typename ...Args>
struct function_alt<f, JSValueConst, Args...>
{
    using Is = std::make_index_sequence<sizeof ...(Args)>;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, int argc, JSValueConst *argv)
    {
        return (argc == sizeof ...(Args)) && detail::check<Args...>(ctx, argv, Is{});
    }

    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue self, JSValue *argv, std::index_sequence<Is...> is)
    {
        if(auto error = valid<Args...>(ctx, argv, is); error)
            return JS_ThrowTypeError(ctx, "%s", error->c_str());
        return f(ctx, self, Args(ctx, argv[Is])...);
    }
};

template<typename Target, typename ...Args, std::size_t ...Is>
BOOST_FORCEINLINE Target construct(JSContext *ctx, JSValueConst *argv, std::index_sequence<Is...>)
{
    if constexpr(std::is_pointer_v<Target>)
        return new std::remove_pointer_t<Target>(Args(ctx, argv[Is])...);
    else
        return Target(Args(ctx, argv[Is])...);
}

template<auto>
struct getter : std::false_type {};

template<typename T, JSValue(T::*f)(JSContext *) const>
struct getter<f> : std::true_type
{
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValueConst self)
    {
        if(JSClassID const cid = JS_GetClassID(self); T::cid == cid)
            return (reinterpret_cast<T const *>(JS_GetOpaque(self, T::cid))->*f)(ctx);
        else if(auto it = T::upcast.find(cid); it != std::end(T::upcast))
            return (it->second(JS_GetOpaque(self, cid)).*f)(ctx);
        return JS_ThrowTypeError(ctx, "%s: getter invoked on a wrong type", T::name());
    }
};

template<typename T, JSValue(T::*f)(JSContext *, JSValue)>
struct getter<f> : std::true_type
{
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValueConst self)
    {
        if(JSClassID const cid = JS_GetClassID(self); T::cid == cid)
            return (reinterpret_cast<T*>(JS_GetOpaque(self, T::cid))->*f)(ctx, self);
        else if(auto it = T::upcast.find(cid); it != std::end(T::upcast))
            return (it->second(JS_GetOpaque(self, cid)).*f)(ctx, self);
        return JS_ThrowTypeError(ctx, "%s: getter invoked on a wrong type", T::name());
    }
};

template<auto>
struct setter : std::false_type {};

template<typename T, typename V, typename R, R(T::*f)(JSContext *, V)>
struct setter<f> : std::disjunction<std::is_same<void, R>, std::is_same<JSValue, R>>
{
    using Type = T;

    BOOST_FORCEINLINE static bool checka(JSContext *ctx, JSValue &arg)
    {
        return V::check(ctx, &arg);
    }

    BOOST_FORCEINLINE static bool checks(JSValue self)
    {
        JSClassID const cid = JS_GetClassID(self);
        return T::cid == cid || T::upcast.find(cid) != std::end(T::upcast);
    }

    BOOST_FORCEINLINE static auto valid(JSContext *ctx, JSValue &arg)
    {
        std::string error;
        return V::valid(ctx, &arg, error) ? std::optional<std::string>{} : std::optional<std::string>(std::move(error));
    }

    BOOST_FORCEINLINE static JSValue unsafe(JSContext *ctx, JSValueConst self, JSValue v)
    {
        if constexpr (std::is_same_v<R, JSValue>)
        {
            if(JSClassID const cid = JS_GetClassID(self); T::cid == cid)
                return (reinterpret_cast<T*>(JS_GetOpaque(self, T::cid))->*f)(ctx, V(ctx, v));
            else
                return (T::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, V(ctx, v));
        }
        else
        {
            if(JSClassID const cid = JS_GetClassID(self); T::cid == cid)
                (reinterpret_cast<T*>(JS_GetOpaque(self, T::cid))->*f)(ctx, V(ctx, v));
            else
                (T::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, V(ctx, v));
            return JS_UNDEFINED;
        }
    }
};

template<auto>
struct function : std::false_type {};

template<typename ...Args, JSValue(*f)(JSContext *, Args...)>
struct function<f> : std::true_type
{
    static constexpr int arity = sizeof...(Args);

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue, int argc, JSValue *argv)
    {
        return function_alt<f, Args...>::check(ctx, argc, argv);
    }

    inline static JSValue invoke(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        return function_alt<f, Args...>::invoke(ctx, self, argv, typename function_alt<f, Args...>::Is{});
    }
};

template<typename O, typename ...Args, JSValue(O::*f)(JSContext *, Args...)>
struct function<f> : std::true_type
{
    static constexpr int arity = varargs<Args...>::arity;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        JSClassID const cid = JS_GetClassID(self);
        if constexpr (std::is_same_v<typename varargs<Args...>::type, void>)
        {
            return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast))
                && argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{});
        }
        else
        {
            return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast))
                && argc >= sizeof...(Args) + varargs<Args...>::type::count - 1
                && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{})
                && detail::varargs<Args...>::check(ctx, argc - sizeof...(Args) + 1, argv + sizeof...(Args) - 1)
            ;
        }
    }

    inline static JSValue invoke(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        if constexpr (std::is_same_v<typename varargs<Args...>::type, void>)
            return invoke_<typename varargs<Args...>::type>(ctx, self, argv, std::make_index_sequence<sizeof ...(Args)>{});
        else
            return invoke_<typename varargs<Args...>::type>(ctx, self, argc, argv, std::make_index_sequence<sizeof ...(Args) - 1>{});
    }

private:
    template<typename T, std::size_t ...Is>
    BOOST_FORCEINLINE static auto invoke_(JSContext *ctx, JSValue self, JSValue *argv, std::index_sequence<Is...>)
    -> typename std::enable_if<std::is_same_v<T, void>, JSValue>::type
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O*>(JS_GetOpaque(self, cid))->*f)(ctx, Args(ctx, argv[Is])...);
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, Args(ctx, argv[Is])...);
    }

    template<typename T, std::size_t ...Is>
    BOOST_FORCEINLINE static auto invoke_(JSContext *ctx, JSValue self, int argc, JSValue *argv, std::index_sequence<Is...>)
    -> typename std::enable_if<!std::is_same_v<T, void>, JSValue>::type
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O*>(JS_GetOpaque(self, cid))->*f)(ctx, typename varargs_nth<Is, Args...>::type(ctx, argv[Is])...,
                    typename varargs<Args...>::type{ctx, argc - static_cast<int>(sizeof ...(Args) - 1), argv + sizeof ...(Args) - 1});
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, typename varargs_nth<Is, Args...>::type(ctx, argv[Is])...,
                    typename varargs<Args...>::type{ctx, argc - static_cast<int>(sizeof ...(Args) - 1), argv + sizeof ...(Args) - 1});
    }
};

template<typename O, typename ...Args, JSValue(O::*f)(JSContext *, Args...) const>
struct function<f> : std::true_type
{
    static constexpr int arity = varargs<Args...>::arity;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        JSClassID const cid = JS_GetClassID(self);
        if constexpr (std::is_same_v<typename varargs<Args...>::type, void>)
        {
            return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast))
                && argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{});
        }
        else
        {
            return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast))
                && argc >= sizeof...(Args) + varargs<Args...>::type::count - 1
                && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{})
                && detail::varargs<Args...>::check(ctx, argc - sizeof...(Args) + 1, argv + sizeof...(Args) - 1)
            ;
        }
    }

    inline static JSValue invoke(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        if constexpr (std::is_same_v<typename varargs<Args...>::type, void>)
            return invoke_<typename varargs<Args...>::type>(ctx, self, argv, std::make_index_sequence<sizeof ...(Args)>{});
        else
            return invoke_<typename varargs<Args...>::type>(ctx, self, argc, argv, std::make_index_sequence<sizeof ...(Args) - 1>{});
    }

private:
    template<typename T, std::size_t ...Is>
    BOOST_FORCEINLINE static auto invoke_(JSContext *ctx, JSValue self, JSValue *argv, std::index_sequence<Is...>)
    -> typename std::enable_if<std::is_same_v<T, void>, JSValue>::type
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O const *>(JS_GetOpaque(self, cid))->*f)(ctx, Args(ctx, argv[Is])...);
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, Args(ctx, argv[Is])...);
    }

    template<typename T, std::size_t ...Is>
    BOOST_FORCEINLINE static auto invoke_(JSContext *ctx, JSValue self, int argc, JSValue *argv, std::index_sequence<Is...>)
    -> typename std::enable_if<!std::is_same_v<T, void>, JSValue>::type
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O const *>(JS_GetOpaque(self, cid))->*f)(ctx, typename varargs_nth<Is, Args...>::type(ctx, argv[Is])...,
                    typename varargs<Args...>::type{ctx, argc - static_cast<int>(sizeof ...(Args) - 1), argv + sizeof ...(Args) - 1});
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(ctx, typename varargs_nth<Is, Args...>::type(ctx, argv[Is])...,
                    typename varargs<Args...>::type{ctx, argc - static_cast<int>(sizeof ...(Args) - 1), argv + sizeof ...(Args) - 1});
    }
};

template<typename O, typename ...Args, JSValue(O::*f)(JSValue, JSContext *, Args...)>
struct function<f> : std::true_type
{
    static constexpr int arity = sizeof...(Args);

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        JSClassID const cid = JS_GetClassID(self);
        return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast)) && argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{});
    }

    inline static JSValue invoke(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        return invoke(ctx, self, argv, std::make_index_sequence<sizeof ...(Args)>{});
    }

private:
    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue self, JSValue *argv, std::index_sequence<Is...>)
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O*>(JS_GetOpaque(self, cid))->*f)(self, ctx, Args(ctx, argv[Is])...);
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(self, ctx, Args(ctx, argv[Is])...);
    }
};

template<typename O, typename ...Args, JSValue(O::*f)(JSValue, JSContext *, Args...) const>
struct function<f> : std::true_type
{
    static constexpr int arity = sizeof...(Args);

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        JSClassID const cid = JS_GetClassID(self);
        return (O::cid == cid || O::upcast.find(cid) != std::end(O::upcast)) && argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{});
    }

    inline static JSValue invoke(JSContext *ctx, JSValue self, int argc, JSValue *argv)
    {
        return invoke(ctx, self, argv, std::make_index_sequence<sizeof ...(Args)>{});
    }

private:
    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue self, JSValue *argv, std::index_sequence<Is...>)
    {
        if(JSClassID const cid = JS_GetClassID(self); O::cid == cid)
            return (reinterpret_cast<O const *>(JS_GetOpaque(self, O::cid))->*f)(self, ctx, Args(ctx, argv[Is])...);
        else
            return (O::upcast.find(cid)->second(JS_GetOpaque(self, cid)).*f)(self, ctx, Args(ctx, argv[Is])...);
    }
};

template<auto>
struct function_data : std::false_type {};

template<typename ...Args, JSValue(*f)(JSContext *, JSValueConst *, Args...)>
struct function_data<f> : std::true_type
{
    static constexpr int arity = sizeof...(Args);

    BOOST_FORCEINLINE static bool check(JSContext *ctx, int argc, JSValue *argv)
    {
        return argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{});
    }

    inline static JSValue invoke(JSContext *ctx, JSValue *argv, JSValueConst *data)
    {
        if(auto error = valid<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{}); error)
            return JS_ThrowTypeError(ctx, "%s", error->c_str());
        return invoke(ctx, argv, data, std::make_index_sequence<sizeof ...(Args)>{});
    }

private:
    template<std::size_t ...Is>
    BOOST_FORCEINLINE static JSValue invoke(JSContext *ctx, JSValue *argv, JSValueConst *data, std::index_sequence<Is...>)
    {
        return f(ctx, data, Args(ctx, argv[Is])...);
    }
};

struct Reference
{
    BOOST_FORCEINLINE operator JSValue () const
    {
        return value;
    }
    BOOST_FORCEINLINE JSValue * operator + ()
    {
        return &value;
    }
    static constexpr bool valid(JSContext *ctx, JSValue *value, std::string &message)
    {
        return true;
    }
protected:
    BOOST_FORCEINLINE Reference(JSContext *ctx, JSValue value)
    : ctx{ctx}, value{value} {}

protected:
    JSContext *ctx;
    JSValue value;
};

template<typename T, bool O = has_constructor<T>::value>
struct Strong;

template<typename T>
struct Strong<T, false> : T
{
    BOOST_FORCEINLINE Strong(JSContext *ctx, JSValue value)
    : T{ctx, value} {}

    BOOST_FORCEINLINE T release()
    {
        return T{std::exchange(T::ctx, nullptr), T::value};
    }

    Strong(Strong &&) = delete;
    Strong(Strong const &) = delete;

    Strong &operator = (Strong &&) = delete;
    Strong &operator = (Strong const &) = delete;

    BOOST_FORCEINLINE ~Strong() { if(T::ctx) JS_FreeValue(T::ctx, T::value); }
};

template<typename T>
struct Strong<T, true>
{
    BOOST_FORCEINLINE Strong(JSContext *ctx, JSValue value)
    : ctx{ctx}, value{value} {}

    BOOST_FORCEINLINE typename T::Wrapped &ref()
    {
        return T::Base::get(value);
    }
    BOOST_FORCEINLINE T operator - ()
    {
        return T{ctx, value};
    }
    BOOST_FORCEINLINE operator JSValue () const
    {
        return value;
    }
    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
    {
        return T::check(ctx, value);
    }
    BOOST_FORCEINLINE ~Strong() { JS_FreeValue(ctx, value); }
private:
    JSContext *ctx;
    JSValue value;
};

template<>
struct Strong<void, false> : Reference
{
    BOOST_FORCEINLINE Strong(JSContext *ctx, JSValue value, bool dup = true)
    : Reference{ctx, dup ? JS_DupValue(ctx, value) : value} {}

    BOOST_FORCEINLINE Strong(Strong &&other)
    : Reference{other.ctx, other.value} { other.ctx = nullptr; }

    BOOST_FORCEINLINE ~Strong() { if(ctx) JS_FreeValue(ctx, value); }

    BOOST_FORCEINLINE JSValue swap(JSValue other)
    {
        JSValue res = value;
        value = other;
        return res;
    }

    BOOST_FORCEINLINE operator JSValue * ()
    {
        return &value;
    }

    BOOST_FORCEINLINE JSValue release()
    {
        ctx = nullptr;
        return value;
    }

    Strong(Strong const &) = delete;

    Strong &operator = (Strong &&) = delete;
    Strong &operator = (Strong const &) = delete;
};

class array_iterator
{
public:
    inline array_iterator(JSContext *ctx, JSValue arr)
    : arr{arr}, ctx{ctx}, cur{0}, len{0}
    {
        JSValue lenv = JS_GetPropertyStr(ctx, arr, "length");
        if(!JS_IsException(lenv))
        {
            if(JS_ToUint32(ctx, &len, lenv) < 0) len = 0;
        }
        JS_FreeValue(ctx, lenv);
    }

    array_iterator(array_iterator &&) = delete;
    array_iterator(array_iterator const&) = delete;

    array_iterator &operator = (array_iterator &&) = delete;
    array_iterator &operator = (array_iterator const&) = delete;

    BOOST_FORCEINLINE array_iterator& operator ++ ()
    {
        ++cur;
        return *this;
    }

    BOOST_FORCEINLINE operator bool () const
    {
        return cur != len;
    }

    template<typename T>
    BOOST_FORCEINLINE Strong<T> val()
    {
        return {ctx, JS_GetPropertyUint32(ctx, arr, cur)};
    }

    BOOST_FORCEINLINE Strong<void> operator * ()
    {
        return {ctx, JS_GetPropertyUint32(ctx, arr, cur), false};
    }
private:
    JSValue arr;
    JSContext *ctx;
    std::uint32_t cur;
    std::uint32_t len;
};

class object_iterator
{
public:
    inline object_iterator(JSContext *ctx, JSValue obj)
    : obj{obj}, ctx{ctx}, cur{0}, len{0}
    {
        if(JS_GetOwnPropertyNames(ctx, &tab, &len, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) != 0)
            tab = nullptr;
    }

    object_iterator(object_iterator &&) = delete;
    object_iterator(object_iterator const&) = delete;

    object_iterator &operator = (object_iterator &&) = delete;
    object_iterator &operator = (object_iterator const&) = delete;

    BOOST_FORCEINLINE object_iterator& operator ++ ()
    {
        JS_FreeAtom(ctx, tab[cur++].atom);
        return *this;
    }

    BOOST_FORCEINLINE operator bool () const
    {
        return cur != len;
    }

    BOOST_FORCEINLINE JSAtom atom() const
    {
        return tab[cur].atom;
    }

    BOOST_FORCEINLINE std::string_view key() const
    {
        return {JS_AtomToCString(ctx, tab[cur].atom)};
    }

    template<typename T>
    BOOST_FORCEINLINE Strong<T> val()
    {
        return {ctx, JS_GetPropertyInternal(ctx, obj, tab[cur].atom, obj, 0)};
    }

    BOOST_FORCEINLINE Strong<void> operator * ()
    {
        return {ctx, JS_GetPropertyInternal(ctx, obj, tab[cur].atom, obj, 0), false};
    }

    BOOST_FORCEINLINE ~object_iterator()
    {
        for(;cur < len; ++cur) JS_FreeAtom(ctx, tab[cur].atom);
        js_free(ctx, tab);
    }
private:
    JSValue obj;
    JSContext *ctx;
    std::uint32_t cur;
    std::uint32_t len;
    JSPropertyEnum *tab;
};

template<typename T>
class optional
{
    static constexpr std::size_t SIZE = sizeof(T) + 1;
    static constexpr std::size_t FLAG = SIZE - 1;
public:
    optional(optional const &) = delete;
    optional &operator = (optional &&) = delete;
    optional &operator = (optional const &) = delete;

    BOOST_FORCEINLINE optional(optional &&other)
    {
        if((data[FLAG] = other.data[FLAG]) & 1) new (data) T{std::move(*reinterpret_cast<T*>(other.data))};
    }

    BOOST_FORCEINLINE optional(std::nullopt_t)
    {
        data[FLAG] = 0;
    }
    BOOST_FORCEINLINE ~optional()
    {
        if(data[FLAG] & 1) reinterpret_cast<T*>(data)->~T();
    }

    template<typename ...Args>
    BOOST_FORCEINLINE optional(std::in_place_t, Args&& ...args)
    {
        new (data) T{std::forward<Args>(args)...};
        data[FLAG] = 1;
    }

    BOOST_FORCEINLINE optional& operator = (std::nullopt_t)
    {
        reinterpret_cast<T*>(data)->~T();
        data[FLAG] &= 2;
        return *this;
    }

    BOOST_FORCEINLINE operator bool() const
    {
        return data[FLAG] & 1;
    }

    BOOST_FORCEINLINE void set() { data[FLAG] |= 2; }
    BOOST_FORCEINLINE bool get() { return data[FLAG] & 2; }

private:
    std::uint8_t data[SIZE];
};

} // namespace detail
constexpr std::monostate unsafe;

template<typename T>
constexpr auto field(const char *name)
{
    return boost::hana::make_pair(boost::hana::type_c<T>, name);
}

template<typename ...Ts>
constexpr auto fields(Ts&&... f)
{
    return boost::hana::make_tuple(f...);
}

template<typename T>
using Strong = detail::Strong<T>;

struct Array : detail::Reference
{
    BOOST_FORCEINLINE Array(JSContext *ctx)
    : detail::Reference{ctx, JS_NewArray(ctx)} {}

    BOOST_FORCEINLINE Array(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    inline std::uint32_t size() const
    {
        std::uint32_t len{0};
        JSValue lenv = JS_GetPropertyStr(ctx, value, "length");
        if(!JS_IsException(lenv))
        {
            if(JS_ToUint32(ctx, &len, lenv) < 0) len = 0;
        }
        JS_FreeValue(ctx, lenv);
        return len;
    }

    template<typename T>
    BOOST_FORCEINLINE std::optional<detail::Strong<T>> at(std::uint32_t i)
    {
        JSValue v = JS_GetPropertyUint32(ctx, value, i);
        if(!T::check(ctx, &v))
        {
            JS_FreeValue(ctx, v);
            return std::nullopt;
        }
        return std::optional<Strong<T>>{std::in_place_t{}, ctx, v};
    }

    BOOST_FORCEINLINE detail::Strong<void> operator [](std::uint32_t i)
    {
        return {ctx, JS_GetPropertyUint32(ctx, value, i), false};
    }

    BOOST_FORCEINLINE void set(std::uint32_t i, JSValue v)
    {
        JS_SetPropertyUint32(ctx, value, i, v);
    }

    BOOST_FORCEINLINE void append(JSValue v)
    {
        JS_SetPropertyUint32(ctx, value, size(), v);
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsArray(ctx, *value); }
};

struct ArrayBuffer : detail::Reference
{
    BOOST_FORCEINLINE ArrayBuffer(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE ArrayBuffer(JSContext *ctx, std::string const &str)
    : detail::Reference{ctx, JS_NewArrayBufferCopy(ctx, reinterpret_cast<std::uint8_t const *>(str.c_str()), str.size())} {}

    BOOST_FORCEINLINE ArrayBuffer(JSContext *ctx, std::string &str, JSValue alloc)
    : detail::Reference{ctx, JS_NewArrayBuffer(ctx, reinterpret_cast<std::uint8_t *>(const_cast<char *>(str.c_str())), str.size(), NULL, NULL, 0)}
    {
        detail::set_allocator(ctx, value, alloc);
    }

    BOOST_FORCEINLINE ArrayBuffer(JSContext *ctx, std::uint8_t const *data, std::size_t size)
    : detail::Reference{ctx, JS_NewArrayBufferCopy(ctx, data, size)} {}

    BOOST_FORCEINLINE ArrayBuffer(JSContext *ctx, std::uint8_t const *data, std::size_t size, JSValue alloc)
    : detail::Reference{ctx, JS_NewArrayBuffer(ctx, const_cast<std::uint8_t *>(data), size, NULL, NULL, 0)}
    {
        detail::set_allocator(ctx, value, alloc);
    }

    BOOST_FORCEINLINE std::pair<std::uint8_t const *, std::size_t> data() const
    {
        std::size_t s;
        std::uint8_t const *d = JS_GetArrayBuffer(ctx, &s, value);
        return {d, s};
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { std::size_t s; return nullptr != JS_GetArrayBuffer(ctx, &s, *value); }
};

struct Boolean : detail::Reference
{
    BOOST_FORCEINLINE Boolean(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE operator bool() const
    {
        return JS_ToBool(ctx, value);
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsBool(*value); }
};

struct Null : detail::Reference
{
    BOOST_FORCEINLINE Null(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsNull(*value); }
};

template<typename T, typename ...Ts>
struct Either
{
    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
    {
        return T::check(ctx, value) || ((Ts::check(ctx, value)) || ...);
    }

    BOOST_FORCEINLINE static bool valid(JSContext *ctx, JSValue *value, std::string &message)
    {
        return (!T::check(ctx, value) || T::valid(ctx, value, message)) && (((!Ts::check(ctx, value) || Ts::valid(ctx, value, message))) || ...);
    }
};

struct Lambda : detail::Reference
{
    BOOST_FORCEINLINE Lambda(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE detail::Strong<void> operator () ()
    {
        return detail::Strong<void>{ctx, JS_Call(ctx, value, JS_UNDEFINED, 0, NULL), false};
    }

    BOOST_FORCEINLINE detail::Strong<void> operator () (JSValue self)
    {
        return detail::Strong<void>{ctx, JS_Call(ctx, value, self, 0, NULL), false};
    }

    BOOST_FORCEINLINE detail::Strong<void> operator () (std::vector<JSValue> &args)
    {
        return detail::Strong<void>{ctx, JS_Call(ctx, value, JS_UNDEFINED, args.size(), &args[0]), false};
    }

    template<std::size_t N>
    BOOST_FORCEINLINE detail::Strong<void> operator () (std::array<JSValue, N>&&args)
    {
        return detail::Strong<void>{ctx, JS_Call(ctx, value, JS_UNDEFINED, N, &args[0]), false};
    }

    template<std::size_t N>
    BOOST_FORCEINLINE detail::Strong<void> operator () (JSValue self, std::array<JSValue, N> &&args)
    {
        return detail::Strong<void>{ctx, JS_Call(ctx, value, self, N, &args[0]), false};
    }

    template<typename T>
    inline std::optional<Strong<T>> invoke()
    {
        if(JSValue v = JS_Call(ctx, value, JS_UNDEFINED, 0, NULL); !Strong<T>::check(ctx, &v))
        {
            JS_FreeValue(ctx, v);
            return std::nullopt;
        }
        else
        {
            return std::optional<Strong<T>>{std::in_place_t{}, ctx, v};
        }
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsFunction(ctx, *value); }
};

struct Number : detail::Reference
{
    BOOST_FORCEINLINE Number(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE Number(JSContext *ctx, std::int32_t value)
    : detail::Reference{ctx, JS_NewInt32(ctx, value)} {}

    BOOST_FORCEINLINE Number(JSContext *ctx, std::uint32_t value)
    : detail::Reference{ctx, JS_NewInt64(ctx, value)} {}

    BOOST_FORCEINLINE Number(JSContext *ctx, std::int64_t value)
    : detail::Reference{ctx, JS_NewInt64(ctx, value)} {}

    BOOST_FORCEINLINE Number(JSContext *ctx, std::uint64_t value)
    : detail::Reference{ctx, JS_NewInt64(ctx, value)} {}

    BOOST_FORCEINLINE operator std::int32_t () const
    {
        std::int32_t v;
        JS_ToInt32(ctx, &v, value);
        return v;
    }

    BOOST_FORCEINLINE operator std::int64_t () const
    {
        std::int64_t v;
        JS_ToInt64(ctx, &v, value);
        return v;
    }

    BOOST_FORCEINLINE double as_double() const
    {
        double v;
        JS_ToFloat64(ctx, &v, value);
        return v;
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsNumber(*value); }
};

struct String : detail::Reference
{
    BOOST_FORCEINLINE String(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE String(JSContext *ctx, String const &value)
    : detail::Reference{ctx, JS_NewStringLen(ctx,
        static_cast<std::string_view const &>(value).data(),
        static_cast<std::string_view const &>(value).size()
    )} {}

    BOOST_FORCEINLINE String(JSContext *ctx, std::string const &value)
    : detail::Reference{ctx, JS_NewStringLen(ctx, value.c_str(), value.size())} {}

    BOOST_FORCEINLINE String(JSContext *ctx, std::string_view const &value)
    : detail::Reference{ctx, JS_NewStringLen(ctx, value.data(), value.size())} {}

    inline ~String()
    {
        if(data) JS_FreeCString(ctx, data->data());
    }

    inline operator std::string () const
    {
        if(!data)
        {
            std::size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, value);
            data.emplace(str, len);
        }
        return std::string(data->data(), data->size());
    }

    inline operator std::string_view const &() const
    {
        if(!data)
        {
            std::size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, value);
            data.emplace(str, len);
        }
        return *data;
    }

    BOOST_FORCEINLINE std::string_view::const_iterator begin() const
    {
        return std::begin(static_cast<std::string_view const &>(*this));
    }

    BOOST_FORCEINLINE std::string_view::const_iterator end() const
    {
        return std::end(static_cast<std::string_view const &>(*this));
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsString(*value); }

private:
    mutable std::optional<std::string_view> data;
};

struct Object : detail::Reference
{
    BOOST_FORCEINLINE Object(JSContext *ctx)
    : detail::Reference{ctx, JS_NewObject(ctx)} {}

    BOOST_FORCEINLINE Object(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE detail::object_iterator begin()
    {
        return detail::object_iterator(ctx, value);
    }

    BOOST_FORCEINLINE bool has(char const *key) const
    {
        JSAtom atom = JS_NewAtom(ctx, key);
        int has = JS_HasProperty(ctx, value, atom);
        JS_FreeAtom(ctx, atom);
        return 1 == has;
    }

    BOOST_FORCEINLINE detail::Strong<void> operator [] (char const *key) const
    {
        return {ctx, JS_GetPropertyStr(ctx, value, key), false};
    }

    template<typename T>
    BOOST_FORCEINLINE std::optional<Strong<T>> get(char const *key) const
    {
        if(JSValue v = JS_GetPropertyStr(ctx, value, key); !Strong<T>::check(ctx, &v))
        {
            JS_FreeValue(ctx, v);
            return std::nullopt;
        }
        else
        {
            return std::optional<Strong<T>>{std::in_place_t{}, ctx, v};
        }
    }

    BOOST_FORCEINLINE void remove(char const *key)
    {
        JSAtom atom = JS_NewAtom(ctx, key);
        JS_DeleteProperty(ctx, value, atom, 0);
        JS_FreeAtom(ctx, atom);
    }

    BOOST_FORCEINLINE void set(char const *key, JSValue v)
    {
        JS_SetPropertyStr(ctx, value, key, v);
    }

    BOOST_FORCEINLINE void set(JSAtom atom, JSValue v)
    {
        JS_SetProperty(ctx, value, atom, v);
    }

    BOOST_FORCEINLINE Strong<String> json() const
    {
        return {ctx, JS_JSONStringify(ctx, value, JS_UNDEFINED, JS_UNDEFINED)};
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsObject(*value); }
};


template<typename Impl>
struct Struct : protected Object
{
    static bool check(JSContext *ctx, JSValue *value)
    {
        if(!Object::check(ctx, value) || JS_IsArray(ctx, *value)) return false;

        bool result{true};
        boost::hana::for_each(Impl::fields, [ctx, value, &result](auto pair){
            JSValue v = JS_GetPropertyStr(ctx, *value, boost::hana::second(pair));
            if(!JS_IsUndefined(v)) result &= std::decay_t<decltype(boost::hana::first(pair))>::type::check(ctx, &v);
            JS_FreeValue(ctx, v);
        });
        return result;
    }

    static bool valid(JSContext *ctx, JSValue *value, std::string &message)
    {
        boost::hana::for_each(Impl::fields, [ctx, value, &message](auto pair){
            JSValue v = JS_GetPropertyStr(ctx, *value, boost::hana::second(pair));
            if(!JS_IsUndefined(v)) std::decay_t<decltype(boost::hana::first(pair))>::type::valid(ctx, &v, message);
            JS_FreeValue(ctx, v);
        });
        return message.empty();
    }

    using Object::get;
protected:
    BOOST_FORCEINLINE Struct(JSContext *ctx, JSValue value)
    : Object{ctx, value} {}
};

struct Error : detail::Reference
{
    BOOST_FORCEINLINE Error(JSContext *ctx)
    : detail::Reference{ctx, JS_NewError(ctx)} {}

    BOOST_FORCEINLINE Error(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    inline std::optional<std::string> message()
    {
        if(JSValue v = JS_GetPropertyStr(ctx, value, "message"); !String::check(ctx, &v))
        {
            JS_FreeValue(ctx, v);
            return std::nullopt;
        }
        else
        {
            return std::optional<std::string>(Strong<String>(ctx, v));
        }
    }

    BOOST_FORCEINLINE void message(String const &str)
    {
        JS_SetPropertyStr(ctx, value, "message", str);
    }

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value) { return JS_IsException(*value); }
};

struct Value : detail::Reference
{
    BOOST_FORCEINLINE Value(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    BOOST_FORCEINLINE detail::Strong<String> toString()
    {
        return {ctx, JS_ToString(ctx, value)};
    }

    static constexpr bool check(JSContext *ctx, JSValue *value) { return true; }
    BOOST_FORCEINLINE operator JSContext *() const { return ctx; }
};

template<typename Value>
struct Dict : detail::Reference
{
    BOOST_FORCEINLINE Dict(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    template<typename F>
    inline void each(F &&f)
    {
        if(JS_IsArray(ctx, value))
        {
            for(detail::array_iterator it(ctx, value); it; ++it)
            {
                auto a = Array(ctx, *it);
                f(static_cast<std::string_view const &>(String(ctx, a[0])), Value(ctx, a[1]));
            }
        }
        else
        {
            for(detail::object_iterator it(ctx, value); it; ++it)
                f(it.key(), it.val<Value>());
        }
    }

    static bool check(JSContext *ctx, JSValue *value)
    {
        if(!JS_IsObject(*value)) return false;

        if(JS_IsArray(ctx, *value))
        {
            for(detail::array_iterator it(ctx, *value); it; ++it)
            {
                if(!JS_IsArray(ctx, *it)) return false;

                if(auto a = Array(ctx, *it); 2 != a.size())
                    return false;
                else if(!String::check(ctx, a[0]) || !Value::check(ctx, a[1]))
                    return false;
            }
            return true;
        }
        else
        {
            for(detail::object_iterator it(ctx, *value); it; ++it)
                if(!Value::check(ctx, *it)) return false;
            return true;
        }
    }
};

struct Promise : detail::Reference
{
    BOOST_FORCEINLINE Promise(JSContext *ctx, JSValue value)
    : detail::Reference{ctx, value} {}

    static bool check(JSContext *ctx, JSValue *value)
    {
        if(!JS_IsObject(*value))
            return false;

        JSValue glob = JS_GetGlobalObject(ctx);
        JSValue ctor = JS_GetPropertyStr(ctx, glob, "Promise");

        JSValue pproto = JS_GetPropertyStr(ctx, ctor, "prototype");

        JS_FreeValue(ctx, ctor);
        JS_FreeValue(ctx, glob);

        JSValue cproto = JS_GetPrototype(ctx, *value);
        bool is_instance = false;

        while(JS_IsObject(cproto))
        {
            if(JS_StrictEq(ctx, cproto, pproto))
            {
                is_instance = true;
                break;
            }

            JSValue nproto = JS_GetPrototype(ctx, cproto);
            JS_FreeValue(ctx, cproto);
            cproto = nproto;
        }

        JS_FreeValue(ctx, cproto);
        JS_FreeValue(ctx, pproto);

        return is_instance;
    }

    BOOST_FORCEINLINE Strong<Promise> wrap(JSCFunction f1, JSCFunction f2)
    {
        JSAtom atom = JS_NewAtom(ctx, "then");
        JSValue fns[2] = {
            JS_NewCFunction(ctx, f1, "accept", 1),
            JS_NewCFunction(ctx, f2, "reject", 1)
        };
        JSValue p = JS_Invoke(ctx, value, atom, 2, fns);
        JS_FreeValue(ctx, fns[0]);
        JS_FreeValue(ctx, fns[1]);
        JS_FreeAtom(ctx, atom);
        return {ctx, p};
    }

    static constexpr int MAGIC = 0x789A0000;
    static constexpr int AMASK = 0x0000FFFF;

    BOOST_FORCEINLINE Strong<Promise> wrap(JSCFunctionData f1, JSCFunction f2, int argc, JSValueConst *argv)
    {
        JSAtom atom = JS_NewAtom(ctx, "then");
        JSValue fns[2] = {
            JS_NewCFunctionData(ctx, f1, 1, MAGIC | argc, argc, argv),
            JS_NewCFunction(ctx, f2, "reject", 1)
        };
        JSValue p = JS_Invoke(ctx, value, atom, 2, fns);
        JS_FreeValue(ctx, fns[0]);
        JS_FreeValue(ctx, fns[1]);
        JS_FreeAtom(ctx, atom);
        return {ctx, p};
    }
};

template<auto F, typename _ = std::enable_if_t<detail::getter<F>::value>>
JSValue Getter(JSContext *ctx, JSValue self)
{
    return detail::getter<F>::invoke(ctx, self);
}

template<auto F, typename _ = std::enable_if_t<detail::setter<F>::value>>
JSValue Setter(JSContext *ctx, JSValue self, JSValue v)
{
    if(!detail::setter<F>::checks(self))
        return JS_ThrowTypeError(ctx, "%s: setter invoked on a wrong type", detail::setter<F>::Type::name());
    if(!detail::setter<F>::checka(ctx, v))
        return JS_ThrowTypeError(ctx, "%s: no matching setter found", detail::setter<F>::Type::name());
    if(auto error = detail::setter<F>::valid(ctx, v); error)
        return JS_ThrowTypeError(ctx, "%s: %s", detail::setter<F>::Type::name(), error->c_str());
    return detail::setter<F>::unsafe(ctx, self, v);
}

template<auto F, auto ...Fs>
struct Setters
{
    static_assert(std::conjunction_v<std::is_same<typename detail::setter<F>::Type, typename detail::setter<Fs>::Type>...>);

    inline static JSValue invoke(JSContext *ctx, JSValue self, JSValue v)
    {
        if(!detail::setter<F>::checks(self)) return JS_ThrowTypeError(ctx, "%s: setter invoked on a wrong type", detail::setter<F>::Type::name());
        return unsafe<F, Fs...>(ctx, self, v);
    }
private:
    template<auto G, auto ...Gs>
    BOOST_FORCEINLINE static JSValue unsafe(JSContext *ctx, JSValue self, JSValue v)
    {
        if(!detail::setter<G>::checka(ctx, v))
        {
            if constexpr (sizeof ...(Gs)) return unsafe<Gs...>(ctx, self, v);
            else return JS_ThrowTypeError(ctx, "%s: no matching setter found", detail::setter<G>::Type::name());
        }
        if(auto error = detail::setter<F>::valid(ctx, v); error)
            return JS_ThrowTypeError(ctx, "%s: %s", detail::setter<F>::Type::name(), error->c_str());

        return detail::setter<G>::unsafe(ctx, self, v);
    }
};

template<typename...>
struct Constructor;

template<typename Type>
struct Unconstructable
{
    inline static JSValue invoke(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
    {
        return JS_ThrowTypeError(ctx, "%s: no constructor", Type::name());
    }
};

template<typename Type, typename ...Tail>
struct Constructor<Type(JSContext *, JSValue, std::monostate), Tail...>
{
    inline static JSValue invoke(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
    {
        if(argc == 0)
        {
            JSValue proto = JS_GetPropertyStr(ctx, clazz, "prototype");
            JSValue result = JS_NewObjectProtoClass(ctx, proto, Type::cid);
            JS_FreeValue(ctx, proto);

            JS_SetOpaque(result, (new Type(ctx, result, std::monostate{}))->release());
            return result;
        }
        else if constexpr (sizeof...(Tail))
        {
            return Constructor<Tail...>::invoke(ctx, clazz, argc, argv);
        }
        return JS_ThrowTypeError(ctx, "%s: no matching constructor found", Type::name());
    }
};

template<typename Type, typename ...Args, typename ...Tail>
struct Constructor<Type(Args...), Tail...>
{
    inline static JSValue invoke(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
    {
        if(argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{}))
        {
            if(auto error = detail::valid<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{}); error)
                return JS_ThrowTypeError(ctx, "%s constructor: %s", Type::name(), error->c_str());

            JSValue proto = JS_GetPropertyStr(ctx, clazz, "prototype");
            JSValue result = JS_NewObjectProtoClass(ctx, proto, Type::cid);
            JS_FreeValue(ctx, proto);

            JS_SetOpaque(result, detail::construct<Type*, Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{})->release());
            return result;
        }
        else if constexpr (sizeof...(Tail))
        {
            return Constructor<Tail...>::invoke(ctx, clazz, argc, argv);
        }
        return JS_ThrowTypeError(ctx, "%s: no matching constructor found", Type::name());
    }

    inline static std::optional<Type> invoke(JSContext *ctx, int argc, JSValueConst *argv)
    {
        if(argc == sizeof...(Args) && detail::check<Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{}))
        {
            return std::move(detail::construct<Type, Args...>(ctx, argv, std::make_index_sequence<sizeof ...(Args)>{}));
        }
        else if constexpr (sizeof...(Tail))
        {
            return Constructor<Tail...>::invoke(ctx, argc, argv);
        }
        JS_ThrowTypeError(ctx, "%s: no matching constructor found", Type::name());
        return std::nullopt;
    }
};

template<auto F, auto ...Fs>
struct Function
{
    static constexpr int arity()
    {
        if constexpr (sizeof...(Fs))
            return std::max(detail::function<F>::arity, Function<Fs...>::arity());
        else
            return detail::function<F>::arity;
    }

    static JSValue invoke(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
    {
        return invoke_internal(ctx, self, std::min(argc, arity()), argv);
    }

    BOOST_FORCEINLINE static JSValue invoke_internal(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
    {
        if(detail::function<F>::check(ctx, self, argc, argv)) return detail::function<F>::invoke(ctx, self, argc, argv);
        if constexpr (sizeof...(Fs)) return Function<Fs...>::invoke_internal(ctx, self, argc, argv);
        return JS_ThrowTypeError(ctx, "No matching function overload found");
    }
};

template<auto F, auto ...Fs>
struct FunctionData
{
    static constexpr int arity()
    {
        if constexpr (sizeof...(Fs))
            return std::max(detail::function_data<F>::arity, FunctionData<Fs...>::arity());
        else
            return detail::function_data<F>::arity;
    }

    static JSValue invoke(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int d_id, JSValueConst *data)
    {
        return invoke_internal(ctx, self, std::min(argc, arity()), argv, d_id, data);
    }

    inline static JSValue invoke_internal(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int d_id, JSValueConst *data)
    {
        if(detail::function_data<F>::check(ctx, argc, argv)) return detail::function_data<F>::invoke(ctx, argv, data);
        if constexpr (sizeof...(Fs)) return FunctionData<Fs...>::invoke(ctx, self, argc, argv, d_id, data);
        return JS_ThrowTypeError(ctx, "No matching function overload found");
    }
};

template<typename ...>
struct Private;

template<typename Head, typename ...Tail>
struct Private<Head, Tail...>
{
    template<typename Base>
    static void init()
    {
        static std::string name = std::string(Base::name()) + "$" + std::to_string(sizeof ...(Tail));
        Head::def.class_name = name.c_str();

        JS_NewClassID(&Head::cid);
        Private<Tail...>::template init<Base>();
    }

    BOOST_FORCEINLINE static void init(JSRuntime *rt)
    {
        JS_NewClass(rt, Head::cid, &Head::def);
        Private<Tail...>::init(rt);
    }

    BOOST_FORCEINLINE static void make(JSContext *ctx)
    {
        Head::proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, Head::proto, Head::funcs, sizeof(Head::funcs)/sizeof(Head::funcs[0]));
        JS_SetClassProto(ctx, Head::cid, Head::proto);
        Private<Tail...>::make(ctx);
    }
};

template<>
struct Private<>
{
    template<typename Base>
    static constexpr void init() {}
    static constexpr void init(JSRuntime *) {}
    static constexpr void make(JSContext *) {}
};

template<typename ...>
struct Implements;

template<typename Head, typename ...Tail>
struct Implements<Head, Tail...>
{
    template<typename U>
    static constexpr void register_upcast()
    {
        if(!Head::Base::upcast.count(U::cid)) Head::Base::upcast[U::cid] = [](void *ptr) -> std::unique_ptr<typename Head::Ifac> {
            return std::make_unique<Head>(static_cast<typename U::Wrapped &>(reinterpret_cast<U*>(ptr)->ref()));
        };
        Implements<Tail...>::template register_upcast<U>();
    }
};

template<>
struct Implements<>
{
    template<typename U>
    static constexpr void register_upcast() {}
};

template<typename Impl>
int del_property(JSContext *ctx, JSValueConst self, JSAtom prop)
{
    int result = 0;
    if(const char *name = JS_AtomToCString(ctx, prop); name)
    {
        JSPropertyDescriptor desc;
        if(JS_GetOwnProperty(ctx, &desc, Impl::proto, prop))
        {
            if(!JS_IsUndefined(desc.getter)) JS_FreeValue(ctx, desc.getter);
            if(!JS_IsUndefined(desc.setter)) JS_FreeValue(ctx, desc.setter);
            if(!JS_IsUndefined(desc.value)) JS_FreeValue(ctx, desc.value);
        }
        else
        {
            reinterpret_cast<Impl *>(JS_GetOpaque(self, Impl::cid))->del_property(ctx, name);
            result = 1;
        }
        JS_FreeCString(ctx, name);
    }
    return result;
}

template<typename Impl>
int has_property(JSContext *ctx, JSValueConst self, JSAtom prop)
{
    int result = -1;
    if(const char *name = JS_AtomToCString(ctx, prop); name)
    {
        JSPropertyDescriptor desc;
        if(JS_GetOwnProperty(ctx, &desc, Impl::proto, prop))
        {
            if(!JS_IsUndefined(desc.getter)) JS_FreeValue(ctx, desc.getter);
            if(!JS_IsUndefined(desc.setter)) JS_FreeValue(ctx, desc.setter);
            if(!JS_IsUndefined(desc.value)) JS_FreeValue(ctx, desc.value);
            result = 1;
        }
        else
        {
            result = reinterpret_cast<Impl *>(JS_GetOpaque(self, Impl::cid))->has_property(ctx, name) ? 1 : 0;
        }
        JS_FreeCString(ctx, name);
    }
    return result;
}

template<typename Impl>
JSValue get_property(JSContext *ctx, JSValueConst self, JSAtom prop, JSValueConst receiver)
{
    JSValue value = JS_UNDEFINED;
    if(const char *name = JS_AtomToCString(ctx, prop); name)
    {
        JSPropertyDescriptor desc;
        if(JS_GetOwnProperty(ctx, &desc, Impl::proto, prop))
        {
            if(!JS_IsUndefined(desc.getter))
            {
                value = JS_Call(ctx, desc.getter, receiver, 0, NULL);
                JS_FreeValue(ctx, desc.getter);
            }
            if(!JS_IsUndefined(desc.setter)) JS_FreeValue(ctx, desc.setter);
            if(!JS_IsUndefined(desc.value))
            {
                if(JS_IsUndefined(value)) value = JS_DupValue(ctx, desc.value);
                JS_FreeValue(ctx, desc.value);
            }
        }
        else
        {
            value = reinterpret_cast<Impl const *>(JS_GetOpaque(self, Impl::cid))->get_property(ctx, name);
        }
        JS_FreeCString(ctx, name);
    }
    return value;
}

template<typename Impl>
int own_properties(JSContext *ctx, JSPropertyEnum **ptab, uint32_t *plen, JSValueConst self)
{
    *ptab = nullptr;
    *plen = 0;

    auto const names = reinterpret_cast<Impl *>(JS_GetOpaque(self, Impl::cid))->own_properties();
    if(names.empty()) return 0;

    JSPropertyEnum *tab = static_cast<JSPropertyEnum *>(js_mallocz(ctx, sizeof(JSPropertyEnum) * names.size()));
    if(!tab) return -1;

    for(uint32_t i = 0; i < names.size(); ++i)
    {
        tab[i].atom = JS_NewAtom(ctx, names[i].c_str());
        if(JS_ATOM_NULL == tab[i].atom)
        {
            for (uint32_t j = 0; j < i; ++j)
                JS_FreeAtom(ctx, tab[j].atom);

            js_free(ctx, tab);
            return -1;
        }
        tab[i].is_enumerable = 1;
    }

    *ptab = tab;
    *plen = static_cast<uint32_t>(names.size());
    return 0;
}

template<typename Impl>
int own_property(JSContext *ctx, JSPropertyDescriptor *desc, JSValueConst self, JSAtom prop)
{
    int result = -1;
    if(const char *name = JS_AtomToCString(ctx, prop); name)
    {
        result = reinterpret_cast<Impl *>(JS_GetOpaque(self, Impl::cid))->own_property(ctx, name, desc) ? 1 : 0;
        JS_FreeCString(ctx, name);
    }
    return result;
}

template<typename Impl>
int set_property(JSContext *ctx, JSValueConst self, JSAtom prop, JSValueConst value, JSValueConst receiver, int flags)
{
    int result = -1;
    if(const char *name = JS_AtomToCString(ctx, prop); name)
    {
        JSPropertyDescriptor desc;
        if(JS_GetOwnProperty(ctx, &desc, Impl::proto, prop))
        {
            if(!JS_IsUndefined(desc.getter)) JS_FreeValue(ctx, desc.getter);
            if(!JS_IsUndefined(desc.setter))
            {
                JS_FreeValue(ctx, JS_Call(ctx, desc.setter, receiver, 1, &value));
                JS_FreeValue(ctx, desc.setter);
            }
            if(!JS_IsUndefined(desc.value)) JS_FreeValue(ctx, desc.value);
            result = 0;
        }
        else
        {
            reinterpret_cast<Impl *>(JS_GetOpaque(self, Impl::cid))->set_property(ctx, name, value);
            result = 1;
        }
        JS_FreeCString(ctx, name);
    }
    return result;
}

template<typename Impl, typename Native_ = void, typename Extends_ = void>
class Interface
{
    static constexpr bool Pointer = std::is_pointer_v<Native_>;
public:
    using Base = Interface<Impl, Native_, Extends_>;
    using Native = std::conditional_t<
        Pointer, Native_, std::variant<Native_, std::reference_wrapper<Native_>>
    >;
    using Wrapped = Native_;

    Interface() = default;

    Interface(Interface &&) = default;
    Interface(Interface const &) = delete;

    Interface& operator = (Interface &&) = delete;
    Interface& operator = (Interface const &) = delete;

    using ctor = Constructor
    <
        Impl()
    >;

    using priv = Private<>;
    using impl = Implements<>;

    static JSCFunctionListEntry const funcs[];
    static JSCFunctionListEntry const sfunc[];
    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
    {
        JSClassID const cid = JS_GetClassID(*value);
        return Base::cid == cid || Base::upcast.find(cid) != std::end(Base::upcast);
    }
    static constexpr bool valid(JSContext *ctx, JSValue *value, std::string &message)
    {
        return true;
    }

    BOOST_FORCEINLINE static JSValue from(JSContext *ctx, Native_ &native, JSValue alloc)
    {
        JSValue result = JS_NewObjectProtoClass(ctx, proto, cid);
        JS_SetOpaque(result, new Base(std::ref(native)));
        detail::set_allocator(ctx, result, alloc);
        return result;
    }

    template<bool P = Pointer>
    BOOST_FORCEINLINE static auto from(JSContext *ctx, Native_ &native, std::monostate const &)
    -> std::enable_if_t<!P, JSValue>
    {
        JSValue result = JS_NewObjectProtoClass(ctx, proto, cid);
        JS_SetOpaque(result, new Base(std::ref(native)));
        return result;
    }

    template<bool P = Pointer>
    BOOST_FORCEINLINE static auto from(JSContext *ctx, Native_ &&native)
    -> std::enable_if_t<!P, JSValue>
    {
        JSValue result = JS_NewObjectProtoClass(ctx, proto, cid);
        JS_SetOpaque(result, new Base(std::move(native), std::true_type{}));
        return result;
    }

    BOOST_FORCEINLINE static auto from(JSContext *ctx, Native_ &&native, JSValue alloc)
    {
        JSValue result = JS_NewObjectProtoClass(ctx, proto, cid);
        if constexpr (Pointer) JS_SetOpaque(result, new Base(native));
        else JS_SetOpaque(result, new Base(std::move(native), std::true_type{}));
        detail::set_allocator(ctx, result, alloc);
        return result;
    }

    static JSClassID cid;
    static JSClassDef def;
    static thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> upcast;

    template<typename U>
    BOOST_FORCEINLINE static void register_upcast()
    {
        upcast[U::cid] = [](void *ptr) -> Impl {
            return Impl(std::ref(static_cast<Wrapped &>(reinterpret_cast<U*>(ptr)->ref())));
        };
        Impl::impl::template register_upcast<U>();
        if constexpr (!std::is_same_v<Extends_, void>) Extends_::template register_upcast<U>();
    }

    static void init();
    static void init(JSRuntime *);
    static void init(JSContext *);
    static void init(JSContext *, JSValue);
    static void init(JSContext *, JSModuleDef *);
    static void alias(JSContext *, JSValue, const char *);

    static char const *name();

    template<typename Im, typename If>
    struct I : If
    {
    public:
        I(Native_ &ref): ref{ref} {}
        using Base = I<Im, If>;

    protected:
        Native_ &ref;
    };

protected:
    Interface(Impl &&ref) : native{std::move(ref.ref())} {}
    Interface(Native_ &&ref) : native{std::move(ref)} {}
    Interface(Native_ &&ref, std::true_type) : native{std::move(ref)} { script.set(); }
    Interface(std::reference_wrapper<Native_> &&ref) : native{std::move(ref)} { script.set(); }

    BOOST_FORCEINLINE auto ref() -> std::remove_pointer_t<Native_> &
    {
        if constexpr (Pointer) return *native;
        else return std::holds_alternative<Native_>(native)
            ? std::get<Native_>(native)
            : std::get<std::reference_wrapper<Native_>>(native).get();
    }
    BOOST_FORCEINLINE auto ref() const -> std::remove_pointer_t<Native_> const &
    {
        if constexpr (Pointer) return *native;
        else return std::holds_alternative<Native_>(native)
            ? std::get<Native_>(native)
            : std::get<std::reference_wrapper<Native_>>(native).get();
    }
    BOOST_FORCEINLINE auto ptr() -> std::remove_pointer_t<Native_> *
    {
        if constexpr (Pointer) return native;
        else return &ref();
    }
    BOOST_FORCEINLINE auto ptr() const -> std::conditional_t<!Pointer, Native_ const *, Native_ const>
    {
        if constexpr (Pointer) return native;
        else return &ref();
    }

    BOOST_FORCEINLINE static Wrapped &get(JSValue self)
    {
        if(JSClassID const cid = JS_GetClassID(self); Base::cid == cid)
            return reinterpret_cast<Impl*>(JS_GetOpaque(self, cid))->ref();
        else
            return upcast.find(cid)->second(JS_GetOpaque(self, cid)).ref();
    }
    BOOST_FORCEINLINE Base *release()
    {
        if(script)
        {
            if constexpr (!Pointer) Native{std::in_place_type_t<Native_>{}, ref()}.swap(native);
            script = std::nullopt;
        }
        return this;
    }

    Interface(JSContext *ctx, JSValue val)
    : native(std::in_place_type_t<std::reference_wrapper<Native_>>{}, get(val))
    , script(std::in_place_t{}, ctx, val)
    {}

private:
    static JSValue make(JSContext *);
    static JSValue make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv);

    Native native;
    detail::optional<detail::Strong<void>> script = std::nullopt;

    template<typename ...>
    friend struct Constructor;

    template<typename ...>
    friend struct Implements;

    template<typename, typename, typename>
    friend class Interface;

    friend struct detail::Strong<Impl, true>;

    friend JSValue get_property<Impl>(JSContext *, JSValueConst, JSAtom, JSValueConst);
    friend int own_properties<Impl>(JSContext *, JSPropertyEnum **, uint32_t *, JSValueConst);
    friend int own_property<Impl>(JSContext *, JSPropertyDescriptor *, JSValueConst, JSAtom);
    friend int set_property<Impl>(JSContext *, JSValueConst, JSAtom, JSValueConst, JSValueConst, int);
    friend int has_property<Impl>(JSContext *, JSValueConst, JSAtom);
    friend int del_property<Impl>(JSContext *, JSValueConst, JSAtom);

    static thread_local JSValue proto; // bound to context
public:
    static void free(JSRuntime *, JSValue);

    template<typename P>
    static void invoke_free(P &object)
    {
        using N = std::variant_alternative_t<0, P>;
        if constexpr (detail::has_deleter<Impl, N>::value) {
            if(!object.index()) Impl::free(std::get<0>(object));
        }
        else if constexpr (!std::is_same_v<Extends_, void>) {
            Extends_::template invoke_free<P>(object);
        }
    }

    ~Interface()
    {
        if constexpr (!Pointer) invoke_free(native);
    }
};

template<typename Impl, typename Native_, typename Extends_>
JSClassID Interface<Impl, Native_, Extends_>::cid;

template<typename Impl, typename Native_, typename Extends_>
thread_local JSValue Interface<Impl, Native_, Extends_>::proto;

template<typename Impl, typename Native_, typename Extends_>
thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> Interface<Impl, Native_, Extends_>::upcast;

template<typename Impl, typename Native_, typename Extends_>
JSClassDef Interface<Impl, Native_, Extends_>::def = {
    .class_name = Base::name(),
    .finalizer = Base::free,
    .exotic = detail::get_exotic<Impl>::get()
};

template<typename Impl, typename Native_, typename Extends_>
JSCFunctionListEntry const Interface<Impl, Native_, Extends_>::funcs[] = {};

template<typename Impl, typename Native_, typename Extends_>
JSCFunctionListEntry const Interface<Impl, Native_, Extends_>::sfunc[] = {};

template<typename Impl, typename Native_, typename Extends_>
char const *Interface<Impl, Native_, Extends_>::name()
{
    static_assert(sizeof(Base) == sizeof(Impl));
    static std::string name = boost::core::demangle(typeid(Impl).name());

    if(auto const tmpl = name.rfind('>'); tmpl > name.rfind(':') && tmpl != std::string::npos)
        name[tmpl] = '\0';

    return name.c_str() + name.rfind(':') + 1;
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::free(JSRuntime *rt, JSValue self)
{
    if(auto *ptr = reinterpret_cast<Base*>(JS_GetOpaque(self, cid)); ptr->script.get()) delete ptr;
    else delete static_cast<Impl*>(ptr);
}

template<typename Impl, typename Native_, typename Extends_>
JSValue Interface<Impl, Native_, Extends_>::make(JSContext *ctx)
{
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, Impl::funcs, sizeof(Impl::funcs)/sizeof(Impl::funcs[0]));

    JSValue ctor = JS_NewCFunction2(ctx, &make, name(), 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    for(std::size_t i = 0; i < sizeof(Impl::sfunc) / sizeof(Impl::sfunc[0]); ++i)
        JS_SetPropertyStr(ctx, ctor, Impl::sfunc[i].name, JS_NewCFunction(ctx, Impl::sfunc[i].u.func.cfunc.generic, Impl::sfunc[i].name, Impl::sfunc[i].u.func.length));

    Impl::impl::template register_upcast<Base>();
    if constexpr (!std::is_same_v<Extends_, void>)
    {
        JS_SetPrototype(ctx, proto, Extends_::proto);

        JSValue base = JS_GetPropertyStr(ctx, Extends_::proto, "constructor");
        JS_SetPrototype(ctx, ctor, base);
        JS_FreeValue(ctx, base);

        Extends_::template register_upcast<Base>();
    }

    JS_SetClassProto(ctx, cid, proto);
    Impl::priv::make(ctx);
    return ctor;
}

template<typename Impl, typename Native_, typename Extends_>
JSValue Interface<Impl, Native_, Extends_>::make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
{
    return Impl::ctor::invoke(ctx, clazz, argc, argv);
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::init()
{
    JS_NewClassID(&cid);
    Impl::priv::template init<Base>();
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::init(JSRuntime *rt)
{
    JS_NewClass(rt, cid, &Base::def);
    Impl::priv::init(rt);
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::init(JSContext *ctx)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_FreeValue(ctx, make(ctx));
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::init(JSContext *ctx, JSValue glob)
{
    JS_SetPropertyStr(ctx, glob, name(), make(ctx));
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::init(JSContext *ctx, JSModuleDef *glob)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_SetModuleExport(ctx, glob, name(), make(ctx));
}

template<typename Impl, typename Native_, typename Extends_>
void Interface<Impl, Native_, Extends_>::alias(JSContext *ctx, JSValue target, const char *name)
{
    JS_SetPropertyStr(ctx, target, name, JS_GetPropertyStr(ctx, proto, "constructor"));
}

template<typename Impl>
struct Interface<Impl, void, void> : Object
{
    using Object::Object;
    using Base = Interface<Impl, void, void>;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
    {
        return JS_IsObject(*value) && cid == JS_GetClassID(*value);
    }

    static JSCFunctionListEntry const funcs[];

    static JSClassID cid;
    static JSClassDef def;
    static thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> upcast;

    static void init();
    static void init(JSRuntime *);
    static void init(JSContext *);
    static void init(JSContext *, JSValue);
    static void init(JSContext *, JSModuleDef *);
    static void alias(JSContext *, JSModuleDef *, const char *name = nullptr);

    static char const *name();
    static JSValue ctor(JSContext *);
    static JSValue ctor(JSContext *, Object obj);
    static JSValue data(JSContext *, JSValue data);
    static constexpr bool constructible = true;

private:
    Interface() = delete;
    static thread_local JSValue proto; // bound to context

private:
    static JSValue make(JSContext *);
    static JSValue make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv);
};

template<typename Impl>
JSClassID Interface<Impl, void, void>::cid;

template<typename Impl>
thread_local JSValue Interface<Impl, void, void>::proto;

template<typename Impl>
thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> Interface<Impl, void, void>::upcast;

template<typename Impl>
JSClassDef Interface<Impl, void, void>::def = {
    .class_name = Interface<Impl, void, void>::name(),
    .finalizer = NULL
};

template<typename Impl>
JSCFunctionListEntry const Interface<Impl, void, void>::funcs[] = {};

template<typename Impl>
char const *Interface<Impl, void, void>::name()
{
    static std::string name = boost::core::demangle(typeid(Impl).name());
    return name.c_str() + name.rfind(':') + 1;
}

template<typename Impl>
void Interface<Impl, void, void>::init()
{
    JS_NewClassID(&cid);
}

template<typename Impl>
void Interface<Impl, void, void>::init(JSRuntime *rt)
{
    JS_NewClass(rt, cid, &Interface<Impl, void, void>::def);
}

template<typename Impl>
void Interface<Impl, void, void>::init(JSContext *ctx, JSValue glob)
{
    JS_SetPropertyStr(ctx, glob, name(), make(ctx));
}

template<typename Impl>
void Interface<Impl, void, void>::init(JSContext *ctx)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_FreeValue(ctx, make(ctx));
}

template<typename Impl>
void Interface<Impl, void, void>::init(JSContext *ctx, JSModuleDef *glob)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_SetModuleExport(ctx, glob, name(), make(ctx));
}

template<typename Impl>
void Interface<Impl, void, void>::alias(JSContext *ctx, JSModuleDef *glob, const char *name)
{
    JS_SetModuleExport(ctx, glob, name ? name : Interface<Impl, void, void>::name(), JS_GetPropertyStr(ctx, proto, "constructor"));
}

template<typename Impl>
JSValue Interface<Impl, void, void>::make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
{
    if constexpr (Impl::constructible)
        if(0 == argc) return Interface<Impl, void, void>::ctor(ctx);
    if(1 == argc && Object::check(ctx, argv))
    {
        if(auto p = Object{ctx, argv[0]}.get<String>("data"))
        {
            JSValue self = Interface<Impl, void, void>::ctor(ctx);
            JS_SetPropertyStr(ctx, self, "data", p->release());
            return self;
        }
    }
    return JS_ThrowTypeError(ctx, "%s: no matching constructor found", Interface<Impl>::name());
}

template<typename Impl>
JSValue Interface<Impl, void, void>::data(JSContext *ctx, JSValue data)
{
    JSValue self = Interface<Impl, void, void>::ctor(ctx);
    JS_SetPropertyStr(ctx, self, "data", data);
    return self;
}

template<typename Impl>
JSValue Interface<Impl, void, void>::make(JSContext *ctx)
{
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, Impl::funcs, sizeof(Impl::funcs)/sizeof(Impl::funcs[0]));

    JSValue ctor = JS_NewCFunction2(ctx, &make, name(), 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    JS_SetClassProto(ctx, cid, proto);
    return ctor;
}

template<typename Impl>
JSValue Interface<Impl, void, void>::ctor(JSContext *ctx)
{
    return JS_NewObjectProtoClass(ctx, Impl::proto, Impl::cid);
}

template<typename If>
struct Interface<If, void*, void>
{
    using Ifac = If;
    struct Impl : If::Static
    {
        BOOST_FORCEINLINE If * operator -> () { return value.get(); }
        BOOST_FORCEINLINE If const * operator -> () const { return value.get(); }

        Impl(JSContext *ctx, JSValue val)
        {
            JSClassID const cid = JS_GetClassID(val);
            value = upcast[cid](JS_GetOpaque(val, cid));
        }
        BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
        {
            JSClassID const cid = JS_GetClassID(*value);
            return upcast.find(cid) != std::end(upcast);
        }
        static constexpr bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            return true;
        }
    private:
        std::unique_ptr<If> value;
    };

    struct Static {};
    static std::unordered_map<JSClassID, std::function<std::unique_ptr<If>(void*)>> upcast;
};

template<typename If>
std::unordered_map<JSClassID, std::function<std::unique_ptr<If>(void*)>> Interface<If, void*, void>::upcast;

template<typename Impl>
class Exception
{
public:
    static JSClassID cid;
    static JSClassDef def;

    static void init();
    static void init(JSRuntime *);
    static void init(JSContext *);
    static void init(JSContext *, JSValue);
    static void init(JSContext *, JSModuleDef *);

    static char const *name();
    static JSValue throw_(JSContext *, Impl &&impl);
    static JSValue reject_(JSContext *, Impl &&impl);

protected:
    Exception() {}
    static thread_local JSValue proto; // bound to context

private:
    static JSValue make(JSContext *);
    static JSValue make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv);
};

template<typename Impl>
JSClassID Exception<Impl>::cid;

template<typename Impl>
thread_local JSValue Exception<Impl>::proto;

template<typename Impl>
JSClassDef Exception<Impl>::def = {
    .class_name = Exception<Impl>::name(),
    .finalizer = NULL
};

template<typename Impl>
char const *Exception<Impl>::name()
{
    static std::string name = boost::core::demangle(typeid(Impl).name());
    return name.c_str() + name.rfind(':') + 1;
}

template<typename Impl>
void Exception<Impl>::init()
{
    JS_NewClassID(&cid);
}

template<typename Impl>
void Exception<Impl>::init(JSRuntime *rt)
{
    JS_NewClass(rt, cid, &Exception<Impl>::def);
}

template<typename Impl>
void Exception<Impl>::init(JSContext *ctx, JSValue glob)
{
    JS_SetPropertyStr(ctx, glob, name(), make(ctx));
}

template<typename Impl>
void Exception<Impl>::init(JSContext *ctx)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_FreeValue(ctx, make(ctx));
}

template<typename Impl>
void Exception<Impl>::init(JSContext *ctx, JSModuleDef *glob)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_SetModuleExport(ctx, glob, name(), make(ctx));
}

template<typename Impl>
JSValue Exception<Impl>::make(JSContext *ctx)
{
    proto = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, proto, "name", JS_NewString(ctx, name()), JS_PROP_C_W_E);

    JSValue ctor = JS_NewCFunction2(ctx, &make, name(), 1, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue error = JS_GetPropertyStr(ctx, glob, "Error");
    JSValue error_p = JS_GetPropertyStr(ctx, error, "prototype");

    JS_SetPrototype(ctx, proto, error_p);

    JS_FreeValue(ctx, error_p);
    JS_FreeValue(ctx, error);
    JS_FreeValue(ctx, glob);

    JS_SetClassProto(ctx, cid, proto);
    return ctor;
}

template<typename Impl>
JSValue Exception<Impl>::reject_(JSContext *ctx, Impl &&impl)
{
    bridge::Object obj{ctx, make(ctx, JS_UNDEFINED, 0, NULL)};
    impl.populate(ctx, obj);
    return obj;
}

template<typename Impl>
JSValue Exception<Impl>::throw_(JSContext *ctx, Impl &&impl)
{
    bridge::Object obj{ctx, make(ctx, JS_UNDEFINED, 0, NULL)};
    impl.populate(ctx, obj);
    return JS_Throw(ctx, obj);
}

template<typename Impl>
JSValue Exception<Impl>::make(JSContext *ctx, JSValueConst clazz, int argc, JSValueConst *argv)
{
    if(argc == 0)
    {
        JSValue v = JS_NewError(ctx);
        JS_SetPrototype(ctx, v, Impl::proto);
        return v;
    }
    else if(argc == 1 && Object::check(ctx, argv))
    {
        JSValue v = JS_NewError(ctx);
        JS_SetPrototype(ctx, v, Impl::proto);
        for(auto it = Object{ctx, argv[0]}.begin(); it; ++it)
        {
            JS_SetProperty(ctx, v, it.atom(), JS_DupValue(ctx, *it));
        }
        return v;
    }
    return JS_ThrowTypeError(ctx, "%s: no matching constructor found", Interface<Impl>::name());
}

template<typename Wrapped>
class Iterator
{
public:
    static JSClassID cid;
    static JSClassDef def;

public:
    static void free(JSRuntime *, JSValue);
    static JSCFunctionListEntry const funcs[];

    BOOST_FORCEINLINE static JSValue make(JSContext *ctx, JSValue owner, Wrapped &&i, Wrapped &&e)
    {
        JSValue result = JS_NewObjectProtoClass(ctx, proto, cid);
        JS_SetOpaque(result, new Iterator(ctx, owner, std::move(i), std::move(e)));
        return result;
    }
    static JSValue next(JSContext *ctx, JSValueConst, int, JSValueConst *);
    static JSValue sym(JSContext *ctx, JSValueConst, int, JSValueConst *);
private:
    BOOST_FORCEINLINE Iterator(JSContext *ctx, JSValue owner, Wrapped &&i, Wrapped &&e)
    : owner{ctx, owner}, i{std::move(i)}, e{std::move(e)} {}

    detail::Strong<void> owner;
    Wrapped i;
    Wrapped const e;

private:
    static thread_local JSValue proto;
    BOOST_FORCEINLINE static Iterator<Wrapped> *get(JSValue self)
    {
        return reinterpret_cast<Iterator<Wrapped>*>(JS_GetOpaque(self, cid));
    }

    template<typename ...>
    friend struct Private;
};

template<typename Wrapped>
JSClassID Iterator<Wrapped>::cid;

template<typename Wrapped>
thread_local JSValue Iterator<Wrapped>::proto;

template<typename Wrapped>
JSClassDef Iterator<Wrapped>::def = {
    .class_name = nullptr,
    .finalizer = Iterator<Wrapped>::free
};

template<typename Wrapped>
JSCFunctionListEntry const Iterator<Wrapped>::funcs[] = {
    JS_CFUNC_DEF("next", 0, &Iterator<Wrapped>::next),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &Iterator<Wrapped>::sym)
};

template<typename Wrapped>
JSValue Iterator<Wrapped>::next(JSContext *ctx, JSValueConst self, int, JSValueConst *)
{
    Iterator<Wrapped> *s = reinterpret_cast<Iterator<Wrapped>*>(JS_GetOpaque(self, Iterator<Wrapped>::cid));
    JSValue obj = JS_NewObject(ctx);
    if(s->e == s->i)
    {
        JS_SetPropertyStr(ctx, obj, "value", JS_UNDEFINED);
        JS_SetPropertyStr(ctx, obj, "done", JS_NewBool(ctx, 1));
    }
    else
    {
        JS_SetPropertyStr(ctx, obj, "value", s->i.get(ctx));
        JS_SetPropertyStr(ctx, obj, "done", JS_NewBool(ctx, 0));
        ++s->i;
    }
    return obj;
}

template<typename Wrapped>
JSValue Iterator<Wrapped>::sym(JSContext *ctx, JSValueConst self, int, JSValueConst *)
{
    return JS_DupValue(ctx, self);
}

template<typename Wrapped>
void Iterator<Wrapped>::free(JSRuntime *rt, JSValue self)
{
    delete get(self);
}

template<typename Impl>
struct JSON
{
    JSON() = delete;
    static JSValue toJSON(JSContext *, JSValueConst, int argcc = 0, JSValueConst *argv = NULL);
};

template<typename Impl>
JSValue JSON<Impl>::toJSON(JSContext *ctx, JSValueConst self, int, JSValueConst *)
{
    if constexpr (detail::has_constructor<Impl>::value)
    {
        if(JSClassID const cid = JS_GetClassID(self); Impl::cid == cid)
            return reinterpret_cast<Impl const *>(JS_GetOpaque(self, cid))->toJSON(ctx);
        else if(auto it = Impl::upcast.find(cid); it != std::end(Impl::upcast))
            return it->second(JS_GetOpaque(self, cid)).Impl::toJSON(ctx);
        return JS_NULL;
    }
    else
    {
        return Impl{ctx, self}.toJSON(ctx);
    }
}

} // namespace bridge
