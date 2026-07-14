#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/property_tree/ptree.hpp>

#include <nanodi.hpp>

namespace notojs {

class Config;
class Engine;
class Plugin;
class Router;
class Timers;
class Window;

class Server : public Requires<Engine, Plugin, Router, Timers, Window>
{
private:
    int main();
    void configure(boost::property_tree::ptree const &);
    friend class Config;

public:
    std::thread run(std::function<std::shared_ptr<boost::asio::ip::tcp::acceptor>(boost::asio::io_context &)> &&);
    void stop(std::thread &);

private:
    boost::asio::io_context ioc{1};
    boost::asio::ip::tcp::endpoint endpoint;
    std::pair<std::size_t, std::size_t> threads{1,1};

public:
    boost::asio::thread_pool *disk{nullptr};
    boost::asio::ip::tcp::resolver resolver{ioc};
    std::unique_ptr<boost::asio::thread_pool> sync;
    boost::asio::ssl::context sslcontext{boost::asio::ssl::context::sslv23_client};
};

} // namespace notojs
