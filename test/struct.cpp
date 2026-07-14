#include <bridge.hpp>
#include <map>

namespace {

struct Configurable : bridge::Interface<Configurable, std::map<std::string, std::string>>
{
    struct Method : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            bridge::String v(ctx, *value);
            if(auto const &sv = static_cast<std::string_view>(v); (
                static_cast<std::string_view>(v) != "GET" && static_cast<std::string_view>(v) != "POST"))
            {
                message.append("invalid method [");
                message.append(sv.data(), sv.size());
                message.append("]");
                return false;
            }
            return true;
        }
    };

    struct Config : bridge::Struct<Config>
    {
        BRIDGE_DEFINE_STRUCT(Config);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Either<bridge::String, bridge::Object>>("body"),
            bridge::field<bridge::String>("path"),
            bridge::field<Method>("method")
        );
    };

    Configurable(Config config)
    {
        if(auto method = config.get<Method>("method"); method)
        {
            ref().insert({"method", static_cast<std::string>(*method)});
        }
        if(auto path = config.get<bridge::String>("path"); path)
        {
            ref().insert({"path", static_cast<std::string>(*path)});
        }
        if(auto body = config.get<bridge::String>("body"); body)
        {
            ref().insert({"body", static_cast<std::string>(*body)});
        }
        else if(auto json = config.get<bridge::Object>("body"); json)
        {
            ref().insert({"body", static_cast<std::string>(json->json())});
        }
    }

    JSValue get_config(JSContext *ctx) const
    {
        std::string value;
        for(auto const &[k, v]: ref())
        {
            if(!value.empty()) value.append(";");
            value.append(k);
            value.append("=");
            value.append(v);
        }
        return bridge::String(ctx, value);
    }

    using ctor = bridge::Constructor
    <
        Configurable(Config)
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Configurable::funcs[] = {
    JS_CGETSET_DEF("config", &bridge::Getter<&Configurable::get_config>, NULL)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    Configurable::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Configurable::name());
    return mod;
}

} // extern "C"
