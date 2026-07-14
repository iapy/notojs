#pragma once
#include <boost/property_tree/ptree.hpp>
#include <notojs/detail/jscode.hpp>
#include <nanodi.hpp>

#include <condition_variable>
#include <unordered_map>
#include <utility>
#include <thread>
#include <bitset>
#include <atomic>
#include <mutex>
#include <set>

namespace notojs {

class Cacher;
class Config;
class Engine;
class Errors;
class Folder;
class Server;

class Timers : public Requires<Cacher, Engine, Errors, Folder, Server>
{
public:
    void end();
    void run();

    void removed(std::string const &);
    void updated(std::string const &);

private:
    void configure(boost::property_tree::ptree const &);
    friend class Config;

private:
    std::mutex mutex;
    std::thread thread;
    std::condition_variable cv;
    std::atomic_bool stop{true};

    std::set<int> schedule;
    std::unordered_map<std::string, std::bitset<60>> handlers;
    std::unordered_map<std::string, std::optional<detail::Version>> versions;
};

} // namespace notojs
