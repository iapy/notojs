#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(HTMLExt, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(appendChild)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { image, icon } from 'noto:core';

const d = Document.html();

const i = d.body.appendChild(image('https://imgs.xkcd.com/comics/ahead_stop.png'));
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === i.getAttribute('src'));

const s = d.body.appendChild(await icon('ic/baseline-apple'));
assert(() => 'svg' === s.nodeName);

assert(() => i === d.body.childNodes[0]);
assert(() => s === d.body.childNodes[1]);

const a = await icon('ic/baseline-apple');
a.data = a.data.substr(0, 5);

assert(() => throws(() => d.body.appendChild(a), 'SyntaxError'));
assert(() => throws(() => d.body.appendChild(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertBefore)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { image, icon } from 'noto:core';

const d = Document.html();

const div = d.body.appendChild(d.createElement('div'));
const i = d.body.insertBefore(image('https://imgs.xkcd.com/comics/ahead_stop.png'), div);
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === i.getAttribute('src'));
assert(() => div === i.nextSibling);

const s = d.body.insertBefore(await icon('ic/baseline-apple'), div);
assert(() => 'svg' === s.nodeName);
assert(() => s === i.nextSibling);
assert(() => div === s.nextSibling);

const a = await icon('ic/baseline-apple');
a.data = a.data.substr(0, 5);

assert(() => throws(() => d.body.insertBefore(a, div), 'SyntaxError'));
assert(() => throws(() => d.body.insertBefore(a, d.body), 'NotFoundError'));
assert(() => throws(() => d.body.insertBefore(a, Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChild)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';
import { image, icon, html } from 'noto:core';

const d = Document.html();

const a = d.body.appendChild(d.createElement('a'));
assert(() => a === d.body.replaceChild(image('https://imgs.xkcd.com/comics/ahead_stop.png'), a));
assert(() => !a.isConnected);

const i = d.body.firstElementChild;
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' == i.getAttribute('src'));

const ico = await icon('ic/baseline-apple');
assert(() => i === d.body.replaceChild(ico, i));
assert(() => !i.isConnected);

const s = d.body.firstElementChild;
assert(() => 'svg' === s.nodeName);

assert(() => s === d.body.replaceChild(html('<b>bold</b><i>italic</i>'), s));
assert(() => !s.isConnected);

assert(() => 'B' == d.body.firstChild.nodeName);
assert(() => 'bold' == d.body.firstChild.firstChild.nodeValue);
assert(() => 'I' == d.body.lastChild.nodeName);
assert(() => 'italic' == d.body.lastChild.firstChild.nodeValue);

ico.data = ico.data.substr(0, 5);
assert(() => throws(() => d.body.replaceChild(ico, d.body.firstChild), 'SyntaxError'));
assert(() => throws(() => d.body.replaceChild(html(''), d.body), 'NotFoundError'));
assert(() => throws(() => d.body.replaceChild(html(''), Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(innerHTML)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html } from 'noto:core';

const d = Document.html();

d.body.innerHTML = html('<b>bold</b>');
assert(() => '<b>bold</b>' === d.body.innerHTML);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(outerHTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html } from 'noto:core';

const d = Document.html();

const div = d.body.appendChild(d.createElement('div'));
assert(() => '<body><div></div></body>' === d.body.outerHTML);

div.outerHTML = html('<b>bold</b>');
assert(() => '<body><b>bold</b></body>' === d.body.outerHTML);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(append)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { html, image, icon } from 'noto:core';
import { Document } from 'noto:dom';

const d = Document.html();
d.body.append(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'B' === d.body.childNodes[0].nodeName);
assert(() => 'bold' === d.body.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[1].nodeName);
assert(() => 'italic' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[2].nodeValue);
assert(() => 'IMG' === d.body.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[3].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(after)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { html, image, icon } from 'noto:core';
import { Document } from 'noto:dom';

const d = Document.html();
const e = d.body.appendChild(d.createElement('div'));
e.after(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'DIV' === d.body.childNodes[0].nodeName);
assert(() => 'B' === d.body.childNodes[1].nodeName);
assert(() => 'bold' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[2].nodeName);
assert(() => 'italic' === d.body.childNodes[2].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[3].nodeValue);
assert(() => 'IMG' === d.body.childNodes[4].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[4].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[5].nodeName);

e.after(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'DIV' === d.body.childNodes[0].nodeName);
assert(() => 'B' === d.body.childNodes[1].nodeName);
assert(() => 'bold' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[2].nodeName);
assert(() => 'italic' === d.body.childNodes[2].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[3].nodeValue);
assert(() => 'IMG' === d.body.childNodes[4].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[4].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[5].nodeName);

assert(() => 'B' === d.body.childNodes[6].nodeName);
assert(() => 'bold' === d.body.childNodes[6].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[7].nodeName);
assert(() => 'italic' === d.body.childNodes[7].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[8].nodeValue);
assert(() => 'IMG' === d.body.childNodes[9].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[9].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[10].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(before)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { html, image, icon } from 'noto:core';
import { window, Document } from 'noto:dom';

const d = Document.html();

const e = d.body.appendChild(d.createElement('div'));
e.before(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'B' === d.body.childNodes[0].nodeName);
assert(() => 'bold' === d.body.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[1].nodeName);
assert(() => 'italic' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[2].nodeValue);
assert(() => 'IMG' === d.body.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[3].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[4].nodeName);
assert(() => 'DIV' === d.body.childNodes[5].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertAdjacentElement)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { image, icon } from 'noto:core';
import { window, Document } from 'noto:dom';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));

const i1 = div.insertAdjacentElement('beforebegin', image('https://imgs.xkcd.com/comics/ahead_stop.png'));
const i2 = div.insertAdjacentElement('afterbegin', image('https://imgs.xkcd.com/comics/ahead_stop.png'));
const i3 = div.insertAdjacentElement('beforeend', image('https://imgs.xkcd.com/comics/ahead_stop.png'));
const i4 = div.insertAdjacentElement('afterend', image('https://imgs.xkcd.com/comics/ahead_stop.png'));

assert(() => 'IMG' === i1.nodeName);
assert(() => 'IMG' === i2.nodeName);
assert(() => 'IMG' === i3.nodeName);
assert(() => 'IMG' === i4.nodeName);

assert(() => i1 === div.previousSibling);
assert(() => i2 === div.firstChild);
assert(() => i3 === div.lastChild);
assert(() => i4 === div.nextSibling);

const j1 = div.insertAdjacentElement('beforebegin', await icon('ic/baseline-apple'));
const j2 = div.insertAdjacentElement('afterbegin', await icon('ic/baseline-apple'));
const j3 = div.insertAdjacentElement('beforeend', await icon('ic/baseline-apple'));
const j4 = div.insertAdjacentElement('afterend', await icon('ic/baseline-apple'));

assert(() => 'svg' === j1.nodeName);
assert(() => 'svg' === j2.nodeName);
assert(() => 'svg' === j3.nodeName);
assert(() => 'svg' === j4.nodeName);

assert(() => i1 === j1.previousSibling);
assert(() => j1 === div.previousSibling);
assert(() => j2 === div.firstChild);
assert(() => i2 === j2.nextSibling);
assert(() => i3 === j3.previousSibling);
assert(() => j3 === div.lastChild);
assert(() => j4 === div.nextSibling);
assert(() => i4 === j4.nextSibling);

assert(() => throws(() => div.insertAdjacentElement('a', image('https://imgs.xkcd.com/comics/ahead_stop.png')), 'Wrong position argument [a]'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertAdjacentHTML)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';
import { html } from 'noto:core';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));
div.insertAdjacentHTML('beforebegin', html('<a></a>'));
div.insertAdjacentHTML('afterbegin', html('<b></b>'));
div.insertAdjacentHTML('beforeend', html('<u></u>'));
div.insertAdjacentHTML('afterend', html('<i></i>'));

assert(() => 'A' === div.previousSibling.nodeName);
assert(() => 'B' === div.firstChild.nodeName);
assert(() => 'U' === div.lastChild.nodeName);
assert(() => 'I' === div.nextSibling.nodeName);

div.insertAdjacentHTML('beforebegin', html('<h1></h1>'));
div.insertAdjacentHTML('afterbegin', html('<li></li>'));
div.insertAdjacentHTML('beforeend', html('<ul></ul>'));
div.insertAdjacentHTML('afterend', html('<h2></h2>'));

assert(() => 'A' === div.previousSibling.previousSibling.nodeName);
assert(() => 'H1' === div.previousSibling.nodeName);
assert(() => 'LI' === div.firstChild.nodeName);
assert(() => 'UL' === div.lastChild.nodeName);
assert(() => 'H2' === div.nextSibling.nodeName);
assert(() => 'I' === div.nextSibling.nextSibling.nodeName);

assert(() => throws(() => div.insertAdjacentHTML('a', html('')), 'Wrong position argument [a]'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(prepend)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));

div.prepend(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'B' === div.childNodes[0].nodeName);
assert(() => 'bold' === div.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === div.childNodes[1].nodeName);
assert(() => 'italic' === div.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === div.childNodes[2].nodeValue);
assert(() => 'IMG' === div.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === div.childNodes[3].getAttribute('src'));
assert(() => 'svg' === div.childNodes[4].nodeName);

div.prepend(html('<u>underlined</u>'), 'foo', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 'U' === div.childNodes[0].nodeName);
assert(() => 'underlined' === div.childNodes[0].firstChild.nodeValue);
assert(() => 'foo' === div.childNodes[1].nodeValue);
assert(() => 'IMG' === div.childNodes[2].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === div.childNodes[2].getAttribute('src'));
assert(() => 'svg' === div.childNodes[3].nodeName);
assert(() => 'B' === div.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChildren)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html('<body><div></div>foo<p></p></body>');
const c = [...d.body.childNodes];

assert(() => 3 === c.length);
for(const n of c) assert(() => n.isConnected);

d.body.replaceChildren(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));
for(const n of c) assert(() => !n.isConnected);

assert(() => 'B' === d.body.childNodes[0].nodeName);
assert(() => 'bold' === d.body.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[1].nodeName);
assert(() => 'italic' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[2].nodeValue);
assert(() => 'IMG' === d.body.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[3].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceWith)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const e = d.body.appendChild(d.createElement('div'));
e.replaceWith(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => e.parentNode === null);
assert(() => 'B' === d.body.childNodes[0].nodeName);
assert(() => 'bold' === d.body.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === d.body.childNodes[1].nodeName);
assert(() => 'italic' === d.body.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === d.body.childNodes[2].nodeValue);
assert(() => 'IMG' === d.body.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === d.body.childNodes[3].getAttribute('src'));
assert(() => 'svg' === d.body.childNodes[4].nodeName);

const s = d.body.appendChild(d.createElement('span'));
const beforeSelf = s.previousSibling;
s.replaceWith('before', s, html('<u>underlined</u>'));
assert(() => s.parentNode === d.body);
assert(() => 'before' === s.previousSibling.nodeValue);
assert(() => beforeSelf === s.previousSibling.previousSibling);
assert(() => 'U' === s.nextSibling.nodeName);
assert(() => 'underlined' === s.nextSibling.firstChild.nodeValue);
assert(() => null === s.nextSibling.nextSibling);

const a = await icon('ic/baseline-apple');
a.data = a.data.substr(0, 5);
assert(() => throws(() => d.body.firstChild.replaceWith(a), 'SyntaxError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(atomicFallbackMutations)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { html } from 'noto:core';
import { Document } from 'noto:dom';

const d = Document.html();

{
    const target = d.createElement('target');
    target.append(html('<b>bold</b>'));
    assert(() => 'B' === target.firstChild.nodeName);
    assert(() => 'bold' === target.firstChild.firstChild.nodeValue);
}

for(const method of ['append', 'prepend']) {
    const target = d.createElement('target');
    const source = d.createElement('source');
    const valid = source.appendChild(d.createElement('valid'));

    assert(() => throws(() => target[method](valid, html('')), 'SyntaxError'));
    assert(() => valid === source.firstChild);
    assert(() => !target.hasChildNodes());
}

for(const method of ['before', 'after']) {
    const parent = d.createElement('parent');
    const target = parent.appendChild(d.createElement('target'));
    const source = d.createElement('source');
    const valid = source.appendChild(d.createElement('valid'));

    assert(() => throws(() => target[method](valid, html('')), 'SyntaxError'));
    assert(() => valid === source.firstChild);
    assert(() => target === parent.firstChild);
    assert(() => 1 === parent.childNodes.length);
}

{
    const target = d.createElement('target');
    const existing = target.appendChild(d.createElement('existing'));
    const source = d.createElement('source');
    const valid = source.appendChild(d.createElement('valid'));

    assert(() => throws(() => target.replaceChildren(valid, html('')), 'SyntaxError'));
    assert(() => valid === source.firstChild);
    assert(() => existing === target.firstChild);
    assert(() => 1 === target.childNodes.length);
}

{
    const parent = d.createElement('parent');
    const target = parent.appendChild(d.createElement('target'));
    const source = d.createElement('source');
    const valid = source.appendChild(d.createElement('valid'));

    assert(() => throws(() => target.replaceWith(valid, html('')), 'SyntaxError'));
    assert(() => valid === source.firstChild);
    assert(() => target === parent.firstChild);
    assert(() => 1 === parent.childNodes.length);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentAppend)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();
f.append(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 5 === f.childNodes.length);
assert(() => 4 === f.childElementCount);
assert(() => 'B' === f.childNodes[0].nodeName);
assert(() => 'bold' === f.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === f.childNodes[1].nodeName);
assert(() => 'italic' === f.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === f.childNodes[2].nodeValue);
assert(() => 'IMG' === f.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === f.childNodes[3].getAttribute('src'));
assert(() => 'svg' === f.childNodes[4].nodeName);
assert(() => f.childNodes[0] === f.firstElementChild);
assert(() => f.childNodes[4] === f.lastElementChild);
assert(() => f.childNodes[0] === f.children[0]);
assert(() => f.childNodes[1] === f.children[1]);
assert(() => f.childNodes[3] === f.children[2]);
assert(() => f.childNodes[4] === f.children[3]);
assert(() => f.childNodes[0] === f.querySelector('b'));
assert(() => 4 === f.querySelectorAll('b,i,img,svg').length);

d.body.appendChild(f);
assert(() => 0 === f.childNodes.length);
assert(() => 'B' === d.body.childNodes[0].nodeName);
assert(() => 'I' === d.body.childNodes[1].nodeName);
assert(() => 'text' === d.body.childNodes[2].nodeValue);
assert(() => 'IMG' === d.body.childNodes[3].nodeName);
assert(() => 'svg' === d.body.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentPrepend)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();
f.append(d.createElement('div'));
f.prepend(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));

assert(() => 6 === f.childNodes.length);
assert(() => 'B' === f.childNodes[0].nodeName);
assert(() => 'bold' === f.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === f.childNodes[1].nodeName);
assert(() => 'italic' === f.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === f.childNodes[2].nodeValue);
assert(() => 'IMG' === f.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === f.childNodes[3].getAttribute('src'));
assert(() => 'svg' === f.childNodes[4].nodeName);
assert(() => 'DIV' === f.childNodes[5].nodeName);

f.prepend(html('<u>underlined</u>'), 'foo', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));
assert(() => 'U' === f.childNodes[0].nodeName);
assert(() => 'underlined' === f.childNodes[0].firstChild.nodeValue);
assert(() => 'foo' === f.childNodes[1].nodeValue);
assert(() => 'IMG' === f.childNodes[2].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === f.childNodes[2].getAttribute('src'));
assert(() => 'svg' === f.childNodes[3].nodeName);
assert(() => 'B' === f.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentReplaceChildren)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();
f.append(d.createElement('div'), 'foo', d.createElement('p'));
const c = [...f.childNodes];

assert(() => 3 === c.length);
for(const n of c) assert(() => !n.isConnected);

f.replaceChildren(html('<b>bold</b><i>italic</i>'), 'text', image('https://imgs.xkcd.com/comics/ahead_stop.png'), await icon('ic/baseline-apple'));
assert(() => 3 === c.length);
assert(() => 5 === f.childNodes.length);
for(const n of c) assert(() => n.parentNode === null);

assert(() => 'B' === f.childNodes[0].nodeName);
assert(() => 'bold' === f.childNodes[0].firstChild.nodeValue);
assert(() => 'I' === f.childNodes[1].nodeName);
assert(() => 'italic' === f.childNodes[1].firstChild.nodeValue);
assert(() => 'text' === f.childNodes[2].nodeValue);
assert(() => 'IMG' === f.childNodes[3].nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === f.childNodes[3].getAttribute('src'));
assert(() => 'svg' === f.childNodes[4].nodeName);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentAppendChild)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();

const i = f.appendChild(image('https://imgs.xkcd.com/comics/ahead_stop.png'));
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === i.getAttribute('src'));

const s = f.appendChild(await icon('ic/baseline-apple'));
assert(() => 'svg' === s.nodeName);

assert(() => i === f.childNodes[0]);
assert(() => s === f.childNodes[1]);
assert(() => i.parentNode === f);
assert(() => s.parentNode === f);
assert(() => !i.isConnected);
assert(() => !s.isConnected);

d.body.appendChild(f);
assert(() => 0 === f.childNodes.length);
assert(() => i === d.body.childNodes[0]);
assert(() => s === d.body.childNodes[1]);
assert(() => i.isConnected);
assert(() => s.isConnected);

const a = await icon('ic/baseline-apple');
a.data = a.data.substr(0, 5);
assert(() => throws(() => d.createDocumentFragment().appendChild(a), 'SyntaxError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentInsertBefore)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();

const div = f.appendChild(d.createElement('div'));
const i = f.insertBefore(image('https://imgs.xkcd.com/comics/ahead_stop.png'), div);
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === i.getAttribute('src'));
assert(() => i === f.firstChild);
assert(() => div === i.nextSibling);

const s = f.insertBefore(await icon('ic/baseline-apple'), div);
assert(() => 'svg' === s.nodeName);
assert(() => s === i.nextSibling);
assert(() => div === s.nextSibling);

const tail = f.insertBefore(image('https://imgs.xkcd.com/comics/ahead_stop.png'), null);
assert(() => 'IMG' === tail.nodeName);
assert(() => tail === f.lastChild);

const a = await icon('ic/baseline-apple');
a.data = a.data.substr(0, 5);
assert(() => throws(() => f.insertBefore(a, div), 'SyntaxError'));
assert(() => throws(() => f.insertBefore(a, d.body), 'NotFoundError'));
assert(() => throws(() => f.insertBefore(a, Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentFragmentReplaceChild)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';
import { html, image, icon } from 'noto:core';

const d = Document.html();
const f = d.createDocumentFragment();

const a = f.appendChild(d.createElement('a'));
assert(() => a === f.replaceChild(image('https://imgs.xkcd.com/comics/ahead_stop.png'), a));
assert(() => a.parentNode === null);

const i = f.firstChild;
assert(() => 'IMG' === i.nodeName);
assert(() => 'https://imgs.xkcd.com/comics/ahead_stop.png' === i.getAttribute('src'));

const ico = await icon('ic/baseline-apple');
assert(() => i === f.replaceChild(ico, i));
assert(() => i.parentNode === null);

const s = f.firstChild;
assert(() => 'svg' === s.nodeName);

assert(() => s === f.replaceChild(html('<b>bold</b><i>italic</i>'), s));
assert(() => s.parentNode === null);

assert(() => 2 === f.childNodes.length);
assert(() => 'B' === f.firstChild.nodeName);
assert(() => 'bold' === f.firstChild.firstChild.nodeValue);
assert(() => 'I' === f.lastChild.nodeName);
assert(() => 'italic' === f.lastChild.firstChild.nodeValue);

ico.data = ico.data.substr(0, 5);
assert(() => throws(() => f.replaceChild(ico, f.firstChild), 'SyntaxError'));
assert(() => throws(() => f.replaceChild(html(''), d.body), 'NotFoundError'));
assert(() => throws(() => f.replaceChild(html(''), Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(nodeMixinCharacterData)
{
    db();

    eval(R"JS(
import { assert } from 'noto:assert';
import { html } from 'noto:core';
import { Document } from 'noto:dom';

const d = Document.html();
const host = d.body.appendChild(d.createElement('div'));
const comment = host.appendChild(d.createComment('marker'));

assert(() => undefined === comment.append);

comment.before(html('<b>before</b><i>before too</i>'));
assert(() => 'B' === host.childNodes[0].nodeName);
assert(() => 'I' === host.childNodes[1].nodeName);
assert(() => comment === host.childNodes[2]);

comment.after(html('<u>after</u><em>after too</em>'));
assert(() => 'U' === comment.nextSibling.nodeName);
assert(() => 'EM' === comment.nextSibling.nextSibling.nodeName);

comment.replaceWith(html('<strong>replacement</strong><small>tail</small>'));
assert(() => null === comment.parentNode);
assert(() => 'STRONG' === host.childNodes[2].nodeName);
assert(() => 'SMALL' === host.childNodes[3].nodeName);
assert(() => 6 === host.childNodes.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
