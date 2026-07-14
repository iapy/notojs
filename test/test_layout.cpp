#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Layout, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Columns)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { html } from 'noto:core';

assert(() => throws(() => print['abracadabra'](), "Invalid layout: [abracadabra]"));
assert(() => throws(() => print['40 50'](), "Invalid layout: [40 50]"));
assert(() => throws(() => print['40% 50'](), "Invalid layout: [40% 50]"));
assert(() => throws(() => print['40 50%'](), "Invalid layout: [40 50%]"));
assert(() => throws(() => print['40- 50%'](), "Invalid layout: [40- 50%]"));

print['40%'](html(''));
print['40% 60%'](html(''), html(''));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
