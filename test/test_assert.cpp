#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Assert, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(ReturnType)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1);
    )JS");

    BOOST_TEST(get_error() == "Expecting bool: () => 1");
}

BOOST_AUTO_TEST_CASE(Success)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1 == 1);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Failure)
{
    eval(R"JS(
import { assert } from 'noto:assert';
assert(() => 1 == 2);
    )JS");

    BOOST_TEST(get_error() == "1 == 2");
}

BOOST_AUTO_TEST_CASE(Exception)
{
    eval(R"JS(
import { assert, AssertionError } from 'noto:assert';

let exception;
try {
    assert(() => false);
} catch(error) {
    exception = error;
}

assert(() => exception instanceof AssertionError);
assert(() => exception instanceof Error);
assert(() => exception.message === 'false');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => 1));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => 1))JS");
}

BOOST_AUTO_TEST_CASE(ThrowsSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => { throw ""; }));
assert(throws(() => { throw "Test"; }, 'Test'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMeasageSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => { throw new Error("message"); }, "message"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMeasageFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => { throw new Error("message"); }, "xxx"));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => { throw new Error("message"); }, "xxx"))JS");
}

BOOST_AUTO_TEST_CASE(AsyncThrowsSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var p = new Promise((resolve, reject) => {
    reject(new Error());
});
assert(throws(() => p));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var p = new Promise((resolve, reject) => {
    resolve();
});
assert(throws(() => p));
    )JS");

    BOOST_TEST(get_error() == "throws(() => p)");
}

BOOST_AUTO_TEST_CASE(AsyncThrowsMeasageSuccess)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
var p = new Promise((resolve, reject) => {
    reject(new Error('Test'));
});
assert(throws(() => p, 'Test'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsMeasageFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
var p = new Promise((resolve, reject) => {
    reject(new Error('Foo'));
});
assert(throws(() => p, 'Bar'));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => p, "Bar"))JS");
}

BOOST_AUTO_TEST_CASE(ThrowsDeferred)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

let calls = 0;
const callback = () => { ++calls; throw new TypeError('message'); };
const anyError = throws(callback);
const matchingMessage = throws(callback, 'message');
const matchingType = throws(callback, TypeError);
const matchingTypeAndMessage = throws(callback, TypeError, 'message');
assert(() => calls === 0);

assert(() => typeof anyError === 'function');
assert(() => anyError() === true);
assert(() => calls === 1);

assert(() => typeof matchingMessage === 'function');
assert(() => matchingMessage() === true);
assert(() => calls === 2);

assert(() => typeof matchingType === 'function');
assert(() => matchingType() === true);
assert(() => calls === 3);

