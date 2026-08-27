#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Mixin, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Smoke)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vector, Derived } from 'mixin.so';

const v1 = new Vector();
assert(() => 0 === v1.size());

v1.append("foo", 2);
assert(() => 2 === v1.size());
assert(() => throws(() => v1.count('foo')));

const v2 = new Derived();
assert(() => 0 === v2.size());

v2.append(4, "bar");
assert(() => 2 === v2.size());
assert(() => 1 === v2.count('bar'));

v2.append(2, "bar", "bar", "bar");
assert(() => 6 === v2.size());
assert(() => 4 === v2.count('bar'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
