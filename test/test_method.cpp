#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Method, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Methods)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Headers } from 'method.so';

var h = new Headers();
assert(() => !h.has('Header-1'));
assert(() => 'undefined' == typeof(h.get('Header-1')));

h.set('Header-1', 'Value-1');
assert(() => h.has('Header-1'));
assert(() => 'Value-1' == h.get('Header-1'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
