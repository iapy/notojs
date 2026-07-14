#include <notojs/handle.hpp>
#include <notojs/router.hpp>

namespace notojs {

Handle::Handle(boost::asio::ip::tcp::socket &&socket)
: Socket{std::move(socket)}
{
}

void Handle::finish()
{
    boost::asio::post(socket.get_executor(), [self=shared_from_this()]{
        self->response.prepare_payload();
        self->finish(nullptr);
    });
}

void Handle::finish(std::string_view const *body)
{
    if(!body)
    {
        boost::beast::http::async_write(socket, response, [self=shared_from_this()](boost::beast::error_code ec, std::size_t){
            self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        });
    }
    else
    {
        response.set(boost::beast::http::field::content_length, std::to_string(body->size()));
        serializer.emplace(std::move(response));

        boost::beast::http::async_write_header(socket, *serializer, [body, self=shared_from_this()](boost::beast::error_code ec, std::size_t){
            if(!ec) self->write(body);
            else self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        });
    }
}

void Handle::write(std::string_view const *body)
{
    socket.async_send(boost::asio::buffer(body->data() + serializer->pos, body->size() - serializer->pos),
        [body, self=shared_from_this()](boost::beast::error_code ec, std::size_t count){
            if(!ec && (self->serializer->pos += count) < body->size()) self->write(body);
            else self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        });
}

void Handle::process(Engine &engine, Router &router, boost::asio::thread_pool &pool)
{
    boost::beast::http::async_read(socket, buffer, parser, [&, handle=shared_from_this()](boost::beast::error_code ec, std::size_t){
        if(!ec) router.route(handle, engine, pool);
    });
}

} // namespace notojs
