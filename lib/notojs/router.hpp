#pragma once
#include <notojs/detail/jscode.hpp>
#include <boost/url/url_view.hpp>
#include <boost/asio.hpp>
#include <nanodi.hpp>
#include <utility>
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
    struct Flag
    {
        bool html;
        bool json;
        bool skip{false};
        std::string name;

        BOOST_FORCEINLINE operator bool () { return html || json; }
    };

    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, boost::urls::url_view const &);
    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, detail::Textcode &&, Flag) const;
    void jsexec(std::shared_ptr<Handle>, Engine const &, boost::asio::thread_pool &, detail::Version &&, std::string &&, std::string &&);

    bool storage(std::shared_ptr<Handle> &&, std::string const &tail, boost::asio::thread_pool &, boost::urls::url_view const &url);
private:
    std::string_view window_{""};
    std::function<std::string()> rotate_{};
};

} // namespace notojs
