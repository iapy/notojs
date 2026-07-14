#include <bridge.hpp>
namespace {

struct Variant : bridge::Interface<Variant, std::variant<int, std::string>>
{
    Variant() = default;

    Variant(bridge::Number number)
    : Base{static_cast<int>(number)}
    {}

    Variant(bridge::String string)
    : Base{static_cast<std::string>(string)}
    {}

    using ctor = bridge::Constructor
    <
        Variant(),
        Variant(bridge::Number),
        Variant(bridge::String)
    >;

    JSValue get_i(JSContext *ctx) const
    {
        return std::holds_alternative<int>(ref())
            ? bridge::Number(ctx, std::get<0>(ref()))
            : JS_UNDEFINED;
    }

    void set_i(JSContext *ctx, bridge::Number number)
    {
        ref() = static_cast<int>(number);
    }

    JSValue get_s(JSContext *ctx) const
    {
        return std::holds_alternative<std::string>(ref())
            ? bridge::String(ctx, std::get<1>(ref()))
            : JS_UNDEFINED;
    }

    void set_s(JSContext *ctx, bridge::String string)
    {
        ref() = static_cast<std::string>(string);
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Variant::funcs[] = {
    JS_CGETSET_DEF("i", &bridge::Getter<&Variant::get_i>, &bridge::Setter<&Variant::set_i>),
    JS_CGETSET_DEF("s", &bridge::Getter<&Variant::get_s>, &bridge::Setter<&Variant::set_s>)
};

JSValue make(JSContext *ctx, JSValue self, int argc, JSValueConst *argv)
{
    return Variant::ctor::invoke(ctx, self, argc, argv);
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("make", 1, &make)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Variant::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Variant::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
