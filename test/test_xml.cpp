#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(XML, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Memory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const e = (function() {
    const doc = Document.xml('<root/>');
    return doc.createElement('foo');
})();
assert(() => null === e._document);

const d1 = e.ownerDocument;
assert(() => null !== d1);

const d2 = e.ownerDocument;
assert(() => null !== d2);
assert(() => d1 === d2);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Document)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

assert(() => throws(() => new window.XMLDocument()));

const d = Document.xml('<root/>');

assert(() => d instanceof window.Document);
assert(() => d instanceof window.XMLDocument);
assert(() => !(d instanceof window.HTMLDocument));

assert(() => 'about:blank' === d.baseURI);
assert(() => d.isConnected);
assert(() => null === d.nextSibling);
assert(() => '#document' == d.nodeName);
assert(() => undefined == d.tagName);
assert(() => null === d.nodeValue);
assert(() => null === d.parentElement);
assert(() => null === d.parentNode);
assert(() => null === d.previousSibling);

const e1 = d.documentElement;
const e2 = d.documentElement;

assert(() => e1 instanceof window.Element);
assert(() => !(e1 instanceof window.HTMLElement));

assert(() => e1 === e2);
assert(() => 'about:blank' === e1.baseURI);
assert(() => e1.isConnected)
assert(() => e1 == d.firstChild);
assert(() => e1 == d.lastChild);
assert(() => null === e1.nextSibling);
assert(() => null === e1.nextElementSibling);
assert(() => 'root' === e1.nodeName);
assert(() => 'root' === e1.tagName);
assert(() => 1 === e1.nodeType);
assert(() => d === e1.parentElement);
assert(() => d.isSameNode(e1.parentElement));
assert(() => d === e1.parentNode);
assert(() => d.isSameNode(e1.parentNode));
assert(() => null === e1.previousSibling);
assert(() => null === e1.previousElementSibling);
assert(() => null === e1.firstChild);
assert(() => null === e1.lastChild);

const j1 = d.toJSON();
assert(() => '<?xml version=\"1.0\"?>\n<root/>' === j1.data.trim());
assert(() => 'notojs.XML' === j1.type);

const j2 = e1.toJSON();
assert(() => '<root/>' === j2.data.trim());
assert(() => 'notojs.XML' === j2.type);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Serialize)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, html } from 'noto:core';
import { XML, xml } from 'noto:core';
import { Document } from 'noto:dom';

const d = Document.xml('<root/>');
const a = d.documentElement.appendChild(d.createElement('a'));

assert(() => throws(() => html(d), "No matching function overload found"));
assert(() => throws(() => html(a), "No matching function overload found"));

assert(() => xml(d) instanceof XML);
assert(() => xml(d).toJSON() instanceof Object);

assert(() => xml(d.documentElement) instanceof XML);
assert(() => xml(d.documentElement).toJSON() instanceof Object);

assert(() => xml(a) instanceof XML);
assert(() => xml(a).toJSON() instanceof Object);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(createElement)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
assert(() => a instanceof window.Element);
assert(() => !(a instanceof window.HTMLElement));
assert(() => 'about:blank' === a.baseURI);
assert(() => !a.isConnected);
assert(() => null === a.firstChild);
assert(() => null === a.lastChild);
assert(() => null === a.nextSibling);
assert(() => null === a.nextElementSibling);
assert(() => 'a' === a.nodeName);
assert(() => 'a' === a.tagName);
assert(() => 1 === a.nodeType);
assert(() => null == a.nodeValue);
assert(() => d === a.ownerDocument);
assert(() => d.isSameNode(a.ownerDocument));
assert(() => null === a.parentNode);
assert(() => null === a.parentElement);
assert(() => null === a.previousSibling);
assert(() => null === a.previousElementSibling);
assert(() => '<a/>' === a.toJSON().data.trim());
assert(() => 'notojs.XML' === a.toJSON().type);
assert(() => null === d.documentElement.nextSibling);
assert(() => null === d.documentElement.nextElementSibling);
print(a);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.XML"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "<a/>\n"));
}

