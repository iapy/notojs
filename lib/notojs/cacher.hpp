#pragma once
#include <notojs/detail/jscode.hpp>
#include <nanodi.hpp>

#include <unordered_map>
#include <thread>
#include <tuple>

namespace notojs {

class Engine;
class Errors;
class Folder;
class Server;

class Cacher : public Requires<Engine, Errors, Folder, Server>
{
public:
    using Cached = std::unordered_map<std::string, detail::Cached>;
    using Result = std::tuple<Cached::iterator, std::thread::id, std::chrono::system_clock::time_point>;
    Result cache(Cached &, detail::Version const &, std::string const &);
};

} // namespace notojs
