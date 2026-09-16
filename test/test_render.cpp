#include <boost/test/unit_test.hpp>
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
const wide = renderer.wide;
assert(() => typeof wide === 'function' && calls === 0);

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

BOOST_AUTO_TEST_CASE(InvalidViews)
{
    eval(R"JS(
import render from 'noto:render';
import { assert, throws } from 'noto:assert';

let calls = 0;
const renderer = render('notojs.Render/example.js', /^\d+\/\d+$/, () => ++calls);
assert(() => throws(() => renderer['bad'], 'render: invalid view bad'));
assert(() => throws(() => renderer['16/9x'], 'render: invalid view 16/9x'));
assert(() => throws(() => renderer[Symbol.iterator], 'render: expected a string'));
assert(() => calls === 0);
assert(() => renderer['16/9']().view === '16/9');
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

BOOST_AUTO_TEST_CASE(InvalidArguments)
{
    eval(R"JS(
import render from 'noto:render';
import { assert, throws } from 'noto:assert';

assert(() => throws(() => render('example', 'not a function')));
assert(() => throws(() => render(42, () => [])));
assert(() => throws(() => render('example', {}, () => []), 'render: expected a RegExp'));
assert(() => throws(() => render('example', 'wide', () => [])));
assert(() => throws(() => render('example', /^wide$/, null)));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
