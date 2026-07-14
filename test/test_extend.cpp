#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Extend, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Simple)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { A, B, C } from 'extend.so';

const a = new A();
const b = new B();
const c = new C();

assert(() => b instanceof A);
assert(() => c instanceof A);
assert(() => c instanceof B);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Getter)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { A, B, C } from 'extend.so';

const a = new A();
const b = new B();
const c = new C();

assert(() => 'init' == a.a);
assert(() => 'init' == b.a);
assert(() => 'init' == c.a);

assert(() => 'undefined' == typeof a.b);
assert(() => 42 == b.b);
assert(() => 42 == c.b);

assert(() => 'undefined' == typeof a.c);
assert(() => 'undefined' == typeof b.c);
assert(() => 0 == c.c);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Setter)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { A, B, C } from 'extend.so';

const a = new A();
const b = new B();
const c = new C();

a.a = b.a = c.a = 'test';
assert(() => 'test' == a.a);
assert(() => 'test' == b.a);
assert(() => 'test' == c.a);

b.b = c.b = 24;
assert(() => 24 == b.b);
assert(() => 24 == c.b);

c.c = 1;
assert(() => 1 == c.c);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Method)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { A, B, C } from 'extend.so';

const a = new A();
const b = new B();
const c = new C();

a.set('a', 5);
assert(() => 'aaaaa' == a.a);

a.add('b');
assert(() => 'aaaaab' == a.a);

b.set('a', 6);
assert(() => 'aaaaaa' == b.a);

b.add('b');
assert(() => 'aaaaaabb' == b.a);

c.set('a', 7);
assert(() => 'aaaaaaa' == c.a);

c.add('b');
assert(() => 'aaaaaaabbb' == c.a);

c.adopt(a);
assert(() => 'aaaaab' == c.a);

c.adopt(b);
assert(() => 'aaaaaabb' == c.a);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
