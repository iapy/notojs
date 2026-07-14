#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(HTTPData, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Cache)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { icon } from 'noto:core';
import { open } from 'noto:db';

const c = open('sys:httpdata');
c.drop();

await icon('icon-park-outline/iphone');
let a = c.data;

assert(() => 1 == a.length);
assert(() => 'https://api.iconify.design' == a[0].base);
assert(() => 1 == Object.keys(a[0].path).length);
assert(() => 'internal' == a[0].type);

await icon('icon-park-outline/iphone');
a = c.data;

assert(() => 1 == a.length);
assert(() => 'https://api.iconify.design' == a[0].base);
assert(() => 1 == Object.keys(a[0].path).length);
assert(() => 'internal' == a[0].type);

await icon('icon-park-outline/ipad');
a = c.data;

assert(() => 1 == a.length);
assert(() => 'https://api.iconify.design' == a[0].base);
assert(() => 2 == Object.keys(a[0].path).length);
assert(() => 'internal' == a[0].type);

assert(() => throws(() => c.drop('sys', 'https://api.iconify.design')));
assert(() => throws(() => c.drop('system', 'https://api.iconify.design/')));

assert(() => 0 == c.drop('system', 'https://api.iconify.design'));
assert(() => 2 == c.drop('internal', 'https://api.iconify.design'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()

