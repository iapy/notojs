#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Global, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Headers)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var headers = new Headers();
assert(() => !headers.has('X-My-Header-1'));

headers.set('X-My-Header-1', '20');
assert(() => headers.has('X-My-Header-1'));
assert(() => '20' == headers.get('X-My-Header-1'));
assert(() => '20' == headers.toJSON()['X-My-Header-1']);

headers.append('X-My-Header-1', '30');
assert(() => '20, 30' == headers.get('X-My-Header-1'));
assert(() => !headers.has('X-My-Header-2'));

headers.append('X-My-Header-2', '10');
assert(() => '10' == headers.get('X-My-Header-2'));

headers.append('X-My-Header-2', '20');
assert(() => '10, 20' == headers.get('X-My-Header-2'));

headers = new Headers([['Content-Length', '42']]);
assert(() => headers.has('Content-Length'));
assert(() => '42' == headers.get('Content-Length'));

headers = new Headers({'Host': '127.0.0.1'});
assert(() => headers.has('Host'));
assert(() => '127.0.0.1' == headers.get('Host'));

headers.delete('Host');
assert(() => !headers.has('Host'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HeadersIterator)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const headers = new Headers([['X-A', '42'], ['X-B', 'foo']]);

var keys = new Array();

for(let k of headers.keys()) keys.push(k);
assert(() => 'X-A;X-B' == keys.toSorted().join(';'));

var values = new Array();

for(let k of headers.values()) values.push(k);
assert(() => '42;foo' == values.toSorted().join(';'));

var ha = new Array();

headers.forEach((k, v, h) => {
    ha.push(`${k}=${v}`);
    assert(() => h == headers);
});
assert(() => 'X-A=42;X-B=foo' == ha.toSorted().join(';'));

var hb = new Array();

headers.forEach(hb.push, hb);
assert(() => '42;X-A;X-B;foo' == hb.filter((_, i) => i % 3 - 2).toSorted().join(';'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Request)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new Request('xxx-yyy'), 'Request constructor: invalid url [xxx-yyy]'));
assert(() => throws(() => new Request('ftp://example.org'), 'Request constructor: unsupported scheme [ftp]'));
assert(() => throws(() => new Request('http://google.com', {method: 'XXX'}), 'Request constructor: invalid method [XXX]'));

var a = new Request('http://google.com');
assert(() => 'GET' == a.method);
assert(() => 'http://google.com' == a.url);
assert(() => 'google.com' == a.headers.get('Host'));

var b = new Request('https://google.com', {method: 'POST'});
assert(() => 'POST' == b.method);
assert(() => 'https://google.com' == b.url);
assert(() => 'google.com' == b.headers.get('Host'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(RequestBody)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new Request('http://google.com', {body: 1}), 'Request: no matching constructor found'));

var a = new Request('https://google.com', {method: 'PATCH', body: 'test'});
const ab = await a.text();
assert(() => 'test' == ab);
assert(() => 'PATCH' == a.method);
assert(() => 'google.com' == a.headers.get('Host'));
assert(() => 'application/json' != a.headers.get('Content-Type'));

var b = new Request('https://google.com', {method: 'POST', body: [1,2,3,4]});
const bb = await b.text();
assert(() => 'POST' == b.method);
assert(() => '[1,2,3,4]' == bb);
assert(() => 'google.com' == b.headers.get('Host'));
assert(() => 'application/json' == b.headers.get('Content-Type'));

var c = new Request('https://google.com', {method: 'POST', body: {'foo':'bar'}});
const cb = await c.text();
assert(() => 'POST' == b.method);
assert(() => '{"foo":"bar"}' == cb);
assert(() => 'google.com' == c.headers.get('Host'));
assert(() => 'application/json' == c.headers.get('Content-Type'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(RequestHeaders)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new Request('http://google.com', {headers: 1}), 'Request: no matching constructor found'));

var a = new Request('https://google.com', {method: 'PATCH', body: {'a':'b'}, headers:{
    'Content-Type': 'text/plain'
}});
assert(() => 'PATCH' == a.method);
assert(() => 'text/plain' == a.headers.get('Content-Type'));
assert(() => 'google.com' == a.headers.get('Host'));

