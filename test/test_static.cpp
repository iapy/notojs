#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Static, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Props)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Enum } from 'static.so';

assert(() => 1 == Enum.ONE);
assert(() => 2 == Enum.TWO);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