assert(() => typeof matchingTypeAndMessage === 'function');
assert(() => matchingTypeAndMessage() === true);
assert(() => calls === 4);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsOverloads)
{
    eval(R"JS(
import { assert, throws, AssertionError } from 'noto:assert';

const callback = () => { throw new TypeError('message'); };
assert(throws(callback));
assert(throws(callback, 'message'));
assert(throws(callback, TypeError));
assert(throws(callback, TypeError, 'message'));
assert(throws(callback, Error));

class CustomTypeError extends TypeError {}
assert(throws(() => { throw new CustomTypeError('message'); }, TypeError));
assert(throws(() => { throw new CustomTypeError('message'); }, TypeError, 'message'));
assert(throws(() => assert(() => false), AssertionError));
assert(throws(() => assert(() => false), AssertionError, 'false'));
assert(throws(() => assert(() => false), Error));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMismatches)
{
    eval(R"JS(
import { assert, throws, AssertionError } from 'noto:assert';

const callback = () => { throw new TypeError('message'); };
assert(() => !throws(callback, RangeError)());
assert(() => !throws(callback, 'mess')());
assert(() => !throws(callback, 'Message')());
assert(() => !throws(callback, RangeError, 'message')());
assert(() => !throws(callback, TypeError, 'other')());
assert(throws(() => assert(throws(callback, RangeError)), AssertionError));

assert(() => !throws(() => undefined)());
assert(() => !throws(() => undefined, 'message')());
assert(() => !throws(() => undefined, TypeError)());
assert(() => !throws(() => undefined, TypeError, 'message')());

assert(() => !throws(() => null)());
assert(() => !throws(() => null, 'message')());
assert(() => !throws(() => null, TypeError)());
assert(() => !throws(() => null, TypeError, 'message')());

assert(() => !throws(() => '')());
assert(() => !throws(() => '', 'message')());
assert(() => !throws(() => '', TypeError)());
assert(() => !throws(() => '', TypeError, 'message')());

assert(() => !throws(() => 'message')());
assert(() => !throws(() => 'message', 'message')());
assert(() => !throws(() => 'message', TypeError)());
assert(() => !throws(() => 'message', TypeError, 'message')());

assert(() => !throws(() => 42)());
assert(() => !throws(() => 42, 'message')());
assert(() => !throws(() => 42, TypeError)());
assert(() => !throws(() => 42, TypeError, 'message')());

assert(() => !throws(() => new TypeError('message'))());
assert(() => !throws(() => new TypeError('message'), 'message')());
assert(() => !throws(() => new TypeError('message'), TypeError)());
assert(() => !throws(() => new TypeError('message'), TypeError, 'message')());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsPrimitives)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(throws(() => { throw null; }));
assert(throws(() => { throw null; }, 'null'));
assert(() => !throws(() => { throw null; }, 'other')());
assert(() => !throws(() => { throw null; }, Error)());
assert(() => !throws(() => { throw null; }, Error, 'null')());

assert(throws(() => { throw undefined; }));
assert(throws(() => { throw undefined; }, 'undefined'));
assert(() => !throws(() => { throw undefined; }, 'other')());
assert(() => !throws(() => { throw undefined; }, Error)());
assert(() => !throws(() => { throw undefined; }, Error, 'undefined')());

assert(throws(() => { throw ''; }));
assert(throws(() => { throw ''; }, ''));
assert(() => !throws(() => { throw ''; }, 'other')());
assert(() => !throws(() => { throw ''; }, Error)());
assert(() => !throws(() => { throw ''; }, Error, '')());

assert(throws(() => { throw 'message'; }));
assert(throws(() => { throw 'message'; }, 'message'));
assert(() => !throws(() => { throw 'message'; }, 'other')());
assert(() => !throws(() => { throw 'message'; }, Error)());
assert(() => !throws(() => { throw 'message'; }, Error, 'message')());

assert(throws(() => { throw 0; }));
assert(throws(() => { throw 0; }, '0'));
assert(() => !throws(() => { throw 0; }, 'other')());
assert(() => !throws(() => { throw 0; }, Error)());
assert(() => !throws(() => { throw 0; }, Error, '0')());

assert(throws(() => { throw 42; }));
assert(throws(() => { throw 42; }, '42'));
assert(() => !throws(() => { throw 42; }, 'other')());
assert(() => !throws(() => { throw 42; }, Error)());
assert(() => !throws(() => { throw 42; }, Error, '42')());

assert(throws(() => { throw true; }));
assert(throws(() => { throw true; }, 'true'));
assert(() => !throws(() => { throw true; }, 'other')());
assert(() => !throws(() => { throw true; }, Error)());
assert(() => !throws(() => { throw true; }, Error, 'true')());

assert(throws(() => { throw false; }));
assert(throws(() => { throw false; }, 'false'));
assert(() => !throws(() => { throw false; }, 'other')());
assert(() => !throws(() => { throw false; }, Error)());
assert(() => !throws(() => { throw false; }, Error, 'false')());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsTypeMessageFailureDescription)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(() => { throw new TypeError('Foo'); }, TypeError, 'Bar'));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(() => { throw new TypeError('Foo'); }, TypeError, "Bar"))JS");
}

BOOST_AUTO_TEST_CASE(AsyncThrowsDeferred)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

let calls = 0;
const callback = async () => { ++calls; throw new TypeError('message'); };
const anyError = throws(callback);
const matchingMessage = throws(callback, 'message');
const matchingType = throws(callback, TypeError);
const matchingTypeAndMessage = throws(callback, TypeError, 'message');
assert(() => calls === 0);

assert(() => typeof anyError === 'function');
assert(() => anyError() === true);
assert(() => calls === 1);

assert(() => typeof matchingMessage === 'function');
assert(() => matchingMessage() === true);
assert(() => calls === 2);

assert(() => typeof matchingType === 'function');
assert(() => matchingType() === true);
assert(() => calls === 3);

