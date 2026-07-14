#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Doc, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Smoke)
{
    eval(R"JS(
import doc from 'noto:doc';
print(doc());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
