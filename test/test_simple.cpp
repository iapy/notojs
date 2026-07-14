#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Simple, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Construct)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Variant } from 'simple.so';

assert(() => throws(() =>  { new Variant({}); }, "Variant: no matching constructor found"));
assert(() => throws(() =>  { new Variant(0, 1); }, "Variant: no matching constructor found"));

const v1 = new Variant();
assert(() => v1.i == 0);
assert(() => v1 instanceof Variant);
assert(() => typeof(v1.s) == "undefined");
assert(() => throws(() =>  { v1.i = {}; }, "Variant: no matching setter found"));

const v2 = new Variant(1);
assert(() => v2.i == 1);
assert(() => v2 instanceof Variant);
assert(() => typeof(v2.s) == "undefined");
assert(() => throws(() =>  { v1.s = {}; }, "Variant: no matching setter found"));

const v3 = new Variant("foobar");
assert(() => v3.s == "foobar");
assert(() => v3 instanceof Variant);
assert(() => typeof(v3.i) == "undefined");
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Setter)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Variant } from 'simple.so';

const v1 = new Variant();
assert(() => throws(() => v1.i = {}, "Variant: no matching setter found"));
assert(() => throws(() => v1.s = {}, "Variant: no matching setter found"));

v1.i = 10;
assert(() => v1.i == 10);
assert(() => typeof(v1.s) == "undefined");

v1.s = 'test';
assert(() => v1.s == 'test');
assert(() => typeof(v1.i) == "undefined");
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Factory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { make, Variant } from 'simple.so';

assert(() => throws(() =>  { make({}); }, "Variant: no matching constructor found"));
assert(() => throws(() =>  { make(0, 1); }, "Variant: no matching constructor found"));

const v1 = make.call(Variant);
assert(() => v1 instanceof Variant);
assert(() => v1.i == 0);
assert(() => typeof(v1.s) == "undefined");
assert(() => throws(() =>  { v1.i = {}; }, "Variant: no matching setter found"));

const v2 = make.call(Variant, 1);
assert(() => v2.i == 1);
assert(() => typeof(v2.s) == "undefined");
assert(() => throws(() =>  { v1.s = {}; }, "Variant: no matching setter found"));

const v3 = make.call(Variant, "foobar");
assert(() => v3.s == "foobar");
assert(() => typeof(v3.i) == "undefined");
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
