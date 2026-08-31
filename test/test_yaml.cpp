#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(YAML, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(API)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

assert(() => Object.isFrozen(YAML));
assert(() => 'YAML' === YAML.constructor.name);
assert(() => YAML instanceof YAML.constructor);
assert(() => 'function' === typeof YAML.parse);
assert(() => 'function' === typeof YAML.stringify);

let error;
try { YAML.parse = undefined; } catch(e) { error = e; }
assert(() => error instanceof TypeError);

error = undefined;
try { YAML.stringify = () => {}; } catch(e) { error = e; }
assert(() => error instanceof TypeError);

error = undefined;
try { new YAML.constructor(); } catch(e) { error = e; }
assert(() => error instanceof TypeError);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Scalars)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

assert(() => null === YAML.parse('null'));
assert(() => true === YAML.parse('true'));
assert(() => false === YAML.parse('NO'));
assert(() => 42 === YAML.parse('42'));
assert(() => 42 === YAML.parse('0x2a'));
assert(() => 1.5 === YAML.parse('1.5'));
assert(() => Infinity === YAML.parse('.inf'));
assert(() => Number.isNaN(YAML.parse('.nan')));
assert(() => 'true' === YAML.parse('"true"'));
assert(() => '42' === YAML.parse('!!str 42'));
assert(() => '2026-08-31' === YAML.parse('2026-08-31'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Collections)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

const value = YAML.parse(`
name: notojs
enabled: true
count: 42
nested:
  items: [one, false, 3.5, null]
  object:
    quoted: "42"
    empty: ""
"key with spaces": value
__proto__: safe
`);

assert(() => 'notojs' === value.name);
assert(() => true === value.enabled);
assert(() => 42 === value.count);
assert(() => Array.isArray(value.nested.items));
assert(() => 4 === value.nested.items.length);
assert(() => 'one' === value.nested.items[0]);
assert(() => false === value.nested.items[1]);
assert(() => 3.5 === value.nested.items[2]);
assert(() => null === value.nested.items[3]);
assert(() => '42' === value.nested.object.quoted);
assert(() => '' === value.nested.object.empty);
assert(() => 'value' === value['key with spaces']);
assert(() => 'safe' === value.__proto__);
assert(() => Object.prototype.hasOwnProperty.call(value, '__proto__'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Aliases)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

const value = YAML.parse(`
original: &items [one, two]
copy: *items
`);

assert(() => 'one' === value.original[0]);
assert(() => 'two' === value.copy[1]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Stringify)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

const source = {
    name: 'notojs',
    enabled: true,
    count: 42,
    nested: {
        items: ['one', false, 3.5, null],
        numericString: '42'
    }
};
const yaml = YAML.stringify(source);
const value = YAML.parse(yaml);

assert(() => 'string' === typeof yaml);
assert(() => !yaml.trimStart().startsWith('{'));
assert(() => !yaml.trimStart().startsWith('['));
assert(() => yaml.includes('name: notojs'));
assert(() => yaml.includes('\n'));
assert(() => 'notojs' === value.name);
assert(() => true === value.enabled);
assert(() => 42 === value.count);
assert(() => 'one' === value.nested.items[0]);
assert(() => false === value.nested.items[1]);
assert(() => 3.5 === value.nested.items[2]);
assert(() => null === value.nested.items[3]);
assert(() => '42' === value.nested.numericString);

const jsonCompatible = YAML.parse(YAML.stringify({
    omitted: undefined,
    items: [undefined, () => {}]
}));
assert(() => !Object.prototype.hasOwnProperty.call(jsonCompatible, 'omitted'));
assert(() => null === jsonCompatible.items[0]);
assert(() => null === jsonCompatible.items[1]);

const custom = YAML.parse(YAML.stringify({
    toJSON() { return {answer: 42}; }
}));
assert(() => 42 === custom.answer);

assert(() => undefined === YAML.stringify(undefined));
assert(() => undefined === YAML.stringify(() => {}));

const cyclic = {};
cyclic.self = cyclic;
let error;
try { YAML.stringify(cyclic); } catch(e) { error = e; }
assert(() => error instanceof TypeError);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Errors)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import YAML from 'yaml.so';

const error = source => {
    try { YAML.parse(source); } catch(error) { return error; }
};

assert(() => error('[one, two') instanceof SyntaxError);
assert(() => error('!!bool maybe') instanceof SyntaxError);
assert(() => error('? [one, two]\n: value') instanceof TypeError);
assert(() => 'YAML mappings require scalar keys' === error('? [one, two]\n: value').message);
assert(() => error('&items [*items]') instanceof TypeError);
assert(() => 'Cyclic YAML aliases are not supported' === error('&items [*items]').message);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
