#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Fs, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Mount)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';

const m = fs.mounts();
assert(() => 2 == m.length);
assert(() => 'rw' == m[0].flag);
assert(() => '/rw' == m[0].path);
assert(() => 'ro' == m[1].flag);
assert(() => '/ro' == m[1].path);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Errors)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';

assert(() => throws(() => fs.path('ab'), 'Expecting absolute path'));
assert(() => throws(() => fs.path('/ab'), 'Not mounted'));
assert(() => throws(() => fs.path('/ro').remove(), 'Read-only file system'));
assert(() => throws(() => fs.path('/rw').remove(), 'Read-only file system'));
assert(() => throws(() => fs.path('/ro/test').mkdir(), 'Read-only file system'));
assert(() => throws(() => fs.path('/rw/test/foo').mkdir(), 'No such file or directory'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Directory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';

const a = fs.path('/rw/a')
const b = fs.path('/ro/a')
assert(() => !a.exists);
assert(() => !b.exists);

a.mkdir();
assert(() => a.exists);
assert(() => b.exists);

assert(() => a.is_directory);
assert(() => b.is_directory);

assert(() => !a.is_file);
assert(() => !b.is_file);

await a.append('test.txt').write('42');
a.copy(fs.path('/rw/d'));

assert(() => fs.path('/rw/d/test.txt').exists);

const j = await fs.path('/ro/d/test.txt').json();
assert(() => 42 == j);

assert(() => throws(() => b.remove({recursive: true}), 'Read-only file system'));
assert(() => throws(() => b.remove(), 'Read-only file system'));
assert(() => throws(() => a.remove(), 'Directory not empty'));

const c = fs.path('/rw/b');
const d = fs.path('/ro/b');

assert(() => throws(() => a.move(d), 'Read-only file system'));
assert(() => throws(() => b.move(c), 'Read-only file system'));

a.move(c);
assert(() => !a.exists);
assert(() => c.exists);

c.remove({recursive: true});
assert(() => !c.exists);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Ops)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';

const x = fs.path('/rw/x.txt');
const y = fs.path('/ro/y.txt');
const z = fs.path('/rw/y.txt');

assert(() => !x.exists);
assert(() => !y.exists);
assert(() => !z.exists);

await x.write('Foo');
assert(() => throws(() => x.copy(y), 'Read-only file system'));

x.copy(z);
assert(() => z.exists);
assert(() => y.exists);

await x.write('Bar');
assert(() => throws(() => x.copy(z), 'File exists'));

x.copy(z, {overwrite: true});

const t = await z.text();
assert(() => 'Bar' == t);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Writer)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';

const a = fs.path('/rw/a.txt')
const b = fs.path('/ro/a.txt')
assert(() => !a.exists);
assert(() => !b.exists);

assert(() => throws(() => b.write('test'), 'Read-only file system'));

const n = await a.write('test');
assert(() => 4 == a.size);
assert(() => 4 == n);

let t = await b.text();
assert(() => 'test' == t);

let u = await b.blob();
assert(() => u instanceof Blob);
assert(() => 4 == u.size);
assert(() => '' == u.type);

let v = await b.blob('text/plain');
assert(() => v instanceof Blob);
assert(() => 4 == v.size);
assert(() => 'text/plain' == v.type);

const c = fs.path('/rw/x/a.txt');
assert(() => throws(() => c.write('test'), 'No such file or directory'));

const m = await a.write('foobar');
assert(() => 6 == a.size);
assert(() => 6 == m);

const p = await a.write('baz', {append: true});
assert(() => 9 == a.size);
assert(() => 3 == p);

t = await b.text();
assert(() => 'foobarbaz' == t);

assert(() => throws(() => b.json()));
await a.write('{"value":42}');

t = await b.json();
assert(() => 42 == t.value);

a.remove();
assert(() => !a.exists);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(API)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { make } from 'fs.so';

const a = make('/rw/a.txt');
assert(() => throws(() => make('foo'), 'Expecting absolute path'));
assert(() => throws(() => make('/xx'), 'Not mounted'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
