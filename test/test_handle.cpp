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

BOOST_AUTO_TEST_CASE(Multipart)
{
    std::string body;
    body.append("--AaB03x\r\n");
    body.append("Content-Disposition: form-data; name=\"name\"\r\n\r\n");
    body.append("first\r\n");
    body.append("--AaB03x\r\n");
    body.append("content-disposition: form-data; name=\"name\"\r\n\r\n");
    body.append("second\r\n");
    body.append("--AaB03x\r\n");
    body.append("Content-Disposition: form-data; name=\"empty\"\r\n\r\n");
    body.append("\r\n");
    body.append("--AaB03x\r\n");
    body.append("Content-Disposition: form-data; name=\"upload\"; filename=\"report\\\".bin\"\r\n");
    body.append("Content-Type: application/octet-stream\r\n\r\n");
    body.append("a", 1);
    body.push_back('\0');
    body.append("b\r\n--AaB03xX-not-a-delimiter");
    body.append("\r\n--AaB03x--not-a-delimiter");
    body.append("\r\n--AaB03x-- \t\r\n");

    notojs::SocketBase socket;
    socket.parser.get().target("/multipart");
    socket.parser.get().body() = std::move(body);
    socket.parser.get().set(
        boost::beast::http::field::content_type,
        "multipart/form-data; charset=UTF-8; boundary=\"AaB03x\""
    );
    socket.parser.get().method(boost::beast::http::verb::post);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

const data = await request.formData();
assert(() => data instanceof FormData);
assert(() => 'first' == data.get('name'));
assert(() => 'first,second' == data.getAll('name').join(','));
assert(() => '' == data.get('empty'));

const upload = data.get('upload');
assert(() => upload instanceof File);
assert(() => upload instanceof Blob);
assert(() => 'report".bin' == upload.name);
assert(() => 'application/octet-stream' == upload.type);

const bytes = await upload.bytes();
const contents = String.fromCharCode(...bytes);
assert(() => 'a\0b\r\n--AaB03xX-not-a-delimiter\r\n--AaB03x--not-a-delimiter' == contents);
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(MultipartMalformed)
{
    notojs::SocketBase socket;
    socket.parser.get().target("/multipart");
    socket.parser.get().body() = "body";
    socket.parser.get().set(boost::beast::http::field::content_type, "multipart/form-data");
    socket.parser.get().method(boost::beast::http::verb::post);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => request.formData(), 'Missing multipart boundary'));
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FormDataUrlEncoded)
{
    notojs::SocketBase socket;
    socket.parser.get().target("/form");
    socket.parser.get().body() = "name=first&name=second&message=hello+world&empty=";
    socket.parser.get().set(
        boost::beast::http::field::content_type,
        "Application/X-WWW-Form-Urlencoded; charset=UTF-8"
    );
    socket.parser.get().method(boost::beast::http::verb::post);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

const data = await request.formData();
assert(() => data instanceof FormData);
assert(() => 'first' == data.get('name'));
assert(() => 'first,second' == data.getAll('name').join(','));
assert(() => 'hello world' == data.get('message'));
assert(() => '' == data.get('empty'));
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FormDataUnsupported)
{
    notojs::SocketBase socket;
    socket.parser.get().target("/form");
    socket.parser.get().body() = "{}";
    socket.parser.get().set(boost::beast::http::field::content_type, "application/json");
    socket.parser.get().method(boost::beast::http::verb::post);

    bridge::Context context{notojs::testing::engine->get_context()};
    notojs::testing::global->set_handle(context.get(), socket);

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(
    () => request.formData(),
    'Unsupported form data Content-Type: application/json'
));
    )JS", context.get(), "cell-uuid-1");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
