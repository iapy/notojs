#pragma once
#include <boost/config.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace notojs {

class SocketBase
{
public:
    using Response = boost::beast::http::response<boost::beast::http::string_body>;
    boost::beast::http::request_parser<boost::beast::http::string_body> parser;
    Response response;
};

class Socket : public SocketBase
{
protected:
    BOOST_FORCEINLINE Socket(boost::asio::ip::tcp::socket &&socket)
    : socket{std::move(socket)}
    {
        parser.body_limit(std::numeric_limits<std::size_t>::max());
    }

public:
    boost::beast::flat_buffer buffer;
    boost::asio::ip::tcp::socket socket;
};

} // namespace notojs
