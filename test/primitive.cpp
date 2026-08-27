#include <bridge.hpp>

namespace {

struct Integer : bridge::Interface<Integer, std::pair<int64_t, std::string>>
{
    Integer() = default;

    Integer(bridge::Number n)
    {
        ref().first = static_cast<std::int64_t>(n);
    }

    JSValue hint(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().second};
    }

    JSValue toPrimitive(JSContext *ctx, bridge::String hint)
    {
        if(auto const &hv = static_cast<std::string_view const &>(hint); "default" == hv || "number" == hv)
            return ref().second = hv, bridge::Number{ctx, ref().first};
        else if("string" == hv)
            return ref().second = hv, bridge::String{ctx, std::to_string(ref().first)};
        return JS_UNDEFINED;
    }

    using ctor = bridge::Constructor
    <
        Integer(),
        Integer(bridge::Number)
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Integer::funcs[] = {
    JS_CGETSET_DEF("hint", &bridge::Getter<&Integer::hint>, NULL),
    JS_CFUNC_DEF("[Symbol.toPrimitive]", 0, &bridge::Function<&Integer::toPrimitive>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Integer::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Integer::name());
    return mod;
}

} // extern "C"
