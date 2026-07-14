#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Export, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Function)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    eval(R"JS(
export function test() {
    print('Hello, modules');
}
    )JS", context.get(), "cell-001");

    eval(R"JS(
test();
    )JS", context.get(), "cell-002");

    BOOST_TEST(get_error() == std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "Hello, modules"));
}

BOOST_AUTO_TEST_CASE(FunctionEdit)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    eval(R"JS(
export function test() {
    print('old');
}
    )JS", context.get(), "cell-001");

    eval(R"JS(
export function test() {
    print('new');
}
    )JS", context.get(), "cell-002");

    eval(R"JS(
test();
    )JS", context.get(), "cell-003");

    BOOST_TEST(get_error() == std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "new"));
}

BOOST_AUTO_TEST_CASE(SameClass)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    eval(R"JS(
export const req = new Request();
export const res = new Response();
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => req instanceof Request);
assert(() => res instanceof Response);
    )JS", context.get(), "cell-002");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
