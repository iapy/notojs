#pragma once
#include <boost/property_tree/ptree.hpp>
#include <variant>

struct JSContext;
struct JSModuleDef;
struct JSValue;
struct MDB_env;

namespace notojs {

class IContext
{
public:
    virtual void input(JSContext *, JSValue &);
    virtual void output(JSContext *);
};

class IHost
{
public: // Typedefs
    using Args = std::initializer_list<std::variant<std::int64_t, std::string>>;

public: // Logging
    virtual void clog(std::string &&, Args &&) = 0;
    virtual void clog(std::string &&) = 0;

public: // Notebook execition
    virtual bool exec(std::string const &, IContext &) = 0;
    virtual void load(std::string const &) = 0;

public: // Database access
    virtual MDB_env *lmdb() = 0;
};

class IPlugin : public IContext
{
public:
    typedef JSModuleDef *(*Module)(JSContext *ctx, const char *);
    typedef IPlugin *(*Loader)(boost::property_tree::ptree const &);

public:
    virtual ~IPlugin() {};

    virtual bool run(IHost &) = 0;
    virtual void end(IHost &) = 0;

    virtual Module mod(IHost &);
};

} // namespace notojs
