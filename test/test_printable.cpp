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

assert(() => throws(() => print(a, a), 'Printable object must be only argument'));
assert(() => throws(() => print(1, a), 'Printable object must be only argument'));
assert(() => throws(() => print(a, 1), 'Printable object must be only argument'));
assert(() => throws(() => print['100%'](a), 'Printable object must be only argument'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "test"));
}

BOOST_AUTO_TEST_SUITE_END()
