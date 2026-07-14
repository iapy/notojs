#include <notojs/detail/router.hpp>
#include <notojs/socket.hpp>
#include <memory>

#include "test_engine.hpp"

namespace notojs::testing {
namespace {

constexpr auto router = detail::router(
    detail::route("/text"),
    detail::route("/json"),
    detail::route("/json/bad"),
    detail::route("/redirect"),
    detail::route("/redirect/2"),
    detail::route("/redirect/https"),
    detail::route("/redirect/noloc"),
    detail::route("/redirect/badloc"),
    detail::route("/redirect/post"),
    detail::route("/module.js"),
    detail::route("/script.js"),
    detail::route("/svg")
);

class Handle
: Socket
, public std::enable_shared_from_this<Handle>
{
public:
    Handle(boost::asio::ip::tcp::socket &&socket)
    : notojs::Socket(std::move(socket)) {}

public:
    static void loop(std::shared_ptr<boost::asio::ip::tcp::acceptor>);
    static void process(std::shared_ptr<Handle>);
};

void Handle::process(std::shared_ptr<Handle> self)
{
    boost::beast::http::async_read(self->socket, self->buffer, self->parser.get(), [self](boost::beast::error_code ec, std::size_t){
        if(!ec)
        {
            self->response.set(boost::beast::http::field::server, "boost:beast");

            std::string tail;
            switch(router.find(self->parser.get().target(), tail))
            {
            case router["/text"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.set(boost::beast::http::field::content_type, "text/plain");
                    self->response.body() = "text";
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/json"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.set(boost::beast::http::field::content_type, "application/json");
                    self->response.body() = R"JSON({
                        "string": "string",
                        "number": 42
                    })JSON";
                    break;
                case boost::beast::http::verb::post:
                    self->response.set(boost::beast::http::field::content_type, "application/json");
                    self->response.body() = self->parser.get().body();
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/json/bad"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.set(boost::beast::http::field::content_type, "application/json");
                    self->response.body() = "[}";
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.result(boost::beast::http::status::found);
                    self->response.set(boost::beast::http::field::location, "/json");
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect/2"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.result(boost::beast::http::status::found);
                    self->response.set(boost::beast::http::field::location, "/redirect");
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect/https"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.result(boost::beast::http::status::moved_permanently);
                    self->response.set(boost::beast::http::field::location, "https://api.myip.com/");
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect/noloc"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.result(boost::beast::http::status::found);
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect/badloc"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.set(boost::beast::http::field::location, "json");
                    self->response.result(boost::beast::http::status::found);
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/redirect/post"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::post:
                    self->response.set(boost::beast::http::field::location, "/json");
                    self->response.result(boost::beast::http::status::see_other);
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/module.js"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.body() = R"JS(
export function bar() { print('bar'); }
print('foo');
                    )JS";
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/script.js"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.body() = R"JS(
let x = 1;
var y = 2;
const z = 3;

function foo() { return a + x; }
function bar() { return a + y; }
function baz() { return a + z; }
                    )JS";
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            case router["/svg"]:
                switch(self->parser.get().method())
                {
                case boost::beast::http::verb::get:
                    self->response.set(boost::beast::http::field::content_type, "image/svg+xml");
                    self->response.body() = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="currentColor" d="M7.5 15q-.625 0-1.062-.437T6 13.5v-.25q0-.325.213-.537t.537-.213t.538.213t.212.537v.25H9V9.75q0-.325.213-.537T9.75 9t.538.213t.212.537v3.75q0 .625-.437 1.063T9 15zm5.5 0q-.425 0-.712-.288T12 14v-.5q0-.2.15-.35t.35-.15h.5q.2 0 .35.15t.15.35h2v-1H13q-.425 0-.712-.288T12 11.5V10q0-.425.288-.712T13 9h3q.425 0 .713.288T17 10v.5q0 .2-.15.35t-.35.15H16q-.2 0-.35-.15t-.15-.35h-2v1H16q.425 0 .713.288T17 12.5V14q0 .425-.288.713T16 15z"/></svg>)SVG";
                    break;
                default:
                    self->response.result(boost::beast::http::status::method_not_allowed);
                    break;
                }
                break;
            default:
                self->response.result(boost::beast::http::status::not_found);
                break;
            }
            self->response.prepare_payload();
            boost::asio::post(self->socket.get_executor(), [self]{
                boost::beast::http::async_write(self->socket, self->response, [self](boost::beast::error_code ec, std::size_t){
                    self->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                });
            });
        }
    });
}

void Handle::loop(std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor)
{
    acceptor->async_accept([acceptor](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if(!ec) Handle::process(std::make_shared<Handle>(std::move(socket)));
        Handle::loop(acceptor);
    });
}

} // namespace

ContextFixture::ContextFixture()
{
    io_context = server->run([](boost::asio::io_context &ioc){
        boost::asio::ip::tcp::endpoint endpoint = *std::begin(server->resolver.resolve("127.0.0.1", std::to_string(TEST_SERVER_PORT)));
        auto acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(ioc);

        acceptor->open(endpoint.protocol());
        acceptor->set_option(boost::asio::socket_base::reuse_address(true));

        acceptor->bind(endpoint);
        acceptor->listen(boost::asio::socket_base::max_listen_connections);

        Handle::loop(acceptor);
        return acceptor;
    });
}

ContextFixture::~ContextFixture()
{
    server->stop(io_context);
}

} // namespace notojs::testing
