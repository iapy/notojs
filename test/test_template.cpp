#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Template, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Import)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const Mustache = require('mustache');
assert(() => '42' == Mustache.render('{{x}}', {x: 42}));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Markdown)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Markdown } from 'noto:core';

const raw = $('name = {{ name }}');
const txt = $(raw, {name: 'test'});

assert(() => 'name = test' == txt.data);
assert(() => txt instanceof Markdown);
assert(() => Object.isFrozen(txt));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(String)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 'name = test' == $('name = {{ f }}', {f: 'test'}));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Throws)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { markdown } from 'noto:core';
assert(() => throws(() => $('{{#a}}', {})));
assert(() => {
    try {
        $(markdown({}), {});
    } catch (error) {
        return error instanceof TypeError;
    }
    return false;
});
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
