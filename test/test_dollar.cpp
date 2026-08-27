#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Dollar, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Error)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('dollar');

assert(() => throws(() => $.ajax({dataType: 'json'}), "$.ajax: url should be specified"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Json)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('dollar');

let json = await $.ajax({url: '/json', dataType: 'json'});
assert(() => 42 == json.number);
assert(() => 'string' == json.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(PostJson)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('dollar');

let json = await $.ajax({
    url: '/json',
    method: 'POST',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({'foo': 'bar', 'bar': 42})
});
assert(() => 'bar' == json.foo);
assert(() => 42 == json.bar);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Require)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert } from 'noto:assert';
require('dollar');

assert(() => 'function' === typeof($.ajax));
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 'function' === typeof($.ajax));
require('dollar', {scope: 'cell'});
    )JS", context.get(), "cell-002");

    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => undefined === $.ajax);
require('dollar', {scope: 'notebook'});
    )JS", context.get(), "cell-003");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => 'function' == typeof($.ajax));

assert(() => throws(() => require('dollar', {scope: 1}), 'No matching function overload found'));
assert(() => throws(() => require('dollar', {scope: 'foo'}), 'invalid scope [foo]'));
    )JS", context.get(), "cell-004");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