var b = new Request('https://google.com', {headers:[
    ['X-Foo', 'Bar']
]});
assert(() => 'GET' == b.method);
assert(() => 'Bar' == b.headers.get('X-Foo'));
assert(() => 'google.com' == b.headers.get('Host'));

var h = new Headers([['X-Bar', 'Foo']]);
var c = new Request('https://google.com/search', {headers:h});
assert(() => 'GET' == c.method);
assert(() => '/search' == c.path);
assert(() => 'Foo' == c.headers.get('X-Bar'));
assert(() => 'google.com' == c.headers.get('Host'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(RequestRedirect)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new Request('http://google.com', {redirect: 1}), 'Request: no matching constructor found'));
assert(() => throws(() => new Request('http://google.com', {redirect: 'XXX'}), 'Request constructor: invalid redirect method [XXX]'));

assert(() => 'follow' == new Request('http://google.com').redirect);
assert(() => 'error' == new Request('http://google.com', {redirect: 'error'}).redirect);
assert(() => 'follow' == new Request('http://google.com', {redirect: 'follow'}).redirect);
assert(() => 'manual' == new Request('http://google.com', {redirect: 'manual'}).redirect);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Blob)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new Blob(), 'Blob: no matching constructor found'));
assert(() => throws(() => new Blob('1'), 'Blob: no matching constructor found'));
assert(() => throws(() => new Blob([1]), 'Blob constructor: invalid type [0]'));

const b1 = new Blob([]);
assert(() => 0 == b1.size);

const a1 = await b1.arrayBuffer();
assert(() => 0 == a1.byteLength);

const u1 = new Int32Array(a1);
assert(() => 0 == u1.length);

const v1 = await b1.bytes();
assert(() => 0 == v1.length);

const b2 = new Blob(['foo bar baz'], {type: 'text/plain'});
assert(() => 11 == b2.size);
assert(() => 'text/plain' == b2.type);

const t2 = await b2.text();
assert(() => 'foo bar baz' == t2);

const a2 = await b2.arrayBuffer();
assert(() => 11 == a2.byteLength);

const u2 = new Uint8Array(a2);
assert(() => 11 == u2.length);

for(var i = 0; i < 11; ++i)
    assert(() => t2.charCodeAt(i) == u2[i]);

const v2 = await b2.bytes();
assert(() => 11 == v2.length);

for(var i = 0; i < 11; ++i)
    assert(() => t2.charCodeAt(i) == v2[i]);

u2[0] = 'b'.charCodeAt(0);
v2[1] = 'a'.charCodeAt(0);

var bs = b2.slice();
var ts = await bs.text();
assert(() => 'bao bar baz' == ts);

bs = b2.slice(8);
ts = await bs.text();
assert(() => 'baz' == ts);

bs = b2.slice(-3);
ts = await bs.text();
assert(() => 'baz' == ts);

bs = b2.slice(0, -4);
ts = await bs.text();
assert(() => 'bao bar' == ts);

bs = b2.slice(0, -4, 'x/y');
ts = await bs.text();
assert(() => 'bao bar' == ts);
assert(() => 'x/y' == bs.type);

import { make } from 'global.so';

const b3 = make(10);
assert(() => 10 == b3.size);

const a3 = await b3.arrayBuffer();
assert(() => 10 == a3.byteLength);

const u3 = new Int8Array(a3);
assert(() => 10 == u3.length);

for(var i = 0; i < 10; ++i)
    assert(() => i == u3[i]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Base64)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => 'aGVsbG8gd29ybGQ=' == btoa('hello world'));
assert(() => 'hello world' == atob('aGVsbG8gd29ybGQ='));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(URL)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new URL('abracadabra'), 'URL constructor: invalid url [abracadabra]'));

