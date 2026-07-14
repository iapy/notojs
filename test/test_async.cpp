#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Async, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Success)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { run } from 'async.so';

const p = run(5);
assert(() => p instanceof Promise);

const r = await p;
assert(() => 5 == r.length);
assert(() => '5' == r[0]);
assert(() => '4' == r[1]);
assert(() => '3' == r[2]);
assert(() => '2' == r[3]);
assert(() => '1' == r[4]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Failure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { run } from 'async.so';

const p = run(-1);
assert(() => p instanceof Promise);
assert(() => throws(() => p));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
