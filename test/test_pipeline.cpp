#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Pipeline, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Smoke)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
        import { assert } from 'noto:assert';
        $(input => {
            assert(() => undefined === input);
            return 10;
        })
    )JS", context.get(), "cell-001");

    eval(R"JS(
        import { assert } from 'noto:assert';
        assert(() => 10 === input);
        $(input => {
            return input + 5;
        })
    )JS", context.get(), "cell-002");

    eval(R"JS(
        import { assert } from 'noto:assert';
        assert(() => 15 === input);
        $(input => {})
    )JS", context.get(), "cell-003");

    eval(R"JS(
        import { assert } from 'noto:assert';
        assert(() => 15 === input);
    )JS", context.get(), "cell-004");

    eval(R"JS(
        import { assert } from 'noto:assert';
        await $(async (input) => input + 1);
    )JS", context.get(), "cell-005");

    eval(R"JS(
        import { assert } from 'noto:assert';
        await $((input) => input + 1);
        assert(() => 17 === input);
    )JS", context.get(), "cell-006");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