const u = new URL('https://developer.mozilla.org:443/en-US/docs/Web/API/URL/host?q=host');
assert(() => 'developer.mozilla.org:443' == u.host);
assert(() => 'developer.mozilla.org' == u.hostname);
assert(() => '443' == u.port);
assert(() => 'https:' == u.protocol);
assert(() => '/en-US/docs/Web/API/URL/host' == u.pathname);
assert(() => 'https://developer.mozilla.org:443/en-US/docs/Web/API/URL/host?q=host' == u.href);
assert(() => '?q=host' == u.search);
assert(() => 1 == u.searchParams.size);
assert(() => u.searchParams.has('q'));
assert(() => 'host' == u.searchParams.get('q'));

const v = new URL('http://google.com');
assert(() => 'google.com' == v.host);
assert(() => 'google.com' == v.hostname);
assert(() => '' == v.port);
assert(() => 'http:' == v.protocol);
assert(() => '/' == v.pathname);
assert(() => 'http://google.com' == v.href);
assert(() => '' == v.search);
assert(() => !v.searchParams.has('q'));

assert(() => throws(() => new Request(new URL('--google.com')), 'URL constructor: invalid url [--google.com]'));
assert(() => throws(() => new Request(new URL('ftp://google.com')), 'Request constructor: unsupported scheme [ftp]'));
assert(() => throws(() => fetch(new URL('ftp://google.com')), 'unsupported scheme [ftp]'));
assert(() => throws(() => fetch(new URL('ftp://google.com'), {}), 'unsupported scheme [ftp]'));

assert(() => null == URL.parse('--google.com'));
assert(() => !URL.canParse('--google.com'));
assert(() => URL.canParse('https://developer.mozilla.org:443/en-US/docs/Web/API/URL/host?q=host'));

const w = URL.parse('https://developer.mozilla.org:443/en-US/docs/Web/API/URL/host?q=host');
assert(() => 'developer.mozilla.org:443' == w.host);
assert(() => 'developer.mozilla.org' == w.hostname);
assert(() => '443' == w.port);
assert(() => 'https:' == w.protocol);
assert(() => '/en-US/docs/Web/API/URL/host' == w.pathname);
assert(() => 'https://developer.mozilla.org:443/en-US/docs/Web/API/URL/host?q=host' == w.href);
assert(() => '?q=host' == w.search);
assert(() => 1 == w.searchParams.size);
assert(() => w.searchParams.has('q'));
assert(() => 'host' == w.searchParams.get('q'));

w.host = 'test.com';
assert(() => 'test.com' == w.host);
assert(() => 'test.com' == w.hostname);

w.host = 'test.com:8443';
assert(() => 'test.com:8443' == w.host);
assert(() => 'test.com' == w.hostname);

w.port = '5555';
assert(() => 'test.com:5555' == w.host);
assert(() => 'test.com' == w.hostname);

w.pathname = 'foobar';
assert(() => '/foobar' == w.pathname);
assert(() => '?q=host' == w.search);

w.pathname = '/foobar';
assert(() => '/foobar' == w.pathname);
assert(() => '?q=host' == w.search);

w.search = 'v=foo';
assert(() => '/foobar' == w.pathname);
assert(() => '?v=foo' == w.search);

w.search = '?v=foo&w=bar';
assert(() => '/foobar' == w.pathname);
assert(() => '?v=foo&w=bar' == w.search);

w.href = 'https://test.com/path?q=1';
assert(() => 'test.com' == w.host);
assert(() => 'test.com' == w.hostname);
assert(() => '/path' == w.pathname);
assert(() => '?q=1' == w.search);

assert(() => throws(() => w.protocol = 'aasds'));

w.protocol = 'http:';
assert(() => 'http://test.com/path?q=1' == w.href);

w.username = 'guest';
assert(() => 'http://guest@test.com/path?q=1' == w.href);

w.password = 'guest';
assert(() => 'http://guest:guest@test.com/path?q=1' == w.href);

w.username = '';
assert(() => 'http://test.com/path?q=1' == w.href);

print(w);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0].GetString(), "http://test.com/path?q=1"));
}

BOOST_AUTO_TEST_SUITE_END()
