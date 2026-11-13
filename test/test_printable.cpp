#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Printable, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Printable } from 'printable.so';

const a = new Printable('test');
print(a);

assert(throws(() => print(a, a), RangeError, 'Printable object must be only argument'));
assert(throws(() => print(1, a), RangeError, 'Printable object must be only argument'));
assert(throws(() => print(a, 1), RangeError, 'Printable object must be only argument'));
assert(throws(() => print['100%'](a), RangeError, 'Printable object must be only argument'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "test"));
}

BOOST_AUTO_TEST_CASE(ConsoleLog)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Printable } from 'printable.so';
require('console');

assert(() => globalThis['Print'] === undefined);

assert(() => console.log() === undefined);
assert(() => console.log('message') === undefined);
console.log(42, true, null, {value: 'object'}, () => {});

const printable = new Printable('native printable');
assert(() => console.log(printable) === undefined);
assert(throws(() => console.log(printable, 1), RangeError, 'Printable object must be only argument'));
assert(throws(() => console.log(1, printable), RangeError, 'Printable object must be only argument'));

const log = console.log;
assert(() => log('detached') === undefined);
print('direct');
    )JS");

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 5u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0].GetString(), "message"));
    BOOST_REQUIRE(out[1].Size() == 5u);
    BOOST_TEST(out[1][0].GetInt() == 42);
    BOOST_TEST(out[1][1].GetBool());
    BOOST_TEST(out[1][2].IsNull());
    BOOST_REQUIRE(out[1][3].HasMember("value"));
    BOOST_TEST(!strcmp(out[1][3]["value"].GetString(), "object"));
    BOOST_TEST(!strcmp(out[1][4].GetString(), "function"));
    BOOST_REQUIRE(out[2].Size() == 1u);
    BOOST_TEST(!strcmp(out[2][0].GetString(), "native printable"));
    BOOST_REQUIRE(out[3].Size() == 1u);
    BOOST_TEST(!strcmp(out[3][0].GetString(), "detached"));
    BOOST_REQUIRE(out[4].Size() == 1u);
    BOOST_TEST(!strcmp(out[4][0].GetString(), "direct"));
}

BOOST_AUTO_TEST_SUITE_END()
