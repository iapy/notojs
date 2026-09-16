#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Custom, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Creation)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Custom, consume, factory } from 'custom.so';

const a = new Custom();
assert(() => a instanceof Custom);

const b = new Custom({data: 'test'});
assert(() => b instanceof Custom);
assert(() => 'test' === b.data);

const c = factory();
assert(() => c instanceof Custom);
assert(() => undefined === c.data);

const d = factory('custom');
assert(() => d instanceof Custom);
assert(() => 'custom' === d.data);

const e = d.toJSON();
assert(() => 'Custom' == e.type);
assert(() => 'custom' == e.data);
assert(() => 'custom' == consume(d));
assert(() => throws(() => consume({})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(CopyErrors)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Custom } from 'custom.so';

function expectError(source, expected) {
    let caught;
    try { new Custom(source); } catch (error) { caught = error; }
    assert(() => caught === expected);
}

const enumerationError = new Error('enumeration failed');
expectError(new Proxy({}, {
    ownKeys() { throw enumerationError; }
}), enumerationError);

const getterError = new Error('getter failed');
expectError({
    data: 'copied before failure',
    get failing() { throw getterError; }
}, getterError);

const symbolError = new Error('symbol getter failed');
expectError({
    data: 'copied before failure',
    get [Symbol('failing')]() { throw symbolError; }
}, symbolError);

assert(() => new Custom({data: 'after errors'}).data === 'after errors');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(CopyProperties)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Custom } from 'custom.so';

const source = JSON.parse('{"__proto__":{"unexpected":true},"data":"test","intercepted":"own"}');
const symbol = Symbol('copied');
const hiddenSymbol = Symbol('hidden');
source[symbol] = {nested: true};
Object.defineProperty(source, 'hidden', {value: true});
Object.defineProperty(source, hiddenSymbol, {value: true});
Object.setPrototypeOf(source, {inherited: true});

Object.defineProperty(Custom.prototype, 'intercepted', {
    configurable: true,
    set(value) { throw new Error('inherited setter must not run'); }
});
try {
    const copy = new Custom(source);
    assert(() => copy instanceof Custom && Object.getPrototypeOf(copy) === Custom.prototype);
    assert(() => copy.data === 'test' && copy.intercepted === 'own');
    assert(() => copy.__proto__ === source.__proto__ && copy.unexpected === undefined);
    assert(() => copy[symbol] === source[symbol]);
    assert(() => !Object.hasOwn(copy, 'hidden') && !Object.hasOwn(copy, hiddenSymbol));
    assert(() => !Object.hasOwn(copy, 'inherited'));
    const protoDescriptor = Object.getOwnPropertyDescriptor(copy, '__proto__');
    assert(() => protoDescriptor.value === source.__proto__);
    assert(() => protoDescriptor.writable && protoDescriptor.enumerable && protoDescriptor.configurable);

    const interceptedDescriptor = Object.getOwnPropertyDescriptor(copy, 'intercepted');
    assert(() => interceptedDescriptor.value === source.intercepted);
    assert(() => interceptedDescriptor.writable && interceptedDescriptor.enumerable && interceptedDescriptor.configurable);

    const symbolDescriptor = Object.getOwnPropertyDescriptor(copy, symbol);
    assert(() => symbolDescriptor.value === source[symbol]);
    assert(() => symbolDescriptor.writable && symbolDescriptor.enumerable && symbolDescriptor.configurable);
} finally {
    delete Custom.prototype.intercepted;
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
