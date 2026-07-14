#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Assert, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(ReturnType)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1);
    )JS");

    BOOST_TEST(get_error() == "Expecting bool: () => 1");
}

BOOST_AUTO_TEST_CASE(Success)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1 == 1);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Failure)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1 == 2);
    )JS");

    BOOST_TEST(get_error() == "1 == 2");
}

BOOST_AUTO_TEST_CASE(ThrowsFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => 1));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => 1))JS");
}

BOOST_AUTO_TEST_CASE(ThrowsSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => { throw ""; }));
assert(() => throws(() => { throw "Test"; }, 'Test'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMeasageSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => { throw new Error("message"); }, "message"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMeasageFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => { throw new Error("message"); }, "xxx"));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => { throw new Error("message"); }, "xxx"))JS");
}

BOOST_AUTO_TEST_CASE(AsyncThrowsSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var p = new Promise((resolve, reject) => {
    reject(new Error());
});
assert(() => throws(() => p));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var p = new Promise((resolve, reject) => {
    resolve();
});
assert(() => throws(() => p));
    )JS");

    BOOST_TEST(get_error() == "throws(() => p)");
}

BOOST_AUTO_TEST_CASE(AsyncThrowsMeasageSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
var p = new Promise((resolve, reject) => {
    reject(new Error('Test'));
});
assert(() => throws(() => p, 'Test'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsMeasageFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
var p = new Promise((resolve, reject) => {
    reject(new Error('Foo'));
});
assert(() => throws(() => p, 'Bar'));
    )JS");

    BOOST_TEST(get_error() == "throws(() => p, 'Bar')");
}

BOOST_AUTO_TEST_SUITE_END()
