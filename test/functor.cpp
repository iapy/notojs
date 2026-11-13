#include <bridge.hpp>
namespace {

struct Functor : bridge::Interface<Functor, std::int64_t>
{
    Functor() = default;

    Functor(bridge::Number number)
    : Base{static_cast<std::int64_t>(number)}
    {}

    using ctor = bridge::Constructor
    <
        Functor(),
        Functor(bridge::Number)
    >;

    JSValue call(JSContext *ctx, bridge::Number n)
    {
        return bridge::Number(ctx, ref() + static_cast<std::int64_t>(n));
    }

    JSValue operator () (JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
    {
        return bridge::Function<&Functor::call>::invoke(ctx, self, argc, argv);
    }

    JSValue get_value(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref()};
    }

    void set_value(JSContext *ctx, bridge::Number number)
    {
        ref() = static_cast<std::int64_t>(number);
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Functor::funcs[] = {
    JS_CGETSET_DEF("value", &bridge::Getter<&Functor::get_value>, &bridge::Setter<&Functor::set_value>),
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Functor::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Functor::name());
    return mod;
}

} // extern "C"
