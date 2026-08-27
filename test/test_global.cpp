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

const empty = new Blob();
assert(() => 0 == empty.size);
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

const bparts = new Blob([b2, '!'], {type: 'text/custom'});
assert(() => 12 == bparts.size);
assert(() => 'text/custom' == bparts.type);
const tparts = await bparts.text();
assert(() => 'foo bar baz!' == tparts);

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

BOOST_AUTO_TEST_CASE(File)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => new File(), 'File: no matching constructor found'));

const start = Date.now();
const generated = new File(['generated'], 'generated.txt');
assert(() => generated instanceof File);
assert(() => generated instanceof Blob);
assert(() => 'generated.txt' == generated.name);
assert(() => '' == generated.type);
assert(() => 9 == generated.size);
assert(() => '' == generated.webkitRelativePath);
assert(() => generated.lastModified >= start);
const generatedText = await generated.text();
assert(() => 'generated' == generatedText);

const file = new File(
    [new Blob(['hello']), ' world'],
    'folder/report.txt',
    {type: 'text/plain', lastModified: 42}
);
assert(() => file instanceof Blob);
assert(() => 'folder/report.txt' == file.name);
assert(() => 'text/plain' == file.type);
assert(() => 11 == file.size);
assert(() => 42 == file.lastModified);
const fileText = await file.text();
assert(() => 'hello world' == fileText);

const copied = new Blob([file]);
const copiedText = await copied.text();
assert(() => 'hello world' == copiedText);

const slice = file.slice(6);
assert(() => slice instanceof Blob);
assert(() => !(slice instanceof File));
const sliceText = await slice.text();
assert(() => 'world' == sliceText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FormData)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const data = new FormData();
assert(() => throws(() => new FormData([]), 'FormData: no matching constructor found'));

const blob = new Blob(['payload'], {type: 'text/plain'});
const file = new File(['document'], 'document.txt', {type: 'text/plain', lastModified: 42});
data.append('name', 'first');
data.append('name', 'second');
data.append('upload', blob, 'report.txt');
data.append('document', file);

assert(() => data.has('name'));
assert(() => data.get('name') == 'first');
assert(() => 2 == data.getAll('name').length);
assert(() => data.get('upload') instanceof File);
assert(() => data.get('upload') instanceof Blob);
assert(() => 'report.txt' == data.get('upload').name);
const uploadText = await data.get('upload').text();
assert(() => 'payload' == uploadText);
assert(() => 'text/plain' == data.get('upload').type);
assert(() => data.get('document') instanceof File);
assert(() => 'document.txt' == data.get('document').name);
assert(() => 42 == data.get('document').lastModified);
const renamed = new FormData();
renamed.append('document', file, 'renamed.txt');
assert(() => 'renamed.txt' == renamed.get('document').name);
assert(() => 42 == renamed.get('document').lastModified);
assert(() => 'name,name,upload,document' == Array.from(data.keys()).join(','));
const values = await Promise.all(
    Array.from(data.values()).map(value => value instanceof Blob ? value.text() : value)
);
assert(() => 'first,second,payload,document' == values.join(','));
const entries = await Promise.all(
    Array.from(data).map(async ([name, value]) => [name, value instanceof Blob ? await value.text() : value])
);
assert(() => 'name,first;name,second;upload,payload;document,document' == entries.map(value => value.join(',')).join(';'));

data.set('name', 'replacement');
assert(() => 1 == data.getAll('name').length);
assert(() => 'replacement' == data.get('name'));

data.set('upload', blob);
const replacedUpload = await data.get('upload').text();
assert(() => 'payload' == replacedUpload);
assert(() => data.get('upload') instanceof File);
assert(() => 'blob' == data.get('upload').name);

data.delete('name');
assert(() => !data.has('name'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(TextEncoding)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const encoder = new TextEncoder();
assert(() => 'utf-8' === encoder.encoding);
assert(() => 0 === encoder.encode().length);
assert(() => 0 === encoder.encode(undefined).length);
assert(() => '65,240,159,152,128' === Array.from(encoder.encode('A😀')).join(','));
assert(() => '239,191,189' === Array.from(encoder.encode('\uD800')).join(','));

const destination = new Uint8Array(5);
const encoded = encoder.encodeInto('A😀B', destination);
assert(() => 3 === encoded.read);
assert(() => 5 === encoded.written);
assert(() => '65,240,159,152,128' === Array.from(destination).join(','));
assert(() => throws(() => encoder.encodeInto('x', new Int8Array(1))));

const decoder = new TextDecoder();
assert(() => 'utf-8' === decoder.encoding);
assert(() => !decoder.fatal);
assert(() => !decoder.ignoreBOM);
assert(() => '' === decoder.decode());
assert(() => 'A😀' === decoder.decode(encoder.encode('A😀')));
assert(() => 'payload' === decoder.decode(encoder.encode('payload').buffer));

const viewBuffer = encoder.encode('xpayloady').buffer;
assert(() => 'payload' === decoder.decode(new DataView(viewBuffer, 1, 7)));
assert(() => 'x' === decoder.decode(new Uint8Array([0xEF, 0xBB, 0xBF, 0x78])));
assert(() => '\uFEFFx' === new TextDecoder('utf8', {ignoreBOM: true}).decode(
    new Uint8Array([0xEF, 0xBB, 0xBF, 0x78])));
assert(() => '�(�' === decoder.decode(new Uint8Array([0xE2, 0x28, 0xA1])));
assert(() => throws(() => new TextDecoder('utf-8', {fatal: true}).decode(
    new Uint8Array([0xFF]))));

const streaming = new TextDecoder('unicode-1-1-utf-8');
assert(() => '' === streaming.decode(new Uint8Array([0xE2]), {stream: true}));
assert(() => '€' === streaming.decode(new Uint8Array([0x82, 0xAC])));
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
