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

BOOST_AUTO_TEST_SUITE_END()