BOOST_AUTO_TEST_CASE(attributes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
const n = a.attributes;

assert(() => n === a.attributes);
assert(() => n instanceof window.NamedNodeMap);

assert(() => !a.hasAttribute('href'));
assert(() => null === a.getAttribute('href'));
assert(() => null === n.getNamedItem('href'));

a.setAttribute('href', 'http://apple.com');
assert(() => a.hasAttribute('href'));
assert(() => 'http://apple.com' == a.getAttribute('href'));

let href = n.getNamedItem('href');
assert(() => href.isConnected);
assert(() => href === n.getNamedItem('href'));
assert(() => href instanceof window.Attr);
assert(() => href instanceof window.Node);

assert(() => 'http://apple.com' == href.value);
assert(() => 'http://apple.com' == href.nodeValue);
assert(() => 'http://apple.com' == href.textContent);

a.removeAttribute('alt');
n.removeNamedItem('alt');
assert(() => a.hasAttribute('href'));

a.removeAttribute('href');
assert(() => !a.hasAttribute('href'));
assert(() => null === a.getAttribute('href'));

assert(() => !href.isConnected);
assert(() => null === n.getNamedItem('href'));
assert(() => 'http://apple.com' == href.value);
assert(() => 'http://apple.com' == href.nodeValue);
assert(() => 'http://apple.com' == href.textContent);

a.setAttribute('href', 1);
assert(() => a.hasAttribute('href'));
assert(() => '1' == a.getAttribute('href'));

assert(() => !href.isConnected);
assert(() => null !== n.getNamedItem('href'));

href = n.getNamedItem('href');
assert(() => null !== href);
assert(() => href === n.getNamedItem('href'));

assert(() => href === n.removeNamedItem('href'));
assert(() => null === n.getNamedItem('href'));
assert(() => !href.isConnected);
assert(() => '1' === href.value);

href.value = '2';
assert(() => '2' === href.value);
assert(() => null === a.getAttribute('href'));

a.setAttribute('href', null);
assert(() => a.hasAttribute('href'));
assert(() => 'null' == a.getAttribute('href'));

a.setAttribute('href', true);
assert(() => a.hasAttribute('href'));
assert(() => 'true' == a.getAttribute('href'));

a.setAttribute('href', false);
assert(() => a.hasAttribute('href'));
assert(() => 'false' == a.getAttribute('href'));

let arr = [...a.attributes];

assert(() => 1 === arr.length);
for(const attr of a.attributes)
    assert(() => attr === arr.shift());
assert(() => 0 === arr.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(namedNodeMap)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
assert(() => null === d.documentElement.getAttributeNode('attr'));

const a = d.documentElement.appendChild(d.createElement('div')).attributes;
const b = d.documentElement.appendChild(d.createElement('div')).attributes;

assert(() => 0 === a.length);
assert(() => 0 === b.length);
assert(() => throws(() => a.setNamedItem('a', 'b'), 'No matching function overload found'));

d.documentElement.children[0].setAttribute('foo', 'bar');
d.documentElement.children[1].setAttribute('foo', 'baz');

const bar = a.getNamedItem('foo');
const baz = b.getNamedItem('foo');
assert(() => !bar.hasChildNodes());
assert(() => !baz.hasChildNodes());
assert(() => bar === d.documentElement.children[0].getAttributeNode('foo'));
assert(() => baz === d.documentElement.children[1].getAttributeNode('foo'));
assert(() => d.documentElement.children[0] === bar.ownerElement);
assert(() => d.documentElement.children[1] === baz.ownerElement);
assert(() => null === bar.namespaceURI);
assert(() => null === baz.namespaceURI);
assert(() => baz === b.setNamedItem(bar));
assert(() => !baz.isConnected);
assert(() => null === baz.ownerElement);
assert(() => null === a.setNamedItem(baz));
assert(() => baz.isConnected);
assert(() => d.documentElement.children[1] === bar.ownerElement);
assert(() => d.documentElement.children[0] === baz.ownerElement);
assert(() => d === bar.getRootNode());
assert(() => d === baz.getRootNode());
assert(() => !bar.contains(baz));
assert(() => !bar.contains(d.documentElement.children[1]));
assert(() => d.documentElement.children[1].contains(bar));
assert(() => d.documentElement.children[0].contains(baz));

assert(() => baz === a.item(0));
assert(() => bar === b.item(0));
assert(() => null === a.item(1));
assert(() => null === b.item(1));
assert(() => null === a.item(-1));
assert(() => null === b.item(-1));

assert(() => baz === a[0]);
assert(() => bar === b[0]);
assert(() => undefined === a[1]);
assert(() => undefined === b[1]);
assert(() => undefined === a[-1]);
assert(() => undefined === b[-1]);

assert(() => bar === b.getNamedItem('foo'));
assert(() => baz === a.getNamedItem('foo'));
assert(() => bar.isSameNode(b.getNamedItem('foo')));
assert(() => baz.isSameNode(a.getNamedItem('foo')));
assert(() => !bar.isSameNode(d.documentElement.children[0]));
assert(() => !d.documentElement.children[0].isSameNode(bar));
assert(() => !bar.isEqualNode(d.documentElement.children[0]));
assert(() => !d.documentElement.children[0].isEqualNode(bar));
assert(() => !bar.isSameNode(baz));

assert(() => bar.compareDocumentPosition(baz) === d.documentElement.children[1].compareDocumentPosition(d.documentElement.children[0]));
assert(() => baz.compareDocumentPosition(bar) === d.documentElement.children[0].compareDocumentPosition(d.documentElement.children[1]));

d.documentElement.children[0].setAttribute('bar', 'foo');
const foo = a.getNamedItem('bar');

assert(() => 2 === foo.compareDocumentPosition(baz));
assert(() => 4 === baz.compareDocumentPosition(foo));

foo.normalize();
assert(() => 'foo' === foo.value);
assert(() => foo === d.documentElement.children[0].removeAttributeNode(foo));
assert(() => !foo.isConnected);
assert(() => throws(() => d.documentElement.children[0].removeAttributeNode(foo), 'NotFoundError'));
assert(() => null === d.documentElement.children[0].setAttributeNode(foo));
assert(() => foo.isConnected);

const buz = bar.cloneNode();
assert(() => !buz.isConnected);
assert(() => buz.isEqualNode(bar));
assert(() => !buz.isSameNode(bar));
assert(() => null === buz.getRootNode());

assert(() => throws(() => buz.appendChild(d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => buz.removeChild(d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => buz.replaceChild(d.documentElement, d.documentElement), 'HierarchyRequestError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Text)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const t = d.createTextNode('Some text');
assert(() => t instanceof window.Text);
assert(() => 'about:blank' === t.baseURI);
assert(() => !t.isConnected);
assert(() => null === t.firstChild);
assert(() => null === t.lastChild);
assert(() => null === t.nextSibling);
assert(() => '#text' == t.nodeName);
assert(() => 3 == t.nodeType);
assert(() => 'Some text' == t.nodeValue);
assert(() => d === t.ownerDocument);
assert(() => null === t.parentElement);
assert(() => null === t.parentNode);
assert(() => null === d.documentElement.nextSibling);
assert(() => null === d.documentElement.nextElementSibling);

const j1 = d.toJSON();
assert(() => '<?xml version=\"1.0\"?>\n<root/>' === j1.data.trim());
assert(() => 'notojs.XML' === j1.type);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(childNodes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const c = d.documentElement.childNodes;
assert(() => c === d.documentElement.childNodes);
assert(() => c instanceof window.NodeList);

assert(() => 0 === c.length);
assert(() => null === c.item(0));
assert(() => undefined === c[0]);
assert(() => undefined === c['a']);

const a = d.documentElement.appendChild(d.createElement('a'));
assert(() => 1 === c.length);
assert(() => 'a' == c[0].nodeName);
assert(() => 'a' == c[0].tagName);
assert(() => d.documentElement.firstChild === c[0]);
assert(() => d.documentElement.firstChild === c.item(0));
assert(() => null === c.item(1));
assert(() => undefined === c[1]);

const b = d.documentElement.appendChild(d.createElement('b'));
assert(() => 2 === c.length);
assert(() => 'b' == c[1].nodeName);
assert(() => 'b' == c[1].tagName);
assert(() => d.documentElement.firstChild === c[0]);
assert(() => d.documentElement.lastChild === c[1]);
assert(() => d.documentElement.firstChild === c.item(0));
assert(() => d.documentElement.lastChild === c.item(1));
assert(() => null === c.item(2));
assert(() => undefined === c[2]);

let n = 0;
for(const e of c) {
    if(0 == n) assert(() => a === e);
    if(1 == n) assert(() => b === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.entries()) {
    assert(() => n == e[0]);
    if(0 == n) assert(() => a === e[1]);
    if(1 == n) assert(() => b === e[1]);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.keys()) {
    assert(() => n === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
c.forEach((e, i, s) => {
    assert(() => s == c);
    assert(() => i == n);
    if(0 == n) assert(() => a === e);
    if(1 == n) assert(() => b === e);
    ++n;
});

const v = new Array();
c.forEach(function(e, i, s) {
    assert(() => s == c);
    assert(() => i == this.length);
    if(0 == this.length) assert(() => a === e);
    if(1 == this.length) assert(() => b === e);
    this.push(e);
}, v);

assert(() => 2 == v.length);
assert(() => a == v[0]);
assert(() => b == v[1]);

assert(() => throws(() => c.forEach(() => { throw 1 })));
assert(() => throws(() => c.forEach(() => { throw 1 }, 0)));

const x = Array.from(c);
assert(() => 2 == x.length);
assert(() => a == x[0]);
assert(() => b == x[1]);

d.documentElement.removeChild(d.documentElement.firstChild);
assert(() => 1 === c.length);
assert(() => 'b' == c[0].nodeName);
assert(() => 'b' == c[0].tagName);
assert(() => d.documentElement.firstChild === c[0]);
assert(() => d.documentElement.firstChild === c.item(0));
assert(() => null === c.item(1));
assert(() => undefined === c[1]);

n = 0;
for(const e of c) {
    if(0 == n) assert(() => b === e);
    ++n;
}
assert(() => 1 == n);

n = 0;
for(const e of c.entries()) {
    assert(() => n === e[0]);
    if(0 == n) assert(() => b === e[1]);
    ++n;
}
assert(() => 1 === n);

n = 0;
for(const e of c.keys()) {
    assert(() => n === e);
    ++n;
}
assert(() => 1 === n);

const y = Array.from(c);
assert(() => 1 == y.length);
assert(() => b == y[0]);

for(let i = 0; i < 100; ++i)
    d.documentElement.appendChild(d.createElement('h' + i));

assert(() => 101 == c.length);

while(c.length) d.documentElement.removeChild(c[0]);
assert(() => 0 == c.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(clone)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const a = d.createElement('a');
a.setAttribute('href', 'https://apple.co.uk');
a.appendChild(d.createElement('b')).appendChild(d.createTextNode('Apple'));
d.documentElement.appendChild(a);

const b = a.cloneNode();
assert(() => !b.isConnected);
assert(() => null == b.firstChild);
assert(() => b.hasAttribute('href'));
assert(() => 'https://apple.co.uk' == b.getAttribute('href'));

const c = a.cloneNode(false);
assert(() => !c.isConnected);
assert(() => null == c.firstChild);
assert(() => c.hasAttribute('href'));
assert(() => 'https://apple.co.uk' == c.getAttribute('href'));

const e = a.cloneNode(true);
assert(() => !e.isConnected);
assert(() => 'b' == e.firstChild.nodeName);
assert(() => 'b' == e.firstChild.tagName);
assert(() => 'Apple' == e.firstChild.firstChild.nodeValue);
assert(() => e.hasAttribute('href'));
assert(() => 'https://apple.co.uk' == e.getAttribute('href'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(compareDocumentPosition)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d1 = Document.xml('<root/>');
const d2 = Document.xml('<root/>');

const a = d1.createElement('a');
const b = d1.createElement('b');
const c = d2.createElement('c');
assert(() => 1 === d1.compareDocumentPosition(a));
assert(() => 1 === a.compareDocumentPosition(d1));
assert(() => 0 === a.compareDocumentPosition(a));
assert(() => 1 === a.compareDocumentPosition(b));
assert(() => 1 === a.compareDocumentPosition(c));
assert(() => 1 === c.compareDocumentPosition(a));
assert(() => 1 === c.compareDocumentPosition(b));

const div = d1.createElement('div');
div.appendChild(a);

assert(() => throws(() => a.appendChild(div), 'HierarchyRequestError'));

assert(() => 1 === a.compareDocumentPosition(b));
assert(() => 1 === b.compareDocumentPosition(a));
assert(() => 10 === div.compareDocumentPosition(a));
assert(() => 20 === a.compareDocumentPosition(div));

div.appendChild(b);

assert(() => 2 === a.compareDocumentPosition(b));
assert(() => 4 === b.compareDocumentPosition(a));
assert(() => 10 === div.compareDocumentPosition(b));
assert(() => 20 === b.compareDocumentPosition(div));

const d = d1.createElement('d');
a.appendChild(d);

assert(() => 4 === b.compareDocumentPosition(d));
assert(() => 2 === d.compareDocumentPosition(b));
assert(() => 10 === a.compareDocumentPosition(d));
assert(() => 20 === d.compareDocumentPosition(a));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(contains)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d1 = Document.xml('<root/>');
const d2 = Document.xml('<root/>');

const a = d1.createElement('a');
const b = d1.createElement('b');
const c = d2.createElement('c');
const d = d1.createElement('d');

assert(() => !a.contains(null));
assert(() => !d1.contains(null));
assert(() => !d1.contains(a));
assert(() => !d1.contains(b));
assert(() => !d1.contains(c));
assert(() => !d1.contains(d));

assert(() => !d1.documentElement.contains(a));
assert(() => !d1.documentElement.contains(b));
assert(() => !d1.documentElement.contains(c));
assert(() => !d1.documentElement.contains(d));

a.appendChild(b);
assert(() => a.contains(b));

d1.documentElement.appendChild(a);

assert(() => a.contains(b));
assert(() => d1.contains(a));
assert(() => d1.contains(b));
assert(() => d1.documentElement.contains(a));
assert(() => d1.documentElement.contains(b));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getRootNode)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
const b = d.createElement('b');
assert(() => a === a.getRootNode());
assert(() => b === b.getRootNode());

a.appendChild(b);
assert(() => a === a.getRootNode());
assert(() => a === b.getRootNode());

d.documentElement.appendChild(a);
assert(() => d === a.getRootNode());
assert(() => d === b.getRootNode());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(hasChildNodes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
const b = d.createElement('b');
assert(() => !a.hasChildNodes());
assert(() => !b.hasChildNodes());

a.setAttribute('foo', 'bar');
assert(() => !a.hasChildNodes());

a.appendChild(b);
assert(() => a.hasChildNodes());

b.appendChild(d.createTextNode('test'));
assert(() => b.hasChildNodes());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(isEqualNode)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
assert(() => !a.isEqualNode(d.createElement('b')));

const b = d.createElement('a');
assert(() => a.isEqualNode(b));
assert(() => b.isEqualNode(a));

a.setAttribute('href', 'https://apple.com');
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

a.setAttribute('target', '_blank');
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

b.setAttribute('target', '_blank');
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

b.setAttribute('href', 'https://apple.com');
assert(() => a.isEqualNode(b));
assert(() => b.isEqualNode(a));

a.appendChild(d.createElement('b'));
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

b.appendChild(d.createElement('b'));
assert(() => a.isEqualNode(b));
assert(() => b.isEqualNode(a));

a.firstChild.appendChild(d.createTextNode('Apple'));
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

b.firstChild.appendChild(d.createTextNode('Apple Inc'));
assert(() => !a.isEqualNode(b));
assert(() => !b.isEqualNode(a));

b.firstChild.firstChild.nodeValue = 'Apple';
assert(() => a.isEqualNode(b));
assert(() => b.isEqualNode(a));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertBefore)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.createElement('a');
d.documentElement.insertBefore(a, null);

const b = d.createElement('b');

d.documentElement.insertBefore(b, a);
assert(() => b == d.documentElement.firstChild);
assert(() => a == b.nextSibling);
assert(() => a == b.nextElementSibling);

d.documentElement.insertBefore(a, b);
assert(() => a == d.documentElement.firstChild);
assert(() => b == a.nextElementSibling);

assert(() => '<root><a/><b/></root>' == d.documentElement.toString().replaceAll('\n', '').replaceAll(' ', '').trim());
assert(() => throws(() => a.insertBefore(b, d.documentElement), 'NotFoundError'));
assert(() => throws(() => d.documentElement.insertBefore(a, d.documentElement), 'NotFoundError'));
assert(() => throws(() => d.documentElement.insertBefore(d.documentElement, a), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.insertBefore(a, d.createElement('x')), 'NotFoundError'));

const e = Document.xml('<root/>');
assert(() => throws(() => e.documentElement.insertBefore(a, null), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(normalize)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const t1 = d.documentElement.appendChild(d.createTextNode('foo'));
const t2 = d.documentElement.appendChild(d.createTextNode('bar'));

assert(() => t1 == d.documentElement.firstChild);
assert(() => t2 == d.documentElement.lastChild);

d.documentElement.normalize();
assert(() => t1.isConnected);
assert(() => !t2.isConnected);
assert(() => 'foobar' == t1.nodeValue);
assert(() => 'bar' == t2.nodeValue);
assert(() => t1 == d.documentElement.firstChild);
assert(() => t1 == d.documentElement.lastChild);

const t3 = d.documentElement.appendChild(d.createTextNode('baz'));

const p = d.documentElement.appendChild(d.createElement('p'));
const t4 = p.appendChild(d.createTextNode('foo'));
const t5 = p.appendChild(d.createTextNode('bar'));
assert(() => t4 == p.firstChild);
assert(() => t5 == p.lastChild);

d.documentElement.normalize();
assert(() => t1.isConnected);
assert(() => !t3.isConnected);
assert(() => 'foobarbaz' == t1.nodeValue);
assert(() => 'baz' == t3.nodeValue);
assert(() => t1 == d.documentElement.firstChild);
assert(() => p == t1.nextSibling);

assert(() => t4.isConnected);
assert(() => !t5.isConnected);
assert(() => 'foobar' == t4.nodeValue);
assert(() => 'bar' == t5.nodeValue);
assert(() => t4 == p.firstChild);
assert(() => t4 == p.lastChild);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(removeChild)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
assert(() => throws(() => d.documentElement.removeChild(null), 'No matching function overload found'));
assert(() => throws(() => d.documentElement.removeChild(d.documentElement), 'NotFoundError'));

const a = d.documentElement.appendChild(d.createElement('a'));
const b = d.documentElement.appendChild(d.createElement('b'));

assert(() => a === d.documentElement.removeChild(a));
assert(() => b === d.documentElement.removeChild(b));
assert(() => !a.isConnected);
assert(() => !b.isConnected);

assert(() => throws(() => d.documentElement.removeChild(a), 'NotFoundError'));
assert(() => throws(() => d.documentElement.removeChild(Document.xml('<a/>').documentElement), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChild)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const a = d.documentElement.appendChild(d.createElement('a'));
assert(() => throws(() => d.documentElement.replaceChild(d.documentElement, a), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.replaceChild(Document.xml('<a/>').documentElement, a), 'WrongDocumentError'));

assert(() => a === d.documentElement.replaceChild(d.createElement('b'), a));
assert(() => !a.isConnected);

assert(() => throws(() => d.documentElement.replaceChild(d.createElement('h1'), a), 'NotFoundError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(textContent)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
assert(() => null === d.textContent);

const p1 = d.documentElement.appendChild(d.createElement('p'));
p1.textContent = 'first paragraph';

const p2 = d.documentElement.appendChild(d.createElement('p'));
p2.textContent = 'second paragraph';

assert(() => 'first paragraph' === p1.innerText);
assert(() => 'first paragraph' === p1.textContent);
assert(() => 'second paragraph' === p2.innerText);
assert(() => 'second paragraph' === p2.textContent);
assert(() => 'first paragraphsecond paragraph' === d.documentElement.innerText);
assert(() => 'first paragraphsecond paragraph' === d.documentElement.textContent);

p1.firstChild.textContent = 'changed';
assert(() => 'changed' === p1.textContent);
assert(() => 'changedsecond paragraph' === d.documentElement.textContent);

p1.innerText = 'again';
assert(() => 'again' === p1.innerText);
assert(() => 'again' === p1.textContent);
assert(() => 'againsecond paragraph' === d.documentElement.innerText);
assert(() => 'againsecond paragraph' === d.documentElement.textContent);

p2.firstChild.textContent = null;
assert(() => '' === p2.textContent);
assert(() => 'again' === d.documentElement.textContent);

p1.innerText = null;
assert(() => '' === p1.textContent);
assert(() => '' === d.documentElement.textContent);

d.documentElement.textContent = 'documentElement content';
assert(() => !p1.isConnected);
assert(() => !p2.isConnected);

assert(() => 'documentElement content' === d.documentElement.textContent);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(elementChildren)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const c = d.documentElement.children;
assert(() => !(c instanceof window.NodeList));
assert(() => c instanceof window.HTMLCollection);

assert(() => c === d.documentElement.children);
assert(() => null === d.documentElement.firstElementChild);
assert(() => null === d.documentElement.lastElementChild);
assert(() => 0 === d.documentElement.childElementCount);
assert(() => 0 === c.length);

d.documentElement.appendChild(d.createTextNode('text'));
assert(() => null === d.documentElement.firstElementChild);
assert(() => null === d.documentElement.lastElementChild);
assert(() => 0 === d.documentElement.childElementCount);
assert(() => 0 === c.length);

const a = d.documentElement.appendChild(d.createElement('a'));
assert(() => a === d.documentElement.firstElementChild);
assert(() => a === d.documentElement.lastElementChild);
assert(() => 1 == d.documentElement.childElementCount);
assert(() => 1 === c.length);
assert(() => a === c.item(0));
assert(() => a === c[0]);

d.documentElement.appendChild(d.createTextNode('text'));
assert(() => a === d.documentElement.firstElementChild);
assert(() => a === d.documentElement.lastElementChild);
assert(() => 1 == d.documentElement.childElementCount);
assert(() => 1 === c.length);
assert(() => a === c.item(0));
assert(() => a === c[0]);

const b = d.documentElement.appendChild(d.createElement('b'));
assert(() => a === d.documentElement.firstElementChild);
assert(() => b === d.documentElement.lastElementChild);
assert(() => null === a.previousElementSibling);
assert(() => a === b.previousElementSibling);
assert(() => b === a.nextElementSibling);
assert(() => null === b.nextElementSibling);
assert(() => 2 == d.documentElement.childElementCount);
assert(() => 2 === c.length);
assert(() => a === c.item(0));
assert(() => a === c[0]);
assert(() => b === c.item(1));
assert(() => b === c[1]);

let n = 0;
for(const e of c) {
    if(0 == n) assert(() => a === e);
    if(1 == n) assert(() => b === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.entries()) {
    assert(() => n == e[0]);
    if(0 == n) assert(() => a === e[1]);
    if(1 == n) assert(() => b === e[1]);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.keys()) {
    assert(() => n === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
c.forEach((e, i, s) => {
    assert(() => s == c);
    assert(() => i == n);
    if(0 == n) assert(() => a === e);
    if(1 == n) assert(() => b === e);
    ++n;
});

const v = new Array();
c.forEach(function(e, i, s) {
    assert(() => s == c);
    assert(() => i == this.length);
    if(0 == this.length) assert(() => a === e);
    if(1 == this.length) assert(() => b === e);
    this.push(e);
}, v);

assert(() => 2 == v.length);
assert(() => a == v[0]);
assert(() => b == v[1]);

assert(() => throws(() => c.forEach(() => { throw 1 })));
assert(() => throws(() => c.forEach(() => { throw 1 }, 0)));

b.remove();
assert(() => 1 == d.documentElement.childElementCount);
assert(() => 1 === c.length);
assert(() => null === c.item(1));
assert(() => undefined === c[1]);
b.remove();
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(after)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const e = d.documentElement.appendChild(d.createElement('div'));
e.after(d.createElement('p'), 'foo', d.createElement('a'));

let n = e.nextSibling;
assert(() => 'p' == n.nodeName);
n = n.nextSibling;
assert(() => 'foo' == n.nodeValue);
n = n.nextSibling;
assert(() => 'a' == n.nodeName);

e.nextElementSibling.after(d.createElement('p'), 'bar', d.createElement('b'));

n = e.nextSibling;
assert(() => 'p' == n.nodeName);
n = n.nextSibling;
assert(() => 'p' == n.nodeName);
n = n.nextSibling;
assert(() => 'bar' == n.nodeValue);
n = n.nextSibling;
assert(() => 'b' == n.nodeName);
n = n.nextSibling;
assert(() => 'foo' == n.nodeValue);
n = n.nextSibling;
assert(() => 'a' == n.nodeName);

assert(() => throws(() => e.after(d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.after(''), 'HierarchyRequestError'));
assert(() => throws(() => e.after(Document.xml('<root/>').documentElement), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(append)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
d.documentElement.append(d.createElement('p'), 'foo', d.createElement('a'));

assert(() => 'p' == d.documentElement.childNodes[0].nodeName);
assert(() => 'foo' == d.documentElement.childNodes[1].nodeValue);
assert(() => 'a' == d.documentElement.childNodes[2].nodeName);

assert(() => throws(() => d.documentElement.append(d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.append(Document.xml('<root/>').documentElement), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(before)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');

const e = d.documentElement.appendChild(d.createElement('div'));
e.before(d.createElement('p'), 'foo', d.createElement('a'));

assert(() => 'p' == d.documentElement.childNodes[0].nodeName);
assert(() => 'foo' == d.documentElement.childNodes[1].nodeValue);
assert(() => 'a' == d.documentElement.childNodes[2].nodeName);

assert(() => throws(() => e.before(d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.before(''), 'HierarchyRequestError'));
assert(() => throws(() => e.before(Document.xml('<root/>').documentElement), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertAdjacentElement)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const div = d.documentElement.appendChild(d.createElement('div'));

const a = div.insertAdjacentElement('beforebegin', d.createElement('a'));
const b = div.insertAdjacentElement('afterbegin', d.createElement('b'));
const p = div.insertAdjacentElement('beforeend', d.createElement('p'));
const u = div.insertAdjacentElement('afterend', d.createElement('u'));

assert(() => 'a' === a.nodeName);
assert(() => 'b' === b.nodeName);
assert(() => 'p' === p.nodeName);
assert(() => 'u' === u.nodeName);

assert(() => div === a.nextSibling);
assert(() => b === div.firstChild);
assert(() => p === div.lastChild);
assert(() => u === div.nextSibling);

const h1 = div.insertAdjacentElement('beforebegin', d.createElement('h1'));
const li = div.insertAdjacentElement('afterbegin', d.createElement('li'));
const ul = div.insertAdjacentElement('beforeend', d.createElement('ul'));
const h2 = div.insertAdjacentElement('afterend', d.createElement('h2'));

assert(() => 'h1' === h1.nodeName);
assert(() => 'li' === li.nodeName);
assert(() => 'ul' === ul.nodeName);
assert(() => 'h2' === h2.nodeName);

assert(() => a === h1.previousSibling);
assert(() => h1 === div.previousSibling);
assert(() => li === div.firstChild);
assert(() => b === li.nextSibling);
assert(() => ul === div.lastChild);
assert(() => h2 === div.nextSibling);
assert(() => u === h2.nextSibling);
assert(() => p === ul.previousSibling);

assert(() => throws(() => div.insertAdjacentElement('a', a), 'Wrong position argument [a]'));
assert(() => throws(() => div.insertAdjacentElement('beforebegin', Document.xml('<root/>').documentElement), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('afterbegin', Document.xml('<root/>').documentElement), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('beforeend', Document.xml('<root/>').documentElement), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('afterend', Document.xml('<root/>').documentElement), 'WrongDocumentError'));

assert(() => throws(() => b.insertAdjacentElement('afterbegin', div), 'HierarchyRequestError'));
assert(() => throws(() => b.insertAdjacentElement('beforeend', div), 'HierarchyRequestError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertAdjacentText)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const div = d.documentElement.appendChild(d.createElement('div'));

const a = div.insertAdjacentText('beforebegin', 'a');
const b = div.insertAdjacentText('afterbegin', 'b');
const p = div.insertAdjacentText('beforeend', 'p');
const u = div.insertAdjacentText('afterend', 'u');

assert(() => 'a' === a.nodeValue);
assert(() => 'b' === b.nodeValue);
assert(() => 'p' === p.nodeValue);
assert(() => 'u' === u.nodeValue);

assert(() => div === a.nextSibling);
assert(() => b === div.firstChild);
assert(() => p === div.lastChild);
assert(() => u === div.nextSibling);

const h1 = div.insertAdjacentText('beforebegin', 'h1');
const li = div.insertAdjacentText('afterbegin', 'li');
const ul = div.insertAdjacentText('beforeend', 'ul');
const h2 = div.insertAdjacentText('afterend', 'h2');

assert(() => 'h1' === h1.nodeValue);
assert(() => 'li' === li.nodeValue);
assert(() => 'ul' === ul.nodeValue);
assert(() => 'h2' === h2.nodeValue);

assert(() => a === h1.previousSibling);
assert(() => h1 === div.previousSibling);
assert(() => li === div.firstChild);
assert(() => b === li.nextSibling);
assert(() => ul === div.lastChild);
assert(() => h2 === div.nextSibling);
assert(() => u === h2.nextSibling);
assert(() => p === ul.previousSibling);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(prepend)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root/>');
const div = d.documentElement.appendChild(d.createElement('div'));

const a = d.createElement('a');
const b = d.createElement('b');
div.prepend(a, 'foo', b);

assert(() => a === div.firstChild);
assert(() => 'foo' === div.firstChild.nextSibling.nodeValue);
assert(() => b === div.firstChild.nextSibling.nextSibling);
assert(() => null === div.firstChild.nextSibling.nextSibling.nextSibling);

const i = d.createElement('i');
const u = d.createElement('u');
div.prepend(i, 'bar', u);

assert(() => i === div.firstChild);
assert(() => 'bar' === div.firstChild.nextSibling.nodeValue);
assert(() => u === div.firstChild.nextSibling.nextSibling);
assert(() => a === div.firstChild.nextSibling.nextSibling.nextSibling);

assert(() => throws(() => div.prepend(Document.xml('<root/>').documentElement), 'WrongDocumentError'));
assert(() => throws(() => div.prepend(d.documentElement), 'HierarchyRequestError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(querySelector)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml(`
<root>
  <parent id="p1">
    <child>one</child>
    <child>two</child>
  </parent>
  <parent id="p2">
    <child>eins</child>
    <child>zwei</child>
  </parent>
</root>
`);

const a = d.querySelectorAll('parent');
assert(() => 2 === a.length);
assert(() => d.documentElement.firstElementChild === a[0]);
assert(() => d.documentElement.lastElementChild === a[1]);

const b = d.querySelectorAll('parent:first-child');
assert(() => 1 === b.length);
assert(() => d.documentElement.firstElementChild === b[0]);

const c = d.querySelectorAll('parent:last-child');
assert(() => 1 === b.length);
assert(() => d.documentElement.lastElementChild === c[0]);

const e = d.querySelectorAll('parent:first-child, parent:last-child');
assert(() => 2 === e.length);
assert(() => d.documentElement.firstElementChild === e[0]);
assert(() => d.documentElement.lastElementChild === e[1]);

const f = d.querySelector('parent[id="p1"]');
assert(() => d.documentElement.firstElementChild === f);
assert(() => d.documentElement.firstElementChild === d.querySelector('parent'));

const g = d.querySelector('parent[id="p2"]');
assert(() => d.documentElement.lastElementChild === g);

assert(() => 4 === d.querySelectorAll('root child').length);
assert(() => 2 === d.querySelectorAll('*[id="p2"] > child').length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getElementsByTagName)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml(`
<root>
  <parent>
    <child>
      <grand>one</grand>
      <grand>two</grand>
    </child>
    <child>
      <grand>drei</grand>
    </child>
  </parent>
  <parent>
    <child>eins</child>
    <child>zwei</child>
  </parent>
</root>
`);

const p = d.getElementsByTagName('parent');
assert(() => 2 === p.length);

const c = d.getElementsByTagName('child');
assert(() => 4 === c.length);
assert(() => p[0] === c[0].parentElement);
assert(() => p[0] === c[1].parentElement);
assert(() => p[1] === c[2].parentElement);
assert(() => p[1] === c[3].parentElement);
assert(() => 'eins' === c[2].textContent);
assert(() => 'zwei' === c[3].textContent);

const g = d.getElementsByTagName('grand');
assert(() => 3 === g.length);
assert(() => c[0] === g[0].parentElement);
assert(() => c[0] === g[1].parentElement);
assert(() => c[1] === g[2].parentElement);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(toggleAttribute)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml(`<root/>`);

assert(() => d.documentElement.toggleAttribute('a'));
assert(() => '' === d.documentElement.getAttribute('a'));

assert(() => !d.documentElement.toggleAttribute('a'));
assert(() => !d.documentElement.hasAttribute('a'));

assert(() => !d.documentElement.toggleAttribute('a', false));
assert(() => !d.documentElement.hasAttribute('a'));

assert(() => d.documentElement.toggleAttribute('a', true));
assert(() => '' === d.documentElement.getAttribute('a'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChildren)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root><a/>foo<b/></root>');
const c = [...d.documentElement.childNodes];

assert(() => 3 === c.length);
for(const n of c) assert(() => n.isConnected);

d.documentElement.replaceChildren(d.createElement('c'), 'bar');
for(const n of c) assert(() => !n.isConnected);

assert(() => 2 === d.documentElement.childNodes.length);
assert(() => 'c' === d.documentElement.childNodes[0].tagName);
assert(() => 'bar' === d.documentElement.childNodes[1].nodeValue);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
