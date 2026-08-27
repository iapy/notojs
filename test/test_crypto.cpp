#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Crypto, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(SHA1)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('crypto');

const r = crypto.getRandomValues(10);
const p = crypto.subtle.digest('SHA-1', r);

assert(() => p instanceof Promise);

const b = await p;
assert(() => b instanceof ArrayBuffer);
assert(() => 20 === b.byteLength);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SHA256)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('crypto');

const r = crypto.getRandomValues(10);
const p = crypto.subtle.digest('SHA-256', r);

assert(() => p instanceof Promise);

const b = await p;
assert(() => b instanceof ArrayBuffer);
assert(() => 32 === b.byteLength);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SHA384)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('crypto');

const r = crypto.getRandomValues(10);
const p = crypto.subtle.digest('SHA-384', r);

assert(() => p instanceof Promise);

const b = await p;
assert(() => b instanceof ArrayBuffer);
assert(() => 48 === b.byteLength);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SHA512)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
require('crypto');

const r = crypto.getRandomValues(10);
const p = crypto.subtle.digest('SHA-512', r);

assert(() => p instanceof Promise);

const b = await p;
assert(() => b instanceof ArrayBuffer);
assert(() => 64 === b.byteLength);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
