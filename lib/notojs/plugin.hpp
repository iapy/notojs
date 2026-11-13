#pragma once
#include <notojs/detail/config.hpp>
#include <notojs/detail/jscode.hpp>

#include <nanodi.hpp>
#include <plugin.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <map>

namespace notojs {

class Cacher;
class Engine;
class Errors;
class Folder;
class Module;
class Server;

class Plugin : public Requires<Cacher, Engine, Errors, Folder, Module, Server>, public IHost
{
    void configure(detail::Config const &);
    friend class Config;

public:
    void mod();
    void run();
    void end();

    void removed(std::filesystem::path const &);
    void updated(std::filesystem::path const &);

public:
    void clog(std::string &&) override;
    void clog(std::string &&, IHost::Args &&) override;
    bool exec(std::string const &, IContext &) override;

    MDB_env *lmdb() override;

private:
    std::map<std::string, std::unique_ptr<IPlugin>> plugins;
};

} // namespace notojs
