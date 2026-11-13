#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Vararg, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Const)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vararg } from 'vararg.so';

const v = new Vararg();
assert(throws(() => v.cappend(), TypeError, "No matching function overload found"));
assert(throws(() => v.cappend('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => v.cappend('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => v.cappend(1, {}), TypeError, "No matching function overload found"));
assert(throws(() => v.cappend(1, 'a', 1), TypeError, "No matching function overload found"));
assert(throws(() => v.cappend(1, 1, 'a'), TypeError, "No matching function overload found"));

v.append('a');
assert(() => 'a b' == v.cappend('b'));
assert(() => 'a' == v.value);

assert(() => 'a b c' == v.cappend('b', 'c'));
assert(() => 'a' == v.value);

assert(() => 'a 0' == v.cappend(0));
assert(() => 'a' == v.value);

assert(() => 'a 0 b c' == v.cappend(0, 'b', 'c'));
assert(() => 'a' == v.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NoConst)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vararg } from 'vararg.so';

const v = new Vararg();
assert(throws(() => v.append(), TypeError, "No matching function overload found"));
assert(throws(() => v.append('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => v.append('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => v.append(1, {}), TypeError, "No matching function overload found"));
assert(throws(() => v.append(1, 'a', 1), TypeError, "No matching function overload found"));
assert(throws(() => v.append(1, 1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => v.eappend(1, {}, 1, 'a'), TypeError, "No matching function overload found"));

v.append('a');
assert(() => 'a' == v.value);

v.append('b', 'c');
assert(() => 'a b c' == v.value);

v.append(0);
assert(() => 'a b c 0' == v.value);

v.append(1, 'x', 'y', 'z');
assert(() => 'a b c 0 1 x y z' == v.value);

v.eappend(2, 'd', 3, 'e', 4, 'f');
assert(() => 'a b c 0 1 x y z 2 d 3 e 4 f' == v.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctions)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { optional, required } from 'vararg.so';

assert(() => optional('a') === 'a');
assert(() => optional('a', 'b') === 'a b');
assert(() => optional('a', 'b', 'c', 'd', 'e') === 'a b c d e');
assert(throws(() => optional(), TypeError, "No matching function overload found"));
assert(throws(() => optional(1), TypeError, "No matching function overload found"));
assert(throws(() => optional({}), TypeError, "No matching function overload found"));
assert(throws(() => optional(1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => optional('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => optional('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => optional('a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => optional('a', 'b', 'c', {}), TypeError, "No matching function overload found"));

// String::check succeeds, but the fixed prefix's valid() must still run.
assert(throws(() => optional(''), TypeError, "empty prefix"));
assert(throws(() => optional('', 'a', 'b'), TypeError, "empty prefix"));

assert(() => required('a') === 'a');
assert(() => required('a', 'b') === 'a b');
assert(() => required('a', 'b', 'c', 'd', 'e') === 'a b c d e');
assert(() => required('') === '');
assert(throws(() => required(), TypeError, "No matching function overload found"));
assert(throws(() => required(1), TypeError, "No matching function overload found"));
assert(throws(() => required({}), TypeError, "No matching function overload found"));
assert(throws(() => required(1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => required('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => required('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => required('a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => required('a', 'b', 'c', {}), TypeError, "No matching function overload found"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctionsArgv)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { argvOptional, argvRequired } from 'vararg.so';

const prefixOnly = argvOptional('a');
assert(() => prefixOnly[0] === 'a');
assert(() => prefixOnly[1] === 'a');
assert(() => prefixOnly[2] === 'a');

const prefixWithTail = argvOptional('a', 'b');
assert(() => prefixWithTail[0] === 'a b');
assert(() => prefixWithTail[1] === 'a');
assert(() => prefixWithTail[2] === 'b');

const prefixWithLongTail = argvOptional('a', 'b', 'c', 'd', 'e');
assert(() => prefixWithLongTail[0] === 'a b c d e');
assert(() => prefixWithLongTail[1] === 'a');
assert(() => prefixWithLongTail[2] === 'e');

assert(throws(() => argvOptional(), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional(1), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional({}), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional(1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional('a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional('a', 'b', 'c', {}), TypeError, "No matching function overload found"));
assert(throws(() => argvOptional(''), TypeError, "empty prefix"));
assert(throws(() => argvOptional('', 'a', 'b'), TypeError, "empty prefix"));

const single = argvRequired('a');
assert(() => single[0] === 'a');
assert(() => single[1] === 'a');
assert(() => single[2] === 'a');

const multiple = argvRequired('a', 'b');
assert(() => multiple[0] === 'a b');
assert(() => multiple[1] === 'a');
assert(() => multiple[2] === 'b');

const longTail = argvRequired('a', 'b', 'c', 'd', 'e');
assert(() => longTail[0] === 'a b c d e');
assert(() => longTail[1] === 'a');
assert(() => longTail[2] === 'e');

const emptyString = argvRequired('');
assert(() => emptyString[0] === '');
assert(() => emptyString[1] === '');
assert(() => emptyString[2] === '');

assert(throws(() => argvRequired(), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired(1), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired({}), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired(1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired('a', 1), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired('a', {}), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired('a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => argvRequired('a', 'b', 'c', {}), TypeError, "No matching function overload found"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctionsSelf)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { selfOptional, selfRequired } from 'vararg.so';

const self = {};
const prefixOnly = selfOptional.call(self, 'a');
assert(() => prefixOnly[0] === 'a');
assert(() => prefixOnly[1] === self);

const prefixWithTail = selfOptional.call(self, 'a', 'b');
assert(() => prefixWithTail[0] === 'a b');
assert(() => prefixWithTail[1] === self);

const prefixWithLongTail = selfOptional.call(self, 'a', 'b', 'c', 'd', 'e');
assert(() => prefixWithLongTail[0] === 'a b c d e');
assert(() => prefixWithLongTail[1] === self);

assert(throws(() => selfOptional.call(self), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 1), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, {}), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 'a', 1), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 'a', {}), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 'a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, 'a', 'b', 'c', {}), TypeError, "No matching function overload found"));
assert(throws(() => selfOptional.call(self, ''), TypeError, "empty prefix"));
assert(throws(() => selfOptional.call(self, '', 'a', 'b'), TypeError, "empty prefix"));

const single = selfRequired.call(self, 'a');
assert(() => single[0] === 'a');
assert(() => single[1] === self);

const multiple = selfRequired.call(self, 'a', 'b');
assert(() => multiple[0] === 'a b');
assert(() => multiple[1] === self);

const longTail = selfRequired.call(self, 'a', 'b', 'c', 'd', 'e');
assert(() => longTail[0] === 'a b c d e');
assert(() => longTail[1] === self);

const emptyString = selfRequired.call(self, '');
assert(() => emptyString[0] === '');
assert(() => emptyString[1] === self);

assert(throws(() => selfRequired.call(self), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 1), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, {}), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 1, 'a'), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 'a', 1), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 'a', {}), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 'a', 'b', 'c', 1), TypeError, "No matching function overload found"));
assert(throws(() => selfRequired.call(self, 'a', 'b', 'c', {}), TypeError, "No matching function overload found"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctionsFixedArity)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { fixed } from 'vararg.so';

assert(() => fixed('a') === 'a');
assert(() => fixed('a', 'b') === 'a');
assert(() => fixed('a', 'b', 'c') === 'a');
assert(throws(() => fixed(), TypeError, "No matching function overload found"));
assert(throws(() => fixed(1), TypeError, "No matching function overload found"));
assert(throws(() => fixed({}), TypeError, "No matching function overload found"));
assert(throws(() => fixed(''), TypeError, "empty prefix"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctionsArgvFixedArity)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { argvFixed } from 'vararg.so';

const single = argvFixed('a');
assert(() => single[0] === 'a');
assert(() => single[1] === 'a');
assert(() => single[2] === 'a');

const oneExtra = argvFixed('a', 'b');
assert(() => oneExtra[0] === 'a');
assert(() => oneExtra[1] === 'a');
assert(() => oneExtra[2] === 'a');

const twoExtra = argvFixed('a', 'b', 'c');
assert(() => twoExtra[0] === 'a');
assert(() => twoExtra[1] === 'a');
assert(() => twoExtra[2] === 'a');

assert(throws(() => argvFixed(), TypeError, "No matching function overload found"));
assert(throws(() => argvFixed(1), TypeError, "No matching function overload found"));
assert(throws(() => argvFixed({}), TypeError, "No matching function overload found"));
assert(throws(() => argvFixed(''), TypeError, "empty prefix"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FreeFunctionsSelfFixedArity)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { selfFixed } from 'vararg.so';

const self = {};
const single = selfFixed.call(self, 'a');
assert(() => single[0] === 'a');
assert(() => single[1] === self);

const oneExtra = selfFixed.call(self, 'a', 'b');
assert(() => oneExtra[0] === 'a');
assert(() => oneExtra[1] === self);

const twoExtra = selfFixed.call(self, 'a', 'b', 'c');
assert(() => twoExtra[0] === 'a');
assert(() => twoExtra[1] === self);

assert(throws(() => selfFixed.call(self), TypeError, "No matching function overload found"));
assert(throws(() => selfFixed.call(self, 1), TypeError, "No matching function overload found"));
assert(throws(() => selfFixed.call(self, {}), TypeError, "No matching function overload found"));
assert(throws(() => selfFixed.call(self, ''), TypeError, "empty prefix"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NonmovableArguments)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Vararg, number, sum } from 'vararg.so';

const v = new Vararg();
assert(() => number(42) === 42);
assert(() => sum(42) === 42);
assert(() => sum(39, 1, 2) === 42);
assert(() => v.number(42) === 42);
assert(() => v.sum(42) === 42);
assert(() => v.sum(39, 1, 2) === 42);
v.count();
assert(() => v.value === '0');
v.count(1, 2, 3);
assert(() => v.value === '3');
assert(throws(() => v.count('a'), TypeError, "No matching function overload found"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
