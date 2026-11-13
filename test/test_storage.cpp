#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Storage, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Storage)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => new Storage(''), TypeError, 'Storage constructor: invalid namespace []'));
assert(throws(() => new Storage('a b'), TypeError, 'Storage constructor: invalid namespace [a b]'));
assert(throws(() => new Storage('a/b'), TypeError, 'Storage constructor: invalid namespace [a/b]'));

const s = new Storage('foo');
assert(() => 0 == s.length);
assert(() => null == s.getItem('test'));

assert(throws(() => s.setItem('test'), TypeError));

s.setItem('test', 'one');
s.setItem('json', {'foo': 42});

assert(() => 2 == s.length);
assert(() => 'one' == s.getItem('test'));
assert(() => 42 == s.getItem('json').foo);

const t = new Storage('bar');
assert(() => 0 == t.length);

t.setItem('test', 'two');
assert(() => 1 == t.length);
assert(() => 'two' == t.getItem('test'));

t.clear();
print(t.length);
assert(() => 0 == t.length);

s.removeItem('test');
assert(() => 1 == s.length);
assert(() => null == s.getItem('test'));

s.removeItem('json');
assert(() => 0 == s.length);
assert(() => null == s.getItem('json'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NativeAPI)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { get, remove, setString, setValue } from 'storage.so';

const ns = 'native-api';

setString(ns, 'string', 'test');
setValue(ns, 'value', {foo: 42, nested: {enabled: true}});

assert(() => 'test' === get(ns, 'string'));
assert(() => 42 === get(ns, 'value').foo);
assert(() => true === get(ns, 'value').nested.enabled);

const storage = new Storage(ns);
assert(() => 'test' === storage.getItem('string'));
assert(() => 42 === storage.getItem('value').foo);

remove(ns, 'string');
remove(ns, 'value');

assert(() => null === get(ns, 'string'));
assert(() => null === get(ns, 'value'));
assert(() => 0 === storage.length);

setString(storage, 'string', 'test');
setValue(storage, 'value', {foo: 42, nested: {enabled: true}});

assert(() => 'test' === get(storage, 'string'));
assert(() => 42 === get(storage, 'value').foo);
assert(() => true === get(storage, 'value').nested.enabled);

assert(() => 'test' === storage.getItem('string'));
assert(() => 42 === storage.getItem('value').foo);

remove(storage, 'string');
remove(storage, 'value');

assert(() => null === get(storage, 'string'));
assert(() => null === get(storage, 'value'));
assert(() => 0 === storage.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Extend)
{
    eval(R"JS(
import { assert } from 'noto:assert';

class S extends Storage {
    constructor() {
        super('foo')
    }
    inc() {
        const i = super.getItem('counter') ?? 0;
        super.setItem('counter', i + 1);
        return i + 1;
    }
}

const s = new S();
assert(() => s instanceof S);
assert(() => s instanceof Storage);

assert(() => 0 == s.length);
assert(() => null == s.getItem('counter'));

assert(() => 1 == s.inc());
assert(() => 2 == s.inc());
assert(() => 2 == s.getItem('counter'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Require)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => require('storage'), TypeError));
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('storage', {ns: 'foo'});
assert(() => globalThis.localStorage instanceof Storage);
    )JS", context.get(), "cell-002");

    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => globalThis.localStorage instanceof Storage);
require('storage', {scope: 'cell', ns: 'foo'});
    )JS", context.get(), "cell-003");

    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => undefined === globalThis.localStorage);
require('storage', {scope: 'notebook', ns: 'foo'});
    )JS", context.get(), "cell-004");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => globalThis.localStorage instanceof Storage);

assert(throws(() => require('storage', {scope: 1}), TypeError, 'No matching function overload found'));
assert(throws(() => require('storage', {scope: 'foo'}), TypeError, 'invalid scope [foo]'));
assert(throws(() => require('storage', {ns: 'a b'}), TypeError, 'Storage constructor: invalid namespace [a b]'));
assert(throws(() => require('storage', {ns: 'a/b'}), TypeError, 'Storage constructor: invalid namespace [a/b]'));
    )JS", context.get(), "cell-005");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
