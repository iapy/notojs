#include <bridge.hpp>
namespace {

struct A_
{
    std::string a{"init"};
};

struct B;
struct C;
struct A : bridge::Interface<A, A_>
{
    A() = default;
    A(JSContext *ctx, JSValue val) : Base{ctx, val} {}
    A(std::reference_wrapper<A_> &&rw) : Base(std::move(rw)) {}

    JSValue get_a(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().a);
    }

    void set_a(JSContext *ctx, bridge::String a)
    {
        ref().a = static_cast<std::string>(a);
    }

    JSValue add(JSContext *ctx, bridge::String s)
    {
        ref().a.append(s);
        return JS_UNDEFINED;
    }

    JSValue set(JSContext *ctx, bridge::String s, bridge::Number n)
    {
        std::string value;
        for(std::int32_t i = 0; i < static_cast<std::int32_t>(n); ++i) value.append(s);
        ref().a = value;
        return JS_UNDEFINED;
    }

    friend struct B;
    friend struct C;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const A::funcs[] = {
    JS_CGETSET_DEF("a", &bridge::Getter<&A::get_a>, &bridge::Setter<&A::set_a>),
    JS_CFUNC_DEF("set", 2, &bridge::Function<&A::set>::invoke),
    JS_CFUNC_DEF("add", 1, &bridge::Function<&A::add>::invoke)
};

struct B_ : A_
{
    std::int32_t b{42};
};

struct B : bridge::Interface<B, B_, A>
{
    B() = default;
    B(std::reference_wrapper<B_> &&rw) : Base(std::move(rw)) {}

    JSValue get_b(JSContext *ctx) const
    {
        return bridge::Number(ctx, ref().b);
    }

    void set_b(JSContext *ctx, bridge::Number b)
    {
        ref().b = static_cast<std::int32_t>(b);
    }

    JSValue add(JSContext *ctx, bridge::String s)
    {
        ref().a.append(s);
        ref().a.append(s);
        return JS_UNDEFINED;
    }

    friend struct C;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const B::funcs[] = {
    JS_CGETSET_DEF("b", &bridge::Getter<&B::get_b>, &bridge::Setter<&B::set_b>),
    JS_CFUNC_DEF("add", 1, &bridge::Function<&B::add>::invoke)
};

struct C_ : B_
{
    std::int32_t c{0};
};

struct C : bridge::Interface<C, C_, B>
{
    C() = default;

    JSValue get_c(JSContext *ctx) const
    {
        return bridge::Number(ctx, ref().c);
    }

    void set_c(JSContext *ctx, bridge::Number c)
    {
        ref().c = static_cast<std::int32_t>(c);
    }

    JSValue add(JSContext *ctx, bridge::String s)
    {
        ref().a.append(s);
        ref().a.append(s);
        ref().a.append(s);
        return JS_UNDEFINED;
    }

    JSValue adopt(JSContext *ctx, A a)
    {
        ref().a = a.ref().a;
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const C::funcs[] = {
    JS_CGETSET_DEF("c", &bridge::Getter<&C::get_c>, &bridge::Setter<&C::set_c>),
    JS_CFUNC_DEF("adopt", 1, &bridge::Function<&C::adopt>::invoke),
    JS_CFUNC_DEF("add", 1, &bridge::Function<&C::add>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    A::init(ctx, m);
    B::init(ctx, m);
    C::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, A::name());
    JS_AddModuleExport(ctx, mod, B::name());
    JS_AddModuleExport(ctx, mod, C::name());
    return mod;
}

} // extern "C"
