#include <lmdbxx/lmdb++.h>
#include <module.hpp>
#include <plugin.hpp>

#include <stdexcept>

namespace {

struct TestDatabase : noto::db::Interface<TestDatabase>
{
    JSValue count(JSContext *ctx) const
    {
        if(!ref().db) return noto::db::error(ctx);
        return bridge::Number(ctx, static_cast<std::uint64_t>(ref().db->stat(*ref().tx).ms_entries));
    }

    JSValue get(JSContext *ctx, bridge::String key)
    {
        if(!ref().db) return noto::db::error(ctx);

        auto const &data = static_cast<std::string_view const &>(key);
        lmdb::val k{data.data(), data.size()}, v;
        if(!ref().db->get(*ref().tx, k, v)) return JS_NULL;
        return bridge::String(ctx, std::string_view{v.data(), v.size()});
    }

    JSValue put(JSContext *ctx, bridge::String key, bridge::String value)
    {
        if(!ref().db) return noto::db::error(ctx);

        auto const &kd = static_cast<std::string_view const &>(key);
        auto const &vd = static_cast<std::string_view const &>(value);
        lmdb::val k{kd.data(), kd.size()};
        lmdb::val v{vd.data(), vd.size()};
        ref().db->put(*ref().tx, k, v);
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const TestDatabase::funcs[] = {
    JS_CGETSET_DEF("count", &bridge::Getter<&TestDatabase::count>, NULL),
    JS_CFUNC_DEF("get", 1, &bridge::Function<&TestDatabase::get>::invoke),
    JS_CFUNC_DEF("put", 2, &bridge::Function<&TestDatabase::put>::invoke)
};

class Plugin : public notojs::IPlugin
{
public:
    explicit Plugin(boost::property_tree::ptree const &)
    {
        TestDatabase::init("test:database");
    }

    bool run() override
    {
        return true;
    }

    void end() override {}
};

} // namespace

extern "C" notojs::IPlugin *notojs_plugin(boost::property_tree::ptree const &config)
{
    return new Plugin(config);
}
