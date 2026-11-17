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
assert(() => undefined === v1.last);

v1.last = 5;
assert(() => "5" === v1.last);

v1.append("foo", 2);
assert(() => 3 === v1.size());
assert(() => "2" === v1.last);
assert(throws(() => v1.count('foo'), TypeError));
assert(throws(() => v1.first = 1000, TypeError));
assert(throws(() => 50 === v1.first, TypeError));

const v2 = new Derived();
assert(() => 0 === v2.size());

v2.first = "baz";
assert(() => "baz" === v2.first);
assert(() => "baz" === v2.last);

v2.append(4, "bar");
assert(() => 3 === v2.size());
assert(() => 1 === v2.count('bar'));

v2.append(2, "bar", "bar", "bar");
assert(() => 7 === v2.size());
assert(() => 4 === v2.count('bar'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
