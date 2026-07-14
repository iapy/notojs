#pragma once
#include <notojs/detail/jscode.hpp>

#include <nanodi.hpp>
#include <plugin.hpp>
#include <string>
#include <memory>

namespace notojs {

class Cacher;
class Config;
class Engine;
class Errors;
class Folder;
class Module;
class Server;

class Plugin : public Requires<Cacher, Engine, Errors, Folder, Module, Server>, public IHost
{
private:
    void configure(boost::property_tree::ptree const &);
    friend class Config;

public:
    void mod();
    void run();
    void end();

    void removed(std::string const &);
    void updated(std::string const &);

public:
    bool exec(std::string const &, IContext &) override;
    void load(std::string const &) override;
    void clog(std::string &&) override;
    MDB_env *lmdb() override;

private:
    std::unordered_map<std::string, std::unique_ptr<IPlugin>> plugins;
};

} // namespace notojs
