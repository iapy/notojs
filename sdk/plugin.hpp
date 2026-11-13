#pragma once
#include <boost/property_tree/ptree.hpp>
#include <initializer_list>
#include <functional>
#include <utility>
#include <cstdint>
#include <variant>

struct JSContext;
struct JSModuleDef;
struct JSValue;
struct MDB_env;

namespace notojs {

class IContext
{
public:
    virtual void set_context(JSContext *, JSValue &);
    virtual void get_context(JSContext *, JSValue);
};

class IHost
{
public: // Typedefs
    using Args = std::initializer_list<std::variant<
        bool,
        float,
        double,
        std::string,
        std::int64_t,
        std::uint64_t
    >>;

public: // Logging
    virtual void clog(std::string &&, Args &&) = 0;
    virtual void clog(std::string &&) = 0;

public: // Notebook execition
    virtual bool exec(std::string const &, IContext &) = 0;

public: // Database access
    virtual MDB_env *lmdb() = 0;
};

class IPlugin : public IContext
{
public:
    using JSInit = int(JSContext *, JSModuleDef *);
    using Loader = IPlugin *(*)(boost::property_tree::ptree const &);
    using Module = std::function<JSModuleDef *(JSContext *ctx, char const *)>;

public:
    IPlugin() = default;
    IPlugin(IPlugin &&) = delete;
    IPlugin(IPlugin const &) = delete;
    virtual ~IPlugin() {};

    virtual bool run() = 0;
    virtual void end() = 0;
    virtual auto mod() -> Module;

    virtual void removed(std::string const &);
    virtual bool updated(std::string const &);

    auto def_module(JSContext *, char const *, JSInit) -> JSModuleDef *;
    static auto get_plugin(JSContext *, JSModuleDef *) -> std::pair<JSValue, IPlugin *>;
    static auto get_plugin(JSValue) -> IPlugin *;

protected:
    IHost *host{nullptr};
    friend class Plugin;

private:
    static std::uint32_t cid;
};

} // namespace notojs
