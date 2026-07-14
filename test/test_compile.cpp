#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Compile, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    notojs::detail::Bytecode bytecode;
    eval(R"JS(
print('Compiled');
    )JS", bytecode);

    eval(bytecode);

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "Compiled"));
}

BOOST_AUTO_TEST_CASE(Import)
{
    notojs::detail::Bytecode bytecode;
    eval(R"JS(
import { test } from 'module.js';
test();
    )JS", bytecode);

    eval(bytecode);

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);
    BOOST_TEST(!strcmp(get_output()->get()[0][0].GetString(), "module.js"));

    eval(bytecode);

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);
    BOOST_TEST(!strcmp(get_output()->get()[0][0].GetString(), "module.js"));
}

BOOST_AUTO_TEST_SUITE_END()
