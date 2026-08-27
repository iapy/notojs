#include <bridge.hpp>
#include <global.hpp>

namespace {

struct Printable : bridge::Interface<Printable, std::string>
{
    using Base::Base;

    Printable(bridge::String name)
    {
        ref() = name;
    }

    struct I : Base::I<I, notojs::IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            bridge::Array arr{ctx};
            arr.append(bridge::String{ctx, ref});
            output.append(arr);
            return JS_UNDEFINED;
        }
    };

    using impl = bridge::Implements<I>;
    using ctor = bridge::Constructor
    <
        Printable(bridge::String)
    >;
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Printable::init(ctx, m);
    return 0;
}

} // namespace

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Printable::name());
    return mod;
}

} // extern "C"
