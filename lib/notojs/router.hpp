#pragma once
#include <notojs/detail/jscode.hpp>
#include <boost/url/url_view.hpp>
#include <boost/asio.hpp>
#include <nanodi.hpp>
#include <atomic>
#include <bitset>
#include <memory>

namespace notojs {

class Cacher;
class Engine;
class Folder;
class Handle;
class Plugin;
class Server;
class Window;

class Router : public Requires<Cacher, Folder, Plugin, Server, Window>
{
public:
    Router() = default;
    void route(std::shared_ptr<Handle>, Engine &, boost::asio::thread_pool &);

public: // configuration
    void window(std::string_view &&);
    void rotate(std::function<std::string()> &&);

private:
    struct Flag : private std::bitset<3>
    {
        BOOST_FORCEINLINE Flag()
        : std::bitset<3>{}
        , html{std::bitset<3>::operator[](0)}
        , json{std::bitset<3>::operator[](1)}
        , skip{std::bitset<3>::operator[](2)}
        {}

        BOOST_FORCEINLINE Flag(Flag const &other)
        : std::bitset<3>{other}
        , html{std::bitset<3>::operator[](0)}
        , json{std::bitset<3>::operator[](1)}
        , skip{std::bitset<3>::operator[](2)}
        , name{other.name}
        {}

        std::bitset<3>::reference html;
        std::bitset<3>::reference json;
        std::bitset<3>::reference skip;

        std::string name;
        using std::bitset<3>::none;
        BOOST_FORCEINLINE operator bool () { return 1 == count(); }
    };

    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, detail::Textcode &&, std::string &&, Flag);
    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, boost::urls::url_view const &);
    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, detail::Version &&, std::string &&, std::string &&, std::string &&);

    bool storage(std::shared_ptr<Handle> &&, std::string const &tail, boost::asio::thread_pool &, boost::urls::url_view const &url);

    template<typename F>
    void enqueue(std::shared_ptr<Handle> handle, boost::asio::thread_pool &pool, F &&f);
private:
    std::string_view window_{""};
    std::function<std::string()> rotate_{};
    std::atomic_size_t created_threads_{0};
    std::atomic_size_t outstanding_jobs_{0};
};

} // namespace notojs