assert(() => typeof matchingTypeAndMessage === 'function');
assert(() => matchingTypeAndMessage() === true);
assert(() => calls === 4);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsOverloads)
{
    eval(R"JS(
import { assert, throws, AssertionError } from 'noto:assert';

const callback = async () => { throw new TypeError('message'); };
assert(throws(callback));
assert(throws(callback, 'message'));
assert(throws(callback, TypeError));
assert(throws(callback, TypeError, 'message'));
assert(throws(callback, Error));

class CustomTypeError extends TypeError {}
assert(throws(async () => { throw new CustomTypeError('message'); }, TypeError));
assert(throws(async () => { throw new CustomTypeError('message'); }, TypeError, 'message'));
assert(throws(async () => assert(() => false), AssertionError));
assert(throws(async () => assert(() => false), AssertionError, 'false'));
assert(throws(async () => assert(() => false), Error));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsMismatches)
{
    eval(R"JS(
import { assert, throws, AssertionError } from 'noto:assert';

const callback = async () => { throw new TypeError('message'); };
assert(() => !throws(callback, RangeError)());
assert(() => !throws(callback, 'mess')());
assert(() => !throws(callback, 'Message')());
assert(() => !throws(callback, RangeError, 'message')());
assert(() => !throws(callback, TypeError, 'other')());
assert(throws(() => assert(throws(callback, RangeError)), AssertionError));

assert(() => !throws(async () => undefined)());
assert(() => !throws(async () => undefined, 'message')());
assert(() => !throws(async () => undefined, TypeError)());
assert(() => !throws(async () => undefined, TypeError, 'message')());

assert(() => !throws(async () => null)());
assert(() => !throws(async () => null, 'message')());
assert(() => !throws(async () => null, TypeError)());
assert(() => !throws(async () => null, TypeError, 'message')());

assert(() => !throws(async () => '')());
assert(() => !throws(async () => '', 'message')());
assert(() => !throws(async () => '', TypeError)());
assert(() => !throws(async () => '', TypeError, 'message')());

assert(() => !throws(async () => 'message')());
assert(() => !throws(async () => 'message', 'message')());
assert(() => !throws(async () => 'message', TypeError)());
assert(() => !throws(async () => 'message', TypeError, 'message')());

assert(() => !throws(async () => 42)());
assert(() => !throws(async () => 42, 'message')());
assert(() => !throws(async () => 42, TypeError)());
assert(() => !throws(async () => 42, TypeError, 'message')());

assert(() => !throws(async () => new TypeError('message'))());
assert(() => !throws(async () => new TypeError('message'), 'message')());
assert(() => !throws(async () => new TypeError('message'), TypeError)());
assert(() => !throws(async () => new TypeError('message'), TypeError, 'message')());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsPrimitives)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(throws(async () => { throw null; }));
assert(throws(async () => { throw null; }, 'null'));
assert(() => !throws(async () => { throw null; }, 'other')());
assert(() => !throws(async () => { throw null; }, Error)());
assert(() => !throws(async () => { throw null; }, Error, 'null')());

assert(throws(async () => { throw undefined; }));
assert(throws(async () => { throw undefined; }, 'undefined'));
assert(() => !throws(async () => { throw undefined; }, 'other')());
assert(() => !throws(async () => { throw undefined; }, Error)());
assert(() => !throws(async () => { throw undefined; }, Error, 'undefined')());

assert(throws(async () => { throw ''; }));
assert(throws(async () => { throw ''; }, ''));
assert(() => !throws(async () => { throw ''; }, 'other')());
assert(() => !throws(async () => { throw ''; }, Error)());
assert(() => !throws(async () => { throw ''; }, Error, '')());

assert(throws(async () => { throw 'message'; }));
assert(throws(async () => { throw 'message'; }, 'message'));
assert(() => !throws(async () => { throw 'message'; }, 'other')());
assert(() => !throws(async () => { throw 'message'; }, Error)());
assert(() => !throws(async () => { throw 'message'; }, Error, 'message')());

assert(throws(async () => { throw 0; }));
assert(throws(async () => { throw 0; }, '0'));
assert(() => !throws(async () => { throw 0; }, 'other')());
assert(() => !throws(async () => { throw 0; }, Error)());
assert(() => !throws(async () => { throw 0; }, Error, '0')());

assert(throws(async () => { throw 42; }));
assert(throws(async () => { throw 42; }, '42'));
assert(() => !throws(async () => { throw 42; }, 'other')());
assert(() => !throws(async () => { throw 42; }, Error)());
assert(() => !throws(async () => { throw 42; }, Error, '42')());

assert(throws(async () => { throw true; }));
assert(throws(async () => { throw true; }, 'true'));
assert(() => !throws(async () => { throw true; }, 'other')());
assert(() => !throws(async () => { throw true; }, Error)());
assert(() => !throws(async () => { throw true; }, Error, 'true')());

assert(throws(async () => { throw false; }));
assert(throws(async () => { throw false; }, 'false'));
assert(() => !throws(async () => { throw false; }, 'other')());
assert(() => !throws(async () => { throw false; }, Error)());
assert(() => !throws(async () => { throw false; }, Error, 'false')());

assert(throws(() => Promise.reject(undefined)));
assert(throws(() => Promise.reject(undefined), 'undefined'));
assert(() => !throws(() => Promise.resolve(undefined))());
assert(() => !throws(() => Promise.resolve(undefined), 'undefined')());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsTypeMessageFailureDescription)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
assert(throws(async () => { throw new TypeError('Foo'); }, TypeError, 'Bar'));
    )JS");

    BOOST_TEST(get_error() == R"JS(throws(async () => { throw new TypeError('Foo'); }, TypeError, "Bar"))JS");
}

