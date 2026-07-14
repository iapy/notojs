#include <bridge.hpp>
#include <map>

namespace {

struct IDict : bridge::Interface<IDict, std::map<std::string, int>>
{
    IDict() = default;

    IDict(bridge::Dict<bridge::Number> dict)
    {
        dict.each([this](auto &&key, auto value){
            ref().insert({std::string(key.data(), key.size()), static_cast<int>(value)});
        });
    }

    JSValue get_string(JSContext *ctx) const
    {
        std::string value;
        for(auto const &[k, v]: ref())
        {
            value.append(";", 1);
            value.append(k);
            value.append(":", 1);
            value.append(std::to_string(v));
        }
        return bridge::String(ctx, value.empty() ? value : value.substr(1));
    }

    struct Keys : Wrapped::const_iterator
    {
        using Base = Wrapped::const_iterator;
        Keys(Base &&base): Base{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::String(ctx, (*this)->first);
        }
        inline Keys &operator ++ ()
        {
            Base::operator ++ ();
            return *this;
        }
    };

    struct Values : Wrapped::const_iterator
    {
        using Base = Wrapped::const_iterator;
        Values(Base &&base): Base{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::Number(ctx, (*this)->second);
        }
        inline Values &operator ++ ()
        {
            Base::operator ++ ();
            return *this;
        }
    };

    inline JSValue keys(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Keys>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    inline JSValue values(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Values>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    using ctor = bridge::Constructor
    <
        IDict(),
        IDict(bridge::Dict<bridge::Number>)
    >;

    using priv = bridge::Private
    <
        bridge::Iterator<Keys>,
        bridge::Iterator<Values>
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const IDict::funcs[] = {
    JS_CGETSET_DEF("string", &bridge::Getter<&IDict::get_string>, NULL),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&IDict::keys>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&IDict::values>::invoke)
};

struct SDict : bridge::Interface<SDict, std::map<std::string, std::string>>
{
    SDict() = default;

    SDict(bridge::Dict<bridge::String> dict)
    {
        dict.each([this](auto &&key, auto value){
            ref().insert({std::string(key.data(), key.size()), static_cast<std::string>(value)});
        });
    }

    JSValue get_string(JSContext *ctx) const
    {
        std::string value;
        for(auto const &[k, v]: ref())
        {
            value.append(";", 1);
            value.append(k);
            value.append(":", 1);
            value.append(v);
        }
        return bridge::String(ctx, value.empty() ? value : value.substr(1));
    }

    struct Keys : Wrapped::const_iterator
    {
        using Base = Wrapped::const_iterator;
        Keys(Base &&base): Base{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::String(ctx, (*this)->first);
        }
        inline Keys &operator ++ ()
        {
            Base::operator ++ ();
            return *this;
        }
    };

    struct Values : Wrapped::const_iterator
    {
        using Base = Wrapped::const_iterator;
        Values(Base &&base): Base{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::String(ctx, (*this)->second);
        }
        inline Values &operator ++ ()
        {
            Base::operator ++ ();
            return *this;
        }
    };

    inline JSValue keys(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Keys>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    inline JSValue values(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Values>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    using ctor = bridge::Constructor
    <
        SDict(),
        SDict(bridge::Dict<bridge::String>)
    >;

    using priv = bridge::Private
    <
        bridge::Iterator<Keys>,
        bridge::Iterator<Values>
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SDict::funcs[] = {
    JS_CGETSET_DEF("string", &bridge::Getter<&SDict::get_string>, NULL),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&SDict::keys>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&SDict::values>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    IDict::init(ctx, m);
    SDict::init(ctx, m);
    return 0;
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, IDict::name());
    JS_AddModuleExport(ctx, mod, SDict::name());
    return mod;
}

} // extern "C"
