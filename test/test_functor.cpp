#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Functor, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Invoke)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Functor } from 'functor.so';

const f0 = new Functor();
assert(() => 0 == f0.value);
assert(() => 1 == f0(1));
assert(() => 2 == f0(2));

const f1 = new Functor(1);
assert(() => 1 == f1.value);
assert(() => 2 == f1(1));
assert(() => 3 == f1(2));

f1.value = 2;
assert(() => 2 == f1.value);
assert(() => 3 == f1(1));
assert(() => 4 == f1(2));

assert(throws(() => f1(), TypeError, 'No matching function overload found'));
assert(throws(() => f1(''), TypeError, 'No matching function overload found'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Noinvoke)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Variant } from 'simple.so';

const v1 = new Variant();
assert(throws(() => v1(), TypeError, 'not a function'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}


BOOST_AUTO_TEST_SUITE_END()
