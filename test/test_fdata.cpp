#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Fdata, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bind } from 'fdata.so';

const f = bind("foo");
assert(() => f.length === 1);
assert(() => "foo1" == f(1));
assert(() => "foobar" == f("bar"));
assert(() => "foo1" == f(1, "ignored"));
assert(() => "foobar" == f("bar", 42));
assert(() => throws(() => f()));
assert(() => throws(() => f({})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Self)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindSelf } from 'fdata.so';

const f = bindSelf();
const receiver = {f};
const result = receiver.f('foo', 'bar');
assert(() => result.self === receiver && result.value === 'foobar');
assert(() => f('foo', 1).self === undefined);
assert(() => f('foo', 1).value === 'foo1');
const nullReceiver = f.call(null, 'foo', 'bar');
assert(() => nullReceiver.self === null && nullReceiver.value === 'foobar');

const undefinedReceiver = f.call(undefined, 'foo', 'bar');
assert(() => undefinedReceiver.self === undefined && undefinedReceiver.value === 'foobar');

const numberReceiver = f.call(42, 'foo', 'bar');
assert(() => numberReceiver.self === 42 && numberReceiver.value === 'foobar');

const stringReceiver = f.call('receiver', 'foo', 'bar');
assert(() => stringReceiver.self === 'receiver' && stringReceiver.value === 'foobar');

const booleanReceiver = f.call(false, 'foo', 'bar');
assert(() => booleanReceiver.self === false && booleanReceiver.value === 'foobar');
assert(() => throws(() => f('foo')));
assert(() => throws(() => f('foo', {})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SelfWithCaptures)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindSelf } from 'fdata.so';

const f = bindSelf('foo');
const first = {};
const second = {};
const text = f.call(first, 'bar');
const number = f.call(second, 42);
assert(() => text.self === first && text.value === 'foobar');
assert(() => number.self === second && number.value === 'foo42');
assert(() => throws(() => f.call(first)));
assert(() => throws(() => f.call(first, {})));

const captured = bindSelf('foo', 'bar');
const result = captured.call(first);
assert(() => result.self === first && result.value === 'foobar');
assert(() => captured().self === undefined);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SelfExtraArguments)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { bindSelf } from 'fdata.so';

const self = {};
const unbound = bindSelf();
const partial = bindSelf('foo');
const captured = bindSelf('foo', 'bar');
assert(() => unbound.length === 2);
assert(() => partial.length === 1);
assert(() => captured.length === 0);

const text = unbound.call(self, 'foo', 'bar', 'ignored');
assert(() => text.self === self && text.value === 'foobar');
const number = partial.call(self, 42, 'ignored');
assert(() => number.self === self && number.value === 'foo42');
const result = captured.call(self, 'ignored');
assert(() => result.self === self && result.value === 'foobar');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(RequiredTailCaptures)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindRequired } from 'fdata.so';

const unbound = bindRequired();
const partial = bindRequired('p');
const captured = bindRequired('p', 7);
assert(() => unbound.length === 2);
assert(() => partial.length === 1);
assert(() => captured.length === 0);
assert(() => unbound('p', 7, 'a') === 'p7:1:a:a');
assert(() => partial(7, 'a', 'b') === 'p7:2:a:b');
assert(() => captured('a', 'b', 'c', 'd', 'e') === 'p7:5:a:e');
assert(() => captured('next') === 'p7:1:next:next');

// Capturing every fixed argument still leaves a required runtime tail.
assert(() => throws(() => unbound()));
assert(() => throws(() => unbound('p')));
assert(() => throws(() => unbound('p', 7)));
assert(() => throws(() => partial(7)));
assert(() => throws(() => captured()));
assert(() => throws(() => unbound(1, 7, 'a')));
assert(() => throws(() => partial('wrong', 'a')));
assert(() => throws(() => captured(1)));
assert(() => throws(() => captured('a', 'b', 'c', {})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(OptionalTailAndFixedValidation)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindOptional, bindRequired } from 'fdata.so';

const unbound = bindOptional();
const partial = bindOptional('p');
const captured = bindOptional('p', 7);
assert(() => unbound.length === 2 && partial.length === 1 && captured.length === 0);
assert(() => unbound('p', 7) === 'p7:0');
assert(() => partial(7, 'a', 'b', 'c') === 'p7:3:a:c');
assert(() => captured() === 'p7:0');
assert(() => captured('a') === 'p7:1:a:a');
assert(() => throws(() => unbound('p')));
assert(() => throws(() => partial({})));
assert(() => throws(() => captured('a', 'b', 42)));

// These strings pass check(), but must fail the callback's custom valid().
assert(() => throws(() => unbound('', 7), 'empty prefix'));
assert(() => throws(() => bindRequired()('', 7, 'a'), 'empty prefix'));
const invalidPartial = bindOptional('');
const invalidCaptured = bindRequired('', 7);
assert(() => throws(() => invalidPartial(7, 'a'), 'empty prefix'));
assert(() => throws(() => invalidCaptured('a'), 'empty prefix'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(VariadicSelfForwarding)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindRequiredSelf, bindOptionalSelf } from 'fdata.so';

const unbound = bindRequiredSelf();
const partial = bindRequiredSelf('p');
const captured = bindRequiredSelf('p', 7);
assert(() => unbound.length === 2 && partial.length === 1 && captured.length === 0);
const receiver = {f: unbound};
const first = receiver.f('p', 7, 'a', 'b', 'c');
assert(() => first.self === receiver && first.value === 'p7:3:a:c');
const second = partial.call(null, 7, 'a');
assert(() => second.self === null && second.value === 'p7:1:a:a');
const third = captured.call(42, 'a', 'b', 'c', 'd');
assert(() => third.self === 42 && third.value === 'p7:4:a:d');
assert(() => captured('a').self === undefined);
assert(() => throws(() => captured.call(receiver)));
assert(() => throws(() => partial.call(receiver, 'wrong', 'a')));
assert(() => throws(() => captured.call(receiver, 'a', {})));
const invalid = bindRequiredSelf('', 7);
assert(() => throws(() => invalid.call(receiver, 'a'), 'empty prefix'));

const optional = bindOptionalSelf('p', 7);
assert(() => optional.length === 0);
const empty = optional.call(receiver);
assert(() => empty.self === receiver && empty.value === 'p7:0');
const nonempty = optional.call(false, 'x', 'y');
assert(() => nonempty.self === false && nonempty.value === 'p7:2:x:y');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(MixedFixedAndVariadicOverloads)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { bindMixed } from 'fdata.so';

const unbound = bindMixed();
const partial = bindMixed('p');
const captured = bindMixed('p', 7);
// The fixed overload has three parameters; the variadic prefix has only two.
assert(() => unbound.length === 3 && partial.length === 2 && captured.length === 1);
assert(() => unbound('p', 7, 9) === 'fixed');
assert(() => partial(7, 9) === 'fixed');
assert(() => captured(9) === 'fixed');
assert(() => unbound('p', 7, 'a') === 'p7:1:a:a');
assert(() => partial(7, 'a', 'b', 'c') === 'p7:3:a:c');
assert(() => captured('a', 'b', 'c', 'd', 'e') === 'p7:5:a:e');
assert(() => throws(() => captured()));
assert(() => throws(() => captured({})));
assert(() => throws(() => captured('a', 'b', {})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(InvalidCaptureMetadata)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { rejectsInvalidCaptures } from 'fdata.so';

assert(() => rejectsInvalidCaptures());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
