#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Primitive, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Integer } from 'primitive.so';

const a = new Integer(11);
const b = new Integer(31);

assert(() => 42 === a + b);
assert(() => 'default' == a.hint);
assert(() => 'default' == b.hint);

assert(() => 11 === +a);
assert(() => 'number' == a.hint);

assert(() => '11' == new String(a));
assert(() => 'string' == a.hint);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
