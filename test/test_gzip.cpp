#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(GZip, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(GZip)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';
import { gzip } from 'noto:gzip';

assert(() => throws(() => gzip(fs.path('/ro/file.z')).write(''), 'Read-only file system'));

let bytes = await gzip(fs.path('/rw/file.z')).write('Hello, world!');
assert(() => bytes == 13);

let blob = await gzip(fs.path('/ro/file.z')).blob();
let text = await blob.text();
assert(() => 'Hello, world!' == text);

text = await gzip(fs.path('/ro/file.z')).text();
assert(() => 'Hello, world!' == text);

assert(() => throws(() => gzip(fs.path('/ro/file.z')).json()));
await gzip(fs.path('/rw/file.z')).write('{"value": 42}');

let json = await gzip(fs.path('/rw/file.z')).json();
assert(() => 42 == json.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
