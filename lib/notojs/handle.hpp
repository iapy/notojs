#pragma once
#include <notojs/socket.hpp>
#include <string_view>
#include <memory>

namespace notojs {

class Engine;
class Router;
class Window;
class Handle
: Socket
, public std::enable_shared_from_this<Handle>
{
public:
    Handle(boost::asio::ip::tcp::socket &&);

    Handle(Handle &&) = delete;
    Handle(Handle const &) = delete;

    void process(Engine &, Router &, boost::asio::thread_pool &);

private:
    friend class Router;
    friend class Window;

    void finish();
    void finish(std::string_view const *body);

private:
    struct Serializer : boost::beast::http::response_serializer<boost::beast::http::string_body>
    {
        Serializer(boost::beast::http::response<boost::beast::http::string_body> &&response)
        : boost::beast::http::response_serializer<boost::beast::http::string_body>(std::move(response)) {}

        std::size_t pos{0};
    };
    void write(std::string_view const *body);
    std::optional<Serializer> serializer;
};

} // namespace notojs
