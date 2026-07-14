#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Vararg, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Const)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vararg } from 'vararg.so';

const v = new Vararg();
assert(() => throws(() => v.cappend(), "No matching function overload found"));
assert(() => throws(() => v.cappend('a', 1), "No matching function overload found"));
assert(() => throws(() => v.cappend('a', {}), "No matching function overload found"));
assert(() => throws(() => v.cappend(1, {}), "No matching function overload found"));
assert(() => throws(() => v.cappend(1, 'a', 1), "No matching function overload found"));
assert(() => throws(() => v.cappend(1, 1, 'a'), "No matching function overload found"));

v.append('a');
assert(() => 'a b' == v.cappend('b'));
assert(() => 'a' == v.value);

assert(() => 'a b c' == v.cappend('b', 'c'));
assert(() => 'a' == v.value);

assert(() => 'a 0' == v.cappend(0));
assert(() => 'a' == v.value);

assert(() => 'a 0 b c' == v.cappend(0, 'b', 'c'));
assert(() => 'a' == v.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NoConst)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vararg } from 'vararg.so';

const v = new Vararg();
assert(() => throws(() => v.append(), "No matching function overload found"));
assert(() => throws(() => v.append('a', 1), "No matching function overload found"));
assert(() => throws(() => v.append('a', {}), "No matching function overload found"));
assert(() => throws(() => v.append(1, {}), "No matching function overload found"));
assert(() => throws(() => v.append(1, 'a', 1), "No matching function overload found"));
assert(() => throws(() => v.append(1, 1, 'a'), "No matching function overload found"));
assert(() => throws(() => v.eappend(1, {}, 1, 'a'), "No matching function overload found"));

v.append('a');
assert(() => 'a' == v.value);

v.append('b', 'c');
assert(() => 'a b c' == v.value);

v.append(0);
assert(() => 'a b c 0' == v.value);

v.append(1, 'x', 'y', 'z');
assert(() => 'a b c 0 1 x y z' == v.value);

v.eappend(2, 'd', 3, 'e', 4, 'f');
assert(() => 'a b c 0 1 x y z 2 d 3 e 4 f' == v.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
