#pragma once
#include <nanodi.hpp>
#include <unordered_map>
#include <shared_mutex>
#include <memory>

#include <boost/url/url_view.hpp>

namespace notojs {

class Config;
class Handle;
class Server;

class Window : public Requires<Config, Server>
{
public:
    void create(std::shared_ptr<Handle>);
    bool kernel(std::shared_ptr<Handle>, std::string const &uuid);
    bool jsexec(std::shared_ptr<Handle>, std::string const &uuid, std::string const &pid);
    void end();

private:
    class Impl;
    class Kernel;
    std::shared_mutex mutex;
    std::unordered_map<std::string, std::weak_ptr<Impl>> windows;
};

} // namespace notojs