BOOST_AUTO_TEST_CASE(ThrowsTypeNameCoercionFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const sentinel = new Error('name conversion');
function ExpectedError() {}
Object.defineProperty(ExpectedError, 'name', {
    value: { toString() { throw sentinel; } }
});

let caught;
try {
    assert(throws(() => {}, ExpectedError));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsRejectedWrapperHandled)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const sentinel = new Error('wrapper failure');
const source = Promise.resolve();
// Fail the wrapper independently of the callback's fulfilled promise.
source.then = () => Promise.reject(sentinel);

let caught;
try {
    assert(throws(() => source));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);
assert(throws(async () => { throw new Error('next rejection'); }, Error, 'next rejection'));
    )JS");

    // Catching the synchronous exception must also clear the unhandled rejection.
    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsSpeciesJobFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const sentinel = new Error('species resolve failure');
let resolveCalls = 0;
class BrokenPromise extends Promise {
    static get [Symbol.species]() {
        return function(executor) {
            executor(() => { ++resolveCalls; throw sentinel; }, () => {});
            // The reaction job throws while its returned promise stays pending.
            return new Promise(() => {});
        };
    }
}

let caught;
try {
    assert(throws(() => new BrokenPromise(resolve => resolve())));
} catch (error) {
    caught = error;
}
assert(() => resolveCalls === 1);
assert(() => caught === sentinel);
// A subsequent wait must not rethrow the saved job exception.
assert(throws(async () => { throw new Error('next rejection'); }, Error, 'next rejection'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsMessageGetterFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const sentinel = new Error('message getter failure');
const reason = new Error();
Object.defineProperty(reason, 'message', {
    get() { throw sentinel; }
});

let caught;
try {
    assert(throws(() => { throw reason; }, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(() => { throw reason; }, Error, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(async () => { await Promise.resolve(); throw reason; }, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(async () => { await Promise.resolve(); throw reason; }, Error, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ThrowsHasInstanceFailure)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const sentinel = new Error('instance matcher failure');
class ExpectedError {
    static [Symbol.hasInstance]() { throw sentinel; }
}
const reason = new Error('message');

let caught;
try {
    assert(throws(() => { throw reason; }, ExpectedError));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(() => { throw reason; }, ExpectedError, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(async () => { await Promise.resolve(); throw reason; }, ExpectedError));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);

caught = undefined;
try {
    assert(throws(async () => { await Promise.resolve(); throw reason; }, ExpectedError, 'message'));
} catch (error) {
    caught = error;
}
assert(() => caught === sentinel);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(AsyncThrowsRejectedThenableNotAssimilated)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

let thenReads = 0;
let thenCalls = 0;
class ThenableError extends Error {
    get then() {
        ++thenReads;
        return resolve => {
            ++thenCalls;
            resolve(new Error('assimilated'));
        };
    }
}
const reason = new ThenableError('original rejection');

assert(throws(() => Promise.reject(reason)));
assert(throws(() => Promise.reject(reason), 'original rejection'));
assert(throws(() => Promise.reject(reason), ThenableError));
assert(throws(() => Promise.reject(reason), ThenableError, 'original rejection'));

assert(throws(async () => { await Promise.resolve(); throw reason; }));
assert(throws(async () => { await Promise.resolve(); throw reason; }, 'original rejection'));
assert(throws(async () => { await Promise.resolve(); throw reason; }, ThenableError));
assert(throws(async () => { await Promise.resolve(); throw reason; }, ThenableError, 'original rejection'));
assert(() => thenReads === 0);
assert(() => thenCalls === 0);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
