#include <bridge.hpp>
#include <vector>

namespace {

struct Vector : bridge::Interface<Vector, std::vector<std::string>>
{
    Vector() = default;
    Vector(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Vector(std::reference_wrapper<std::vector<std::string>> &&rw) : Base(std::move(rw)) {}

    friend class Mixin;
    static JSCFunctionListEntry const funcs[];
};

struct Derived_ : std::vector<std::string>
{
};

struct Derived : bridge::Interface<Derived, Derived_, Vector>
{
    friend class Mixin;
};

struct Mixin
{
    static JSValue append(Vector &v, JSContext *ctx, bridge::Tail<1, bridge::String, bridge::Number> tail)
    {
        for(std::size_t i = 0; i < tail.size(); ++i)
        {
            if(auto s = tail.get<bridge::String>(i))
                v.ref().push_back(static_cast<std::string>(*s));
            else if(auto n = tail.get<bridge::Number>(i))
                v.ref().push_back(std::to_string(static_cast<std::int32_t>(*n)));
        }
        return JS_UNDEFINED;
    }

    static JSValue count(Derived const &v, JSContext *ctx, bridge::String value)
    {
        std::uint64_t n{0};
        for(auto const &w: v.ref())
            n += (w == static_cast<std::string_view const &>(value));
        return bridge::Number{ctx, n};
    }

    static JSValue size(Vector const &v, JSContext *ctx)
    {
        return bridge::Number{ctx, static_cast<std::uint64_t>(v.ref().size())};
    }

    static JSValue get_first(Derived const &v, JSContext *ctx)
    {
        if(v.ref().empty()) return JS_UNDEFINED;
        return bridge::String(ctx, v.ref().front());
    }

    static void set_first_0(Derived &v, JSContext *ctx, bridge::String value)
    {
        if(v.ref().empty()) v.ref().push_back(value);
        v.ref().front() = value;
    }

    static void set_first_1(Derived &v, JSContext *ctx, bridge::Number value)
    {
        if(v.ref().empty()) v.ref().push_back(std::to_string(static_cast<std::int32_t>(value)));
        v.ref().front() = std::to_string(static_cast<std::int32_t>(value));
    }

    using set_first = bridge::Setters<&set_first_0, &set_first_1>;

    static JSValue get_last(Vector const &v, JSContext *ctx)
    {
        if(v.ref().empty()) return JS_UNDEFINED;
        return bridge::String(ctx, v.ref().back());
    }

    static void set_last_0(Vector &v, JSContext *ctx, bridge::String value)
    {
        if(v.ref().empty()) v.ref().push_back(value);
        v.ref().back() = value;
    }

    static void set_last_1(Vector &v, JSContext *ctx, bridge::Number value)
    {
        if(v.ref().empty()) v.ref().push_back(std::to_string(static_cast<std::int32_t>(value)));
        v.ref().back() = std::to_string(static_cast<std::int32_t>(value));
    }

    using set_last = bridge::Setters<&set_last_0, &set_last_1>;
};

JSCFunctionListEntry const Vector::funcs[] = {
    JS_CGETSET_DEF("first", &bridge::Getter<&Mixin::get_first>, &Mixin::set_first::invoke),
    JS_CGETSET_DEF("last", &bridge::Getter<&Mixin::get_last>, &Mixin::set_last::invoke),
    JS_CFUNC_DEF("append", 1, &bridge::Function<&Mixin::append>::invoke),
    JS_CFUNC_DEF("count", 1, &bridge::Function<&Mixin::count>::invoke),
    JS_CFUNC_DEF("size", 1, &bridge::Function<&Mixin::size>::invoke),
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Vector::init(ctx, m);
    Derived::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Vector::name());
    JS_AddModuleExport(ctx, mod, Derived::name());
    return mod;
}

} // extern "C"
