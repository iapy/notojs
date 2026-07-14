#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Getter, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Getters)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Headers, Request } from 'getter.so';

var h = new Headers("headers");
var r = new Request(h, "body+headers");

assert(() => r.body == "body+headers");
assert(() => r.headers.value == "headers");

r.headers.value = "foo";
assert(() => r.headers.value == "foo");

var q = new Request(r.headers, "body+copy");
assert(() => q.body == "body+copy");
assert(() => q.headers.value == "foo");

r.headers.value = "bar";
assert(() => r.headers.value == "bar");
assert(() => q.headers.value == "foo");

r.headers = "baz";
assert(() => r.headers.value == "baz");
assert(() => q.headers.value == "foo");

q.headers = new Headers("xxx");
assert(() => r.headers.value == "baz");
assert(() => q.headers.value == "xxx");

h = q.headers;
r = new Request(h, "body+new");
assert(() => r.headers.value == "xxx");
assert(() => q.headers.value == "xxx");

q = new Request(h, "q");
assert(() => q.headers.value == "xxx");
assert(() => q.body == "q");

q.headers.value = "bar";
assert(() => q.headers.value == "bar");
assert(() => r.headers.value == "xxx");
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Bodyof)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Headers, Request, bodyof } from 'getter.so';

assert(() => '' == bodyof());
assert(() => 'bar' == bodyof('foo', 'bar'));
assert(() => 'body' == bodyof(new Request('body'), new Headers('bar')));
assert(() => throws(() => bodyof(1,2,3)));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Memory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Headers, Request } from 'getter.so';

function get_headers() {
    return new Request(new Headers("headers"), "body").headers;
}

assert(() => 'headers' == get_headers().value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
