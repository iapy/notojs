#include <bridge.hpp>
namespace {

struct Variant : bridge::Interface<Variant, std::string>
{
    Variant() = default;

    Variant(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Variant(bridge::String string)
    : Base{static_cast<std::string>(string)}
    {}

    using ctor = bridge::Constructor<
        Variant(),
        Variant(Variant),
        Variant(bridge::String)
    >;

    JSValue get(JSContext *ctx) const
    {
        return bridge::String(ctx, ref());
    }

    void set_string(JSContext *ctx, bridge::String value)
    {
        ref() = static_cast<std::string>(value);
    }

    void set_number(JSContext *ctx, bridge::Number value)
    {
        ref() = std::to_string(static_cast<std::int32_t>(value));
    }

    void set_variant(JSContext *ctx, Variant value)
    {
        if(ptr() != value.ptr()) ref() = value.ref();
    }

    using value = bridge::Setters<
        &Variant::set_number,
        &Variant::set_string,
        &Variant::set_variant
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Variant::funcs[] = {
    JS_CGETSET_DEF("value", &bridge::Getter<&Variant::get>, &value::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Variant::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Variant::name());
    return mod;
}

} // extern "C"
