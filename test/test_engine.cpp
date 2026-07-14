#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_AUTO_TEST_SUITE(Engine)

BOOST_AUTO_TEST_CASE(Preprocessor)
{
    std::string code{"<[[\n]]>"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code == "print(new $.__Markdown({\"data\":\"\"}));/*[\n]*/");
}

BOOST_AUTO_TEST_SUITE_END()

