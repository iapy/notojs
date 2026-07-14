#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(JSDict, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(IDict)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { IDict } from 'jsdict.so';

assert(() => throws(() => new IDict(['a']), "IDict: no matching constructor found"));
assert(() => throws(() => new IDict([['a']]), "IDict: no matching constructor found"));
assert(() => throws(() => new IDict({'a': 'b'}), "IDict: no matching constructor found"));
assert(() => throws(() => new IDict([['a', 'b']]), "IDict: no matching constructor found"));

const empty = new IDict();
assert(() => '' == empty.string);

const fromo = new IDict({'a': 1, 'b': 2});
assert(() => 'a:1;b:2' == fromo.string);

const froma = new IDict([['a', 1], ['b', 2], ['c', 3]]);
assert(() => 'a:1;b:2;c:3' == froma.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(IDictIterate)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { IDict } from 'jsdict.so';

const froma = new IDict([['a', 1], ['b', 2], ['c', 3]]);

var keys = new Array();
for(let k of froma.keys()) keys.push(k);
assert(() => 'abc' == keys.toSorted().join(''));

var values = new Array();
for(let k of froma.values()) values.push(k);
assert(() => 1 == values.toSorted()[0]);
assert(() => 2 == values.toSorted()[1]);
assert(() => 3 == values.toSorted()[2]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SDict)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { SDict } from 'jsdict.so';

assert(() => throws(() => new SDict(['a']), "SDict: no matching constructor found"));
assert(() => throws(() => new SDict([['a']]), "SDict: no matching constructor found"));
assert(() => throws(() => new SDict({'a': 1}), "SDict: no matching constructor found"));
assert(() => throws(() => new SDict([['a', 1]]), "SDict: no matching constructor found"));

const empty = new SDict();
assert(() => '' == empty.string);

const fromo = new SDict({'a': 'z', 'b': 'y'});
assert(() => 'a:z;b:y' == fromo.string);

const froma = new SDict([['a', 'z'], ['b', 'y'], ['c', 'x']]);
assert(() => 'a:z;b:y;c:x' == froma.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SDictIterate)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { SDict } from 'jsdict.so';

const froma = new SDict([['a', 'z'], ['b', 'y'], ['c', 'x']]);

var keys = new Array();
for(let k of froma.keys()) keys.push(k);
assert(() => 'abc' == keys.toSorted().join(''));

var values = new Array();
for(let k of froma.values()) values.push(k);
assert(() => 'x' == values.toSorted()[0]);
assert(() => 'y' == values.toSorted()[1]);
assert(() => 'z' == values.toSorted()[2]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
