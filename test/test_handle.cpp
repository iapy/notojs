#include <boost/test/unit_test.hpp>
#include <notojs/socket.hpp>
#include <memory.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Handle, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Text)
{
    notojs::SocketBase socket;
    socket.parser.get().target("/text");
    socket.parser.get().body() = "hello";
    socket.parser.get().set(boost::beast::http::field::content_type, "text/txt");
    socket.parser.get().method(boost::beast::http::verb::post);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => "/text" == request.path);
assert(() => "POST" == request.method);
assert(() => request.headers.has("Content-Type"));
assert(() => "text/txt" == request.headers.get("Content-Type"));

const text = await request.text();
response.status = 250;
response.body = text;
response.headers.set("X-Foo", "bar");
response.headers.set("Content-Type", "text");
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(250 == socket.response.result_int());
    BOOST_TEST("hello" == socket.response.body());
    BOOST_TEST("bar" == socket.response.find("X-Foo")->value());
    BOOST_TEST("text" == socket.response.find(boost::beast::http::field::content_type)->value());
    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Json)
{
    notojs::SocketBase socket;
    socket.parser.get().body() = "{\"a\":10}";
    socket.parser.get().target("/json");
    socket.parser.get().method(boost::beast::http::verb::put);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => "/json" == request.path);
assert(() => "PUT" == request.method);

const json = await request.json();
response.status = 200;
response.body = json;
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(200 == socket.response.result_int());
    BOOST_TEST("{\"a\":10}" == socket.response.body());
    BOOST_TEST("application/json" == socket.response.find(boost::beast::http::field::content_type)->value());
    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
