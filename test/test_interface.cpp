#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Interface, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { A, B, C, D, get_name} from 'interface.so';

assert(() => "bar" == get_name(new A("bar")));
assert(() => "foo" == get_name(new A("foo")));

assert(() => "0" == get_name(new B(0)));
assert(() => "1" == get_name(new B(1)));

assert(() => "bar" == get_name(new C("bar")));
assert(() => "foo" == get_name(new C("foo")));

assert(() => "D:bar" == get_name(new D("bar")));
assert(() => "D:foo" == get_name(new D("foo")));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
