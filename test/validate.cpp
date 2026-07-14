#include <bridge.hpp>
namespace {

struct Path : bridge::Interface<Path, std::string>
{
    struct Absolute : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            bridge::String v(ctx, *value);
            if(auto const &sv = static_cast<std::string_view>(v); !sv.size() || '/' != *sv.data())
            {
                message.append("invalid path [");
                message.append(sv.data(), sv.size());
                message.append("]");
                return false;
            }
            return true;
        }
    };

    Path(Absolute path)
    {
        ref() = static_cast<std::string>(path);
    }

    JSValue get(JSContext *ctx) const
    {
        return bridge::String(ctx, ref());
    }

    void set(JSContext *ctx, Absolute value)
    {
        ref() = static_cast<std::string>(value);
    }

    using ctor = bridge::Constructor
    <
        Path(Absolute)
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Path::funcs[] = {
    JS_CGETSET_DEF("path", &bridge::Getter<&Path::get>, &bridge::Setter<&Path::set>)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Path::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Path::name());
    return mod;
}

} // extern "C"
