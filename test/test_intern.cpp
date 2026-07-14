#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Intern, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(FetchPlain)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { request } from 'intern.so';

const response = await request();
assert(() => response.ok);

const json = await response.json();
assert(() => 42 == json.foo);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchMake)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { request } from 'intern.so';

const response = await request('foo', {a: 10});
assert(() => response.ok);

const json = await response.json();
assert(() => 'foo' == json.type);
assert(() => 10 == json.data.a);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
