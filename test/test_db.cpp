#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(DB, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(CRUD)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { open } from 'noto:db';

assert(() => throws(() => open(), 'No matching function overload found'));
assert(() => throws(() => open(1), 'No matching function overload found'));
assert(() => throws(() => open(''), 'Invalid database name: []'));
assert(() => throws(() => open(':'), 'Invalid database name: [:]'));
assert(() => throws(() => open('a::b'), 'Invalid database name: [a::b]'));
assert(() => throws(() => open('a:b:'), 'Invalid database name: [a:b:]'));
assert(() => throws(() => open('sys:a', 'b'), 'Invalid database name: [sys:a]'));

const db = open('test:a', 'test:b');
assert(() => 2 == db.length);

db.rw((a, b) => {
    a.set('a', 'b');
    b.set('b', {x:1, y:'test'});
});

db.ro((a, b) => {
    assert(() => a.has('a'));
    assert(() => 'b' === a.get('a'));
    assert(() => !a.has('b'));
    assert(() => null === a.get('b'));
    assert(() => 1 == a.stat.entries);

    assert(() => !b.has('a'));
    assert(() => null === b.get('a'));
    assert(() => b.has('b'));
    assert(() => 1 === b.get('b').x);
    assert(() => 'test' === b.get('b').y);
    assert(() => 1 == b.stat.entries);
});

db.rw((a, b) => {
    a.del('a');
    b.del('b');
});

let tx;
db.ro((a, b) => {
    tx = a;
    assert(() => 0 == a.stat.entries);
    assert(() => 0 == b.stat.entries);
});

assert(() => throws(() => tx.get('a'), 'Transaction is finished'));
assert(() => throws(() => db.ro((a) => a.set('a', 'b')), 'mdb_put'));

assert(() => throws(() => db.rw((a, b) => {
    a.set('a', 'b');
    b.set('b', 1);
    throw '';
})));

db.ro((a, b) => {
    assert(() => 0 == a.stat.entries);
    assert(() => 0 == b.stat.entries);
});

db.rw((a, b) => {
    a.set(1, 'foo');
    b.set(-1, [1,2,3]); 
});

db.ro((a, b) => {
    assert(() => a.has(1));
    assert(() => 'foo' == a.get(1));

    assert(() => b.has(-1));
    assert(() => 1 == b.get(-1)[0]);
    assert(() => 2 == b.get(-1)[1]);
    assert(() => 3 == b.get(-1)[2]);
});

db.drop();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Iterate)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { open } from 'noto:db';

function is_same(a1, a2) {
    if(a1.length != a2.length) return false;
    for(let i = 0; i < a1.length; ++i) {
        if(a1[i] instanceof Array && a2[i] instanceof Array) {
            if(!is_same(a1[i], a2[i])) return false;
        }
        else if(a1[i] != a2[i]) return false;
    }
    return true;
};

const db = open('test:a');
db.rw((a) => {
    a.set('a', 'a');
    a.set('b', 'b');
    a.set('c', 'c');
    a.set('d', 'd');
    a.set('e', 'e');
});

let arr = new Array();
db.rw((a) => a.each((k, v) => {
    arr.push([k, v]);
    a.set(k, [v]);
}));

assert(() => 5 == arr.length);
assert(() => is_same(['a', 'a'], arr[0]));
assert(() => is_same(['b', 'b'], arr[1]));
assert(() => is_same(['c', 'c'], arr[2]));
assert(() => is_same(['d', 'd'], arr[3]));
assert(() => is_same(['e', 'e'], arr[4]));

arr = db.ro((a) => a.map((k, v) => {
    return k;
}));
assert(() => is_same(['a', 'b', 'c', 'd', 'e'], arr));

arr = new Array();
db.ro((a) => a.each((k, v) => {
    arr.push([k, v]);
}));
assert(() => 5 == arr.length);
assert(() => is_same(['a', ['a']], arr[0]));
assert(() => is_same(['b', ['b']], arr[1]));
assert(() => is_same(['c', ['c']], arr[2]));
assert(() => is_same(['d', ['d']], arr[3]));
assert(() => is_same(['e', ['e']], arr[4]));

arr = new Array();
db.ro((a) => a.each((k, v) => {
    if(k == 'd') return false;
    arr.push([k, v]);
}));
assert(() => 3 == arr.length);
assert(() => is_same(['a', ['a']], arr[0]));
assert(() => is_same(['b', ['b']], arr[1]));
assert(() => is_same(['c', ['c']], arr[2]));

assert(() => throws(() => db.ro(a => {
    a.each(() => {
        throw new Error('Dummy');
    });
}), 'Dummy'));

db.drop();

db.rw((a) => {
    a.set(-2, 'a');
    a.set(-1, 'b');
    a.set(0, 'c');
    a.set(1, 'd');
    a.set(2, 'e');
});

arr = new Array();
db.rw((a) => a.each((k, v) => {
    arr.push(k);
}));

assert(() => is_same([-2,-1,0,1,2], arr));

arr = db.ro(a => a.map(k => k));
assert(() => is_same([-2,-1,0,1,2], arr));

db.drop();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Ranges)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { open } from 'noto:db';

function is_same(a1, a2) {
    if(a1.length != a2.length) return false;
    for(let i = 0; i < a1.length; ++i) {
        if(a1[i] instanceof Array && a2[i] instanceof Array) {
            if(!is_same(a1[i], a2[i])) return false;
        }
        else if(a1[i] != a2[i]) return false;
    }
    return true;
};

const db = open('test:a');
db.rw((a) => {
    a.set('a', 'a');
    a.set('b', 'b');
    a.set('c', 'c');
    a.set('d', 'd');
    a.set('e', 'e');
});

