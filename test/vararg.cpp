#include <bridge.hpp>
#include <map>

namespace {

struct Vararg : bridge::Interface<Vararg, std::string>
{
    Vararg() = default;

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
    JS_CFUNC_DEF("append", 1, &Vararg::append::invoke),
    JS_CFUNC_DEF("cappend", 1, &Vararg::cappend::invoke),
    JS_CFUNC_DEF("eappend", 1, &bridge::Function<&Vararg::eappend>::invoke),
    JS_CGETSET_DEF("value", &bridge::Getter<&Vararg::get_value>, NULL),
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Vararg::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Vararg::name());
    return mod;
}

} // extern "C"
