#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Storage, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Storage)
{
    db();
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(() => throws(() => new Storage(''), 'Storage constructor: invalid namespace []'));
assert(() => throws(() => new Storage('a b'), 'Storage constructor: invalid namespace [a b]'));
assert(() => throws(() => new Storage('a/b'), 'Storage constructor: invalid namespace [a/b]'));

const s = new Storage('foo');
assert(() => 0 == s.length);
assert(() => null == s.getItem('test'));

assert(() => throws(() => s.setItem('test')));

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

BOOST_AUTO_TEST_CASE(Extend)
{
    db();
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

BOOST_AUTO_TEST_CASE(Attach)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';

const st = new Storage('foo').attach();
assert(() => st === localStorage);
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => undefined === globalThis['localStorage']);
const st = new Storage('bar').attach({notebook: true});
    )JS", context.get(), "cell-002");

    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => localStorage instanceof Storage);
    )JS", context.get(), "cell-003");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