function range(r) {
    db.ro((a) => a.each(r, () => {}));
}

function mrange(r) {
    db.ro((a) => a.map(r, () => {}));
}

assert(() => throws(() => range([]), 'Expecting array of size 2'));
assert(() => throws(() => range(['a']), 'Expecting array of size 2'));
assert(() => throws(() => range(['a', {}]), 'Expecting number, string or null'));
assert(() => throws(() => range([{}, 'a']), 'Expecting number, string or null'));

assert(() => throws(() => mrange([]), 'Expecting array of size 2'));
assert(() => throws(() => mrange(['a']), 'Expecting array of size 2'));
assert(() => throws(() => mrange(['a', {}]), 'Expecting number, string or null'));
assert(() => throws(() => mrange([{}, 'a']), 'Expecting number, string or null'));

let arr = new Array();
db.ro((a) => a.each(['b', 'd'], (k, v) => {
    arr.push([k, v]);
}));
assert(() => is_same([['b', 'b'], ['c', 'c'], ['d', 'd']], arr));

arr = new Array();
db.ro((a) => a.each(['b', 'd'], (k, v) => {
    if(k == 'd') return false;
    arr.push([k, v]);
}));
assert(() => is_same([['b', 'b'], ['c', 'c']], arr));

arr = new Array();
db.ro((a) => a.each(['d', null], (k, v) => {
    arr.push([k, v]);
}));
assert(() => is_same([['d', 'd'], ['e', 'e']], arr));

arr = new Array();
db.ro((a) => a.each([null, 'b'], (k, v) => {
    arr.push([k, v]);
}));
assert(() => is_same([['a', 'a'], ['b', 'b']], arr));

arr = new Array();
db.ro((a) => a.each([null, null], (k, v) => {
    arr.push([k, v]);
}));
assert(() => is_same([['a', 'a'], ['b', 'b'], ['c', 'c'], ['d', 'd'], ['e', 'e']], arr));

arr = db.ro((a) => a.map(['d', null], (k, v) => {
    return k;
}));
assert(() => is_same(['d', 'e'], arr));

arr = db.ro((a) => a.map(['b', 'd'], (k, v) => {
    return k;
}));
assert(() => is_same(['b', 'c', 'd'], arr));

arr = db.ro((a) => a.map([null, 'b'], (k, v) => {
    return k;
}));
assert(() => is_same(['a', 'b'], arr));

arr = db.ro((a) => a.map([null, null], (k, v) => {
    return k;
}));
assert(() => is_same(['a', 'b', 'c', 'd', 'e'], arr));

assert(() => throws(() => db.ro(a => {
    a.each(['a', null], () => {
        throw new Error('Dummy');
    });
}), 'Dummy'));

assert(() => throws(() => db.ro(a => {
    a.map(['a', null], (k) => {
        if(k == 'b') throw new Error('Dummy');
        return k;
    });
}), 'Dummy'));

db.drop();

db.rw((a) => {
    a.set(-2, 'a');
    a.set(-1, 'b');
    a.set(0, 'c');
    a.set(1, 'd');
    a.set(2, 'e');
});

arr = db.ro(a => a.map([0,1], k => k));
assert(() => is_same([0,1], arr));

arr = db.ro(a => a.map([-1, 1], (_, v) => v));
assert(() => is_same(['b', 'c', 'd'], arr));

arr = db.ro(a => a.map([0,null], k => k));
assert(() => is_same([0,1,2], arr));

arr = db.ro(a => a.map([null, 1], (_, v) => v));
assert(() => is_same(['a', 'b', 'c', 'd'], arr));

db.drop();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Reduce)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { open } from 'noto:db';

function is_same(a1, a2) {
    if(a1.length != a2.length) return false;
    for(let i = 0; i < a1.length; ++i) {
        if(a1[i] instanceof Array && a2[i] instanceof Array) {
            if(!is_same(a1[i], a2[i])) return false;
        }
        else if(a1[i] != a2[i]) return false;
    }
    return true;
};

const db = open('test:a');
db.rw(d => {
    d.set(-3, 'a');
    d.set(-2, 'b');
    d.set(-1, 'c');
    d.set( 0, 'd');
    d.set( 1, 'e');
    d.set( 2, 'f');
    d.set( 3, 'g');
});

let arr;
arr = db.ro(d => d.reduce((acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same([-3,-2,-1,0,1,2,3], arr));

arr = db.ro(d => d.reduce((acc, _, v) => {
    acc.push(v);
    return acc;
}, []));
assert(() => is_same(['a','b','c','d','e','f','g'], arr));

arr = db.ro(d => d.reduce([0, null], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same([0,1,2,3], arr));

arr = db.ro(d => d.reduce([null, 0], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same([-3,-2,-1,0], arr));

arr = db.ro(d => d.reduce([-1,1], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same([-1,0,1], arr));

db.drop();

db.rw((a) => {
    a.set('a', 'a');
    a.set('b', 'b');
    a.set('c', 'c');
    a.set('d', 'd');
    a.set('e', 'e');
});

arr = db.ro(d => d.reduce((acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same(['a','b','c','d','e'], arr));

arr = db.ro(d => d.reduce(['b','d'], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same(['b','c','d'], arr));

arr = db.ro(d => d.reduce(['b',null], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same(['b','c','d','e'], arr));

arr = db.ro(d => d.reduce([null,'c'], (acc, k) => {
    acc.push(k);
    return acc;
}, []));
assert(() => is_same(['a','b','c'], arr));

arr = db.ro(d => d.reduce((acc, k) => null, 0));
assert(() => null == arr);

db.drop();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()

