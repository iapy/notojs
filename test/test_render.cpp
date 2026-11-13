#include <boost/test/unit_test.hpp>
#include <memory.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Render, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(WithoutRegex)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

let calls = 0;
const renderer = render('notojs.Render/example.js', function (a, b) {
    ++calls;
    return this.prefix + (a + b);
});
assert(() => typeof renderer === 'function' && calls === 0);

const self = {prefix: 'sum: '};
const result = renderer.call(self, 2, 3);
assert(() => calls === 1);
assert(() => result.type === 'notojs.Render/example.js');
assert(() => result.data === 'sum: 5');
assert(() => !('view' in result));
assert(() => /^[0-9a-f]{8}$/.test(result.sign));
assert(() => renderer.call(self, 2, 3).sign === result.sign);
assert(() => renderer.call(self, 2, 4).sign !== result.sign);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(WithPrototype)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

const prototype = {inherited: 'prototype'};
let receiver;
const renderer = render(prototype, 'notojs.Render/prototype.js', function (value) {
    receiver = this;
    return value;
});
assert(() => render.length === 2);
assert(() => typeof renderer === 'function');

const self = {};
const result = renderer.call(self, {answer: 42});
assert(() => receiver === self);
assert(() => Object.getPrototypeOf(result) === prototype);
assert(() => result.inherited === 'prototype');
assert(() => result.type === 'notojs.Render/prototype.js');
assert(() => result.data.answer === 42);
assert(() => !('view' in result));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(WithRegex)
{
    eval(R"JS(
import { render } from 'noto:render';
import { assert } from 'noto:assert';

let calls = 0;
const renderer = render('notojs.Render/example.js', /^(wide|compact)$/g, (a, b) => {
    ++calls;
    return [a, b];
});
const rendererView = Object.getOwnPropertyDescriptor(renderer, 'render.v');
const rendererPrototype = Object.getOwnPropertyDescriptor(renderer, 'render.p');
assert(() => rendererView.value === undefined && !rendererView.enumerable);
assert(() => rendererPrototype.value === undefined && !rendererPrototype.enumerable);
assert(() => !Object.keys(renderer).includes('render.v'));
assert(() => !Object.keys(renderer).includes('render.p'));

const wide = renderer.wide;
const wideView = Object.getOwnPropertyDescriptor(wide, 'render.v');
const widePrototype = Object.getOwnPropertyDescriptor(wide, 'render.p');
assert(() => typeof wide === 'function' && calls === 0);
assert(() => wideView.value === 'wide' && !wideView.enumerable);
assert(() => widePrototype.value === undefined && !widePrototype.enumerable);
assert(() => !Object.keys(wide).includes('render.v'));
assert(() => !Object.keys(wide).includes('render.p'));

const plain = renderer(1, 2);
assert(() => plain.type === 'notojs.Render/example.js');
assert(() => plain.data.join(',') === '1,2');
assert(() => !('view' in plain));
assert(() => /^[0-9a-f]{8}$/.test(plain.sign));

const viewed = wide(3, 4);
assert(() => viewed.type === plain.type && viewed.sign === plain.sign);
assert(() => viewed.data.join(',') === '3,4' && viewed.view === 'wide');
assert(() => renderer.compact(5, 6).view === 'compact');
assert(() => renderer.wide(7, 8).view === 'wide');
assert(() => !('view' in renderer(9, 10)));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(WithRegexAndPrototype)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

const prototype = {inherited: 'prototype'};
const renderer = render(prototype, 'notojs.Render/prototype.js', /^(wide|compact)$/, value => value);
const rendererView = Object.getOwnPropertyDescriptor(renderer, 'render.v');
const rendererPrototype = Object.getOwnPropertyDescriptor(renderer, 'render.p');
assert(() => rendererView.value === undefined && !rendererView.enumerable);
assert(() => rendererPrototype.value === prototype && !rendererPrototype.enumerable);
assert(() => !Object.keys(renderer).includes('render.v'));
assert(() => !Object.keys(renderer).includes('render.p'));

const plain = renderer('plain');
assert(() => Object.getPrototypeOf(plain) === prototype);
assert(() => plain.inherited === 'prototype');
assert(() => plain.data === 'plain' && !('view' in plain));

const wideRenderer = renderer.wide;
const wideView = Object.getOwnPropertyDescriptor(wideRenderer, 'render.v');
const widePrototype = Object.getOwnPropertyDescriptor(wideRenderer, 'render.p');
assert(() => wideView.value === 'wide' && !wideView.enumerable);
assert(() => widePrototype.value === prototype && !widePrototype.enumerable);
assert(() => !Object.keys(wideRenderer).includes('render.v'));
assert(() => !Object.keys(wideRenderer).includes('render.p'));

const wide = wideRenderer('view');
assert(() => Object.getPrototypeOf(wide) === prototype);
assert(() => wide.inherited === 'prototype');
assert(() => wide.data === 'view' && wide.view === 'wide');

const compact = renderer.compact('other view');
assert(() => Object.getPrototypeOf(compact) === prototype);
assert(() => compact.view === 'compact');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(InvalidViews)
{
    eval(R"JS(
import render from 'noto:render';
import { assert, throws } from 'noto:assert';

let calls = 0;
const renderer = render('notojs.Render/example.js', /^\d+\/\d+$/, () => ++calls);
assert(throws(() => renderer['bad'], TypeError, 'render: invalid view bad'));
assert(throws(() => renderer['16/9x'], TypeError, 'render: invalid view 16/9x'));
assert(() => renderer[Symbol.iterator] === undefined);
assert(() => calls === 0);
assert(() => renderer['16/9']().view === '16/9');
assert(() => renderer['16/9']['16/9'] === undefined);
assert(() => calls === 1);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

const error = new Error('callback failed');
const fail = () => { throw error; };
const plain = render('notojs.Render/example.js', fail);
const withRegex = render('notojs.Render/example.js', /^wide$/, fail);
const viewed = withRegex.wide;

let plainError;
try { plain(); } catch (e) { plainError = e; }
assert(() => plainError === error);

let regexError;
try { withRegex(); } catch (e) { regexError = e; }
assert(() => regexError === error);

let viewError;
try { viewed(); } catch (e) { viewError = e; }
assert(() => viewError === error);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(VariadicArguments)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

let receiver;
let received;
const renderer = render('notojs.Render/example.js', function (...args) {
    receiver = this;
    received = args;
    return args.length;
});
assert(() => renderer.length === 0);
assert(() => renderer().data === 0);
assert(() => receiver === undefined && received.length === 0);
assert(() => renderer(42).data === 1);
assert(() => received[0] === 42);

const self = {};
const object = {};
const symbol = Symbol('argument');
assert(() => renderer.call(self, 1, 'two', null, undefined, object, symbol).data === 6);
assert(() => receiver === self);
assert(() => received.length === 6);
assert(() => received[0] === 1);
assert(() => received[1] === 'two');
assert(() => received[2] === null);
assert(() => received[3] === undefined);
assert(() => received[4] === object);
assert(() => received[5] === symbol);
assert(() => renderer.call(null).data === 0);
assert(() => receiver === null);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(VariadicViews)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

let receiver;
let received;
const renderer = render('notojs.Render/example.js', /^wide$/, function (...args) {
    receiver = this;
    received = args;
    return args.length;
});
const wide = renderer.wide;
assert(() => wide.length === 0);
assert(() => wide().data === 0);
assert(() => receiver === undefined && received.length === 0);
assert(() => wide(42).data === 1);
assert(() => received[0] === 42);

const self = {};
const object = {};
const symbol = Symbol('argument');
const result = wide.call(self, 1, 'two', null, undefined, object, symbol);
assert(() => result.data === 6 && result.view === 'wide');
assert(() => receiver === undefined);
assert(() => received.length === 6);
assert(() => received[0] === 1);
assert(() => received[1] === 'two');
assert(() => received[2] === null);
assert(() => received[3] === undefined);
assert(() => received[4] === object);
assert(() => received[5] === symbol);

const plain = Reflect.apply(renderer, self, [1, 2, 3, 4, 5]);
assert(() => plain.data === 5 && !('view' in plain));
assert(() => receiver === self);
assert(() => received.length === 5 && received[4] === 5);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Block)
{
    eval(R"JS(
import { render, Block } from 'noto:render';
import { assert, throws } from 'noto:assert';

const b = render('notojs.Render/example.js', Block, o => o);

print(b({x: 24}));
print(b({x: 42}));
assert(throws(() => print(b({}), b({})), RangeError));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(BlockDataLifetime)
{
    eval(R"JS(
import { render, Block } from 'noto:render';
import { assert } from 'noto:assert';

const renderer = render('notojs.Render/lifetime.js', Block, o => o);
for (let i = 0; i < 4; ++i) {
    const data = {nested: {value: 'retained'}};
    const block = renderer(data);
    assert(() => block instanceof Block);
    print(block);
    print(block);
    assert(() => block.type === 'notojs.Render/lifetime.js');
    assert(() => block.data === data && data.nested.value === 'retained');
    const next = renderer('retained string');
    print(next);
    print(next);
    assert(() => next.data === 'retained string');
    assert(() => next.type === block.type);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FinalDataLifetime)
{
    eval(R"JS(
import { render, Final } from 'noto:render';
import { assert, throws } from 'noto:assert';

const renderer = render('notojs.Render/final.js', Final, o => o);
const data = {nested: {value: 'retained'}};
const final = renderer(data);
assert(() => final instanceof Final);
print(final);
assert(throws(() => print(final), TypeError, 'object is not extensible'));
const next = renderer('retained string');
assert(throws(() => print(next), TypeError, 'object is not extensible'));
assert(() => final.type === 'notojs.Render/final.js' && next.type === final.type);
assert(() => final.data === data && data.nested.value === 'retained');
assert(() => next.data === 'retained string');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(DeferredPrototype)
{
    eval(R"JS(
import render, { Block, Final } from 'noto:render';
import { assert } from 'noto:assert';

const prototype = {
    toJSON() {
        return {prototype: this.data.kind, type: this.type};
    }
};
const blockRenderer = render(prototype, 'notojs.Render/block-prototype.js', Block, value => value);
const finalRenderer = render(prototype, 'notojs.Render/final-prototype.js', Final, value => value);
const block = blockRenderer({kind: 'block'});
const final = finalRenderer({kind: 'final'});

const blockPrototype = Object.getOwnPropertyDescriptor(block, 'render.p');
const finalPrototype = Object.getOwnPropertyDescriptor(final, 'render.p');
assert(() => block instanceof Block && final instanceof Final);
assert(() => blockPrototype.value === prototype && !blockPrototype.enumerable);
assert(() => finalPrototype.value === prototype && !finalPrototype.enumerable);
assert(() => !Object.keys(block).includes('render.p'));
assert(() => !Object.keys(final).includes('render.p'));

print(block);
print(final);
    )JS");

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 2u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_REQUIRE(out[1].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0]["prototype"].GetString(), "block"));
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Render/block-prototype.js"));
    BOOST_TEST(!strcmp(out[1][0]["prototype"].GetString(), "final"));
    BOOST_TEST(!strcmp(out[1][0]["type"].GetString(), "notojs.Render/final-prototype.js"));
}

BOOST_AUTO_TEST_CASE(InvalidArguments)
{
    eval(R"JS(
import render from 'noto:render';
import { assert, throws } from 'noto:assert';

assert(throws(() => render('example', 'not a function'), TypeError));
assert(throws(() => render(42, () => []), TypeError));
assert(throws(() => render('example', {}, () => []), TypeError, 'render: expected a RegExp'));
assert(throws(() => render('example', 'wide', () => []), TypeError));
assert(throws(() => render('example', /^wide$/, null), TypeError));
assert(throws(() => render(null, 'example', () => []), TypeError));
assert(throws(() => render({}, 42, () => []), TypeError));
assert(throws(() => render({}, 'example', null), TypeError));
assert(throws(() => render(null, 'example', /^wide$/, () => []), TypeError));
assert(throws(() => render({}, 'example', {}, () => []), TypeError, 'render: expected a RegExp'));
assert(throws(() => render({}, 'example', /^wide$/, null), TypeError));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ObjectBacked)
{
    eval(R"JS(
import render from 'noto:render';
import { assert } from 'noto:assert';

const Proxy = globalThis.Proxy;
globalThis.Proxy = function () { throw new Error('Proxy must not be used'); };
try {
    const renderer = render('notojs.Render/object.js', /^(function|regex)$/, function (...args) {
        return [this, ...args];
    });
    assert(() => typeof renderer === 'function' && renderer instanceof Function);
    const receiver = {};
    const plain = Reflect.apply(renderer, receiver, [1, 2]);
    assert(() => plain.data[0] === receiver && plain.data[1] === 1 && plain.data[2] === 2);
    assert(() => !('view' in plain));
    for (const view of ['function', 'regex']) {
        const result = renderer[view](3);
        assert(() => result.view === view);
        assert(() => result.data[0] === undefined && result.data[1] === 3);
    }
} finally {
    globalThis.Proxy = Proxy;
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(CapturedLifetime)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    BOOST_REQUIRE(eval(R"JS(
import render from 'noto:render';

export const renderLifetime = (() => {
    const regex = /^wide$/g;
    const callback = value => ({value});
    const renderer = render('notojs.Render/lifetime.js', regex, callback);
    regex.owner = renderer;
    callback.owner = renderer;
    return {renderer, view: renderer.wide, weak: new WeakRef(renderer)};
})();
    )JS", context.get(), "render-lifetime-1") == boost::beast::http::status::ok);

    JS_RunGC(JS_GetRuntime(context.get()));

    BOOST_REQUIRE(eval(R"JS(
import { assert } from 'noto:assert';

assert(() => renderLifetime.renderer(1).data.value === 1);
assert(() => renderLifetime.renderer.wide(2).view === 'wide');
renderLifetime.renderer = null;
    )JS", context.get(), "render-lifetime-2") == boost::beast::http::status::ok);

    JS_RunGC(JS_GetRuntime(context.get()));

    BOOST_REQUIRE(eval(R"JS(
import { assert } from 'noto:assert';

const result = renderLifetime.view(3);
assert(() => result.data.value === 3 && result.view === 'wide');
renderLifetime.view = null;
    )JS", context.get(), "render-lifetime-3") == boost::beast::http::status::ok);

    JS_RunGC(JS_GetRuntime(context.get()));

    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => renderLifetime.weak.deref() === undefined);
    )JS", context.get(), "render-lifetime-4");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
