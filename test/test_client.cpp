#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Client, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(FetchError)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => fetch(), 'No matching function overload found'));
assert(() => throws(() => fetch(1), 'No matching function overload found'));
assert(() => throws(() => fetch('http://abracad.abra'), 'Host not found (authoritative)'));
assert(() => throws(() => fetch('http://127.0.0.1:7777'), 'Connection refused'));
assert(() => throws(() => fetch(new Request(), 0), 'No matching function overload found'));
assert(() => throws(() => fetch(new Request('http://abracad.abra/')), 'Host not found (authoritative)'));
assert(() => throws(() => fetch(new Request('http://127.0.0.1:7777')), 'Connection refused'));
assert(() => throws(() => fetch('abracadabra'), 'invalid url [abracadabra]'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Fetch404)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/404');
assert(() => !response.ok);
assert(() => !response.redirected);
assert(() => 404 == response.status);
assert(() => 'Not Found' == response.status_text);
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchText)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/text');
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'text/plain' == response.headers.get('Content-Type'));

const text = await response.text();
assert(() => 'text' == text);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchTextRequest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch(new Request('/text'));
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/text'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'text/plain' == response.headers.get('Content-Type'));

const text = await response.text();
assert(() => 'text' == text);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchJson)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json');
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));

const json = await response.json();
assert(() => 42 == json.number);
assert(() => 'string' == json.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchJsonRequest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch(new Request('/json'));
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));

const json = await response.json();
assert(() => 42 == json.number);
assert(() => 'string' == json.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchBadJson)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json/bad');
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json/bad'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));
assert(() => throws(() => response.json()));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchBadJsonRequest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch(new Request('/json/bad'));
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.headers.has('Server'));
assert(() => response.url.endsWith('/json/bad'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));
assert(() => throws(() => response.json()));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchPostJson)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json', {
    method: 'POST',
    body: {'foo': 'bar', 'bar': 42}
});
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));

const json = await response.json();
assert(() => 'bar' == json.foo);
assert(() => 42 == json.bar);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchPostJsonRequest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch(new Request('/json', {
    method: 'POST',
    body: {'bar': 'foo', 'foo': 42}
}));
assert(() => response.ok);
assert(() => !response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));

const json = await response.json();
assert(() => 42 == json.foo);
assert(() => 'foo' == json.bar);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchMethodNotAllowed)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json', {
    method: 'PUT',
    body: {'foo': 'bar', 'bar': 42}
});
assert(() => !response.ok);
assert(() => !response.redirected);
assert(() => 405 == response.status);
assert(() => response.url.endsWith('/json'));
assert(() => 'Method Not Allowed' == response.status_text);
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchMethodNotAllowedRequest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch(new Request('/json', {
    method: 'PUT',
    body: {'foo': 'bar', 'bar': 42}
}));
assert(() => !response.ok);
assert(() => !response.redirected);
assert(() => 405 == response.status);
assert(() => response.url.endsWith('/json'));
assert(() => 'Method Not Allowed' == response.status_text);
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Redirect)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

assert(() => throws(() => fetch('/redirect', {redirect: 'error'}), 'Redirect was not allowed'));
assert(() => throws(() => fetch('/redirect/noloc'), 'Redirect without location'));
assert(() => throws(() => fetch('/redirect/badloc'), 'Bad redirect location'));

var response = await fetch('/redirect', {redirect: 'manual'});
assert(() => 302 == response.status);
assert(() => '/json' == response.headers.get('Location'));

response = await fetch('/redirect');
assert(() => response.ok);
assert(() => response.redirected);
assert(() => 200 == response.status);
assert(() => 'OK' == response.status_text);
assert(() => response.url.endsWith('/json'));
assert(() => response.headers.has('Server'));
assert(() => 'boost:beast' == response.headers.get('Server'));
assert(() => response.headers.has('Content-Type'));
assert(() => 'application/json' == response.headers.get('Content-Type'));

response = await fetch('/redirect/2');
assert(() => response.ok);
assert(() => response.redirected);
assert(() => response.url.endsWith('/json'));

response = await fetch('/redirect/https');
assert(() => response.ok);
assert(() => response.redirected);
assert(() => 'https://api.myip.com/' == response.url);
assert(() => 200 == response.status);

const json = await response.json();
assert(() => typeof(json['cc']) != 'undefined');
assert(() => typeof(json['ip']) != 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Redirect303)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

var response = await fetch('/redirect/post', {
    method: 'POST'
});
assert(() => response.ok);
assert(() => response.redirected);
assert(() => 200 == response.status);
assert(() => 'application/json' == response.headers.get('Content-Type'));

const json = await response.json();
assert(() => 42 == json.number);
assert(() => 'string' == json.string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Https)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const r1 = await fetch('https://api.myip.com/');
assert(() => r1.ok);
assert(() => !r1.redirected);

const j1 = await r1.json();
assert(() => typeof(j1['cc']) != 'undefined');
assert(() => typeof(j1['ip']) != 'undefined');

const r2 = await fetch(new URL('https://api.myip.com/'));
assert(() => r2.ok);
assert(() => !r2.redirected);

const j2 = await r2.json();
assert(() => typeof(j2['cc']) != 'undefined');
assert(() => typeof(j2['ip']) != 'undefined');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchAll)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const json = await Promise.all([
    fetch('/json').then((r) => r.json()),
    fetch('/json').then((r) => r.json())
]);

assert(() => 2 == json.length);
assert(() => 42 == json[0].number);
assert(() => 'string' == json[0].string);
assert(() => 42 == json[1].number);
assert(() => 'string' == json[1].string);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchArrayBuffer)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json');
const txt = await response.text();
const arr = await response.arrayBuffer();

assert(() => arr.byteLength == txt.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchBlob)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/json');
const text = await response.text();
const blob = await response.blob();
const abuf = await response.arrayBuffer();

assert(() => text.length == blob.size);

const data = await blob.arrayBuffer();
assert(() => text.length == data.byteLength);

const btxt = await blob.text();
const ctxt = await (new Blob([abuf]).text());
assert(() => text == btxt);
assert(() => text == ctxt);
assert(() => 'application/json' == blob.type);

const b64 = await URL.createObjectURL(blob);
const d = "data:application/json;base64,";
assert(() => b64.substr(0, d.length) == d);
URL.revokeObjectURL(b64);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(FetchMemory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

async function get() {
    const response = await fetch('/json');
    const text = await response.text();
    const blob = await response.blob();
    const bytes = await response.bytes();
    return [text, blob, bytes, response.headers];
}

const [text, blob, bytes, headers] = await get();
assert(() => text.length == blob.size);
assert(() => text.length == bytes.length);

const data = await blob.arrayBuffer();
assert(() => text.length == data.byteLength);

const btxt = await blob.text();
assert(() => text == btxt);
assert(() => 'application/json' == blob.type);

assert(() => 'application/json' == headers.get('Content-Type'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(API)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { load } from 'global.so';

const response = await load('/text');

assert(() => response.ok);
assert(() => 200 == response.status);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Inheritance)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const response = await fetch('/text');

assert(() => response.ok);
assert(() => 200 == response.status);
assert(() => !response.headers.has('X-Foo'));

const t1 = await response.text();
assert(() => 'text' == t1);

assert(() => throws(() => response.body = 'foobar', 'no setter for property'));
assert(() => throws(() => response.status = 500, 'no setter for property'));
response.headers.set('X-Foo', 'bar');

assert(() => response.ok);
assert(() => 200 == response.status);
assert(() => response.headers.has('X-Foo'));

const t2 = await response.text();
assert(() => 'text' == t2);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
