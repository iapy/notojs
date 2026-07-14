#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Validate, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Validate)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Path } from 'validate.so'

assert(() => throws(() => new Path(), 'Path: no matching constructor found'));
assert(() => throws(() => new Path(''), 'Path constructor: invalid path []'));
assert(() => throws(() => new Path('xxx'), 'Path constructor: invalid path [xxx]'));

var a = new Path('/');
assert(() => '/' == a.path);

var b = new Path('/var');
assert(() => '/var' == b.path);

assert(() => throws(() => a.path = 'x', 'Path: invalid path [x]'));
assert(() => '/' == a.path);

a.path = '/etc';
assert(() => '/etc' == a.path);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
