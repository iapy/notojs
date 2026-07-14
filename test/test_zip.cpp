#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Zip, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Zip)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as fs from 'noto:fs';
import { zip } from 'noto:zip';

assert(() => throws(() => zip(fs.path('/ro/archive.zip')).write({}), 'Read-only file system'));
assert(() => throws(() => zip(fs.path('/ro/archive.zip')).read(), 'No such file or directory'));
assert(() => throws(() => zip(fs.path('/ro/archive.zip')).remove(''), 'No such file or directory'));

await fs.path('/rw/foo.txt').write('This is foo.txt');
await fs.path('/rw/bar.txt').write('This is bar.txt');

await zip(fs.path('/rw/archive.zip')).write({
    'foo.txt': fs.path('/ro/foo.txt'),
    'bar.txt': (await fs.path('/ro/bar.txt').blob()),
    'baz.txt': 'This is baz.txt'
});

assert(() => fs.path('/rw/archive.zip').exists);

const a = await zip(fs.path('/rw/archive.zip')).read('foo.txt');
assert(() => 1 == Object.keys(a).length);

let t = await a['foo.txt'].text();
assert(() => 'This is foo.txt' == t);

const b = await zip(fs.path('/rw/archive.zip')).read();
assert(() => 3 == Object.keys(b).length);

t = await b['bar.txt'].text();
assert(() => 'This is bar.txt' == t);

t = await b['baz.txt'].text();
assert(() => 'This is baz.txt' == t);

const c = await zip(fs.path('/rw/archive.zip')).read('');
assert(() => 0 == Object.keys(c).length);

await zip(fs.path('/rw/archive.zip')).write({
    'foo.txt': fs.path('/ro/foo.txt')
});

const d = await zip(fs.path('/rw/archive.zip')).read();
assert(() => 1 == Object.keys(d).length);

await zip(fs.path('/rw/archive.zip')).write({
    'foo.txt': fs.path('/ro/foo.txt')
}, {append: true});

const e = await zip(fs.path('/rw/archive.zip')).read();
assert(() => 1 == Object.keys(e).length);

await zip(fs.path('/rw/archive.zip')).write({
    'bar.txt': fs.path('/ro/bar.txt')
}, {append: true});

const f = await zip(fs.path('/rw/archive.zip')).read();
assert(() => 2 == Object.keys(f).length);

for(const [name, size] of zip(fs.path('/rw/archive.zip'))) assert(() => (name in f));

assert(() => throws(() => zip(fs.path('/ro/archive.zip')).remove('foo.txt'), 'Read-only file system'));
assert(() => throws(() => zip(fs.path('/ro/archive.zip')).remove(), 'No matching function overload found'));
await zip(fs.path('/rw/archive.zip')).remove('foo.txt');

const g = await zip(fs.path('/rw/archive.zip')).read();
assert(() => 1 == Object.keys(g).length);

t = await g['bar.txt'].text();
assert(() => 'This is bar.txt' == t);

print(zip(fs.path('/ro/asdf.zip')));
    )JS");

    BOOST_TEST(get_error() == "Bad file descriptor");
}

BOOST_AUTO_TEST_SUITE_END()
