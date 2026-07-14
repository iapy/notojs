#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Import, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Native)
{
    eval(R"JS(
import 'foobarbaz.so';
    )JS");

    BOOST_TEST(get_error() == "Module foobarbaz.so not found");
}

BOOST_AUTO_TEST_CASE(UnsupportedScheme)
{
    eval(R"JS(
import 'ftp://127.0.0.1/m.js';
    )JS");

    BOOST_TEST(get_error() == "Module ftp://127.0.0.1/m.js not loaded: unsupported scheme [ftp]");
}

BOOST_AUTO_TEST_CASE(HostNotFound)
{
    eval(R"JS(
import 'http://abracadabra/m.js';
    )JS");

    BOOST_TEST(get_error() == "Module http://abracadabra/m.js not loaded: Host not found (authoritative)");
}

BOOST_AUTO_TEST_CASE(NotFound)
{
    eval(R"JS(
import '/m.js';
    )JS");

    BOOST_TEST(get_error() == "Module /m.js not loaded: 404 Not Found");
}

BOOST_AUTO_TEST_CASE(HttpModule)
{
    eval(R"JS(
import { bar } from '/module.js';
bar();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "foo"));
    BOOST_TEST(!strcmp(out[1][0].GetString(), "bar"));
}

BOOST_AUTO_TEST_CASE(ScriptCapture)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const { x } = await require.script('/script.js', 'x');
assert(() => x == 1);
assert(() => typeof(globalThis['x']) == 'undefined');

const { y, z } = await require.script('/script.js', ['y', 'z']);
assert(() => y == 2);
assert(() => z == 3);
assert(() => typeof(globalThis['y']) == 'undefined');
assert(() => typeof(globalThis['z']) == 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ScriptContext)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const { foo } = await require.script('/script.js', 'foo', {'a': 5});
assert(() => foo() == 6);
assert(() => typeof(globalThis['foo']) == 'undefined');

const { bar, baz } = await require.script('/script.js', ['bar', 'baz'], {'a': 6});
assert(() => bar() == 8);
assert(() => baz() == 9);
assert(() => typeof(globalThis['bar']) == 'undefined');
assert(() => typeof(globalThis['baz']) == 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ScriptRequest)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const { x } = await require.script(new Request('/script.js'), 'x');
assert(() => x == 1);
assert(() => typeof(globalThis['x']) == 'undefined');

const { y, z } = await require.script(new Request('/script.js'), ['y', 'z']);
assert(() => y == 2);
assert(() => z == 3);
assert(() => typeof(globalThis['y']) == 'undefined');
assert(() => typeof(globalThis['z']) == 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ScriptRequestContext)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const { foo } = await require.script(new Request('/script.js'), 'foo', {'a': 5});
assert(() => foo() == 6);
assert(() => typeof(globalThis['foo']) == 'undefined');

const { bar, baz } = await require.script(new Request('/script.js'), ['bar', 'baz'], {'a': 6});
assert(() => bar() == 8);
assert(() => baz() == 9);
assert(() => typeof(globalThis['bar']) == 'undefined');
assert(() => typeof(globalThis['baz']) == 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ScriptHttpError)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => require.script('/doesnotexist.js', 'a'), 'Bad HTTP status code: 404'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(RequireMustache)
{
    eval(R"JS(
import { assert } from 'noto:assert';
const mustache = require('mustache');
assert(() => typeof(mustache) == 'object');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HttpInvalidURL)
{
    db();
    eval(R"JS(
import 'http:// ';
    )JS");

    BOOST_TEST(get_error() != std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
