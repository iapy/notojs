#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(HTML, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Memory)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const e = (function() {
    const doc = Document.html();
    return doc.createElement('div');
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

BOOST_AUTO_TEST_CASE(attach)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window } from 'noto:dom';

const d = window.attach();
assert(() => d === document);
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window } from 'noto:dom';

assert(() => undefined === globalThis['document']);
window.attach({notebook: true});
    )JS", context.get(), "cell-002");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window } from 'noto:dom';

assert(() => document instanceof window.HTMLDocument);
    )JS", context.get(), "cell-003");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Document)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

assert(() => throws(() => new Document()));
assert(() => throws(() => new window.HTMLDocument()));

const d = Document.html();

assert(() => d instanceof window.Document);
assert(() => d instanceof window.HTMLDocument);
assert(() => !(d instanceof window.XMLDocument));

assert(() => 'about:blank' === d.baseURI);
assert(() => d.isConnected);
assert(() => 10 === d.firstChild.nodeType);
assert(() => 'html' === d.firstChild.nodeName);
assert(() => d.documentElement === d.lastChild);
assert(() => 'http://www.w3.org/1999/xhtml' === d.namespaceURI);
assert(() => 'http://www.w3.org/1999/xhtml' === d.documentElement.namespaceURI);
assert(() => d.head === d.documentElement.firstChild);
assert(() => d.body === d.documentElement.lastChild);
assert(() => '#document' === d.nodeName);
assert(() => undefined === d.tagName);
assert(() => 'HTML' === d.documentElement.nodeName);
assert(() => 'HTML' === d.documentElement.tagName);
assert(() => 9 === d.nodeType);
assert(() => 1 === d.documentElement.nodeType);
assert(() => d === d.documentElement.parentNode);
assert(() => null === d.nodeValue);
assert(() => null === d.parentElement);
assert(() => null === d.parentNode);

assert(() => throws(() => d.toJSON(), "HTMLDocument cannot be serialized"));
assert(() => throws(() => d.documentElement.toJSON(), "<HTML> cannot be serialized"));

assert(() => d.isDefaultNamespace('http://www.w3.org/1999/xhtml'));
assert(() => !d.isDefaultNamespace('http://www.w3.org/1998/Math/MathML'));
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

const d = Document.html();
const a = d.body.appendChild(d.createElement('a'));

assert(() => throws(() => xml(d), "No matching function overload found"));
assert(() => throws(() => xml(a), "Invalid XML: not an XML document"));

assert(() => html(d) instanceof HTML);
assert(() => throws(() => html(d).toJSON(), "HTML cannot be serialized"));

assert(() => html(d.body) instanceof HTML);
assert(() => throws(() => html(d.body).toJSON(), "HTML cannot be serialized"));

assert(() => html(a) instanceof HTML);
assert(() => html(a).toJSON() instanceof Object);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Body)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const b1 = d.body;
const b2 = d.body;
assert(() => b1 === b2);
assert(() => b1.isSameNode(b2));
assert(() => b1 instanceof window.Element);
assert(() => b1 instanceof window.HTMLElement);
assert(() => 'about:blank' === b1.baseURI);
assert(() => b1.isConnected);
assert(() => null === b1.firstChild);
assert(() => null === b1.lastChild);
assert(() => 'http://www.w3.org/1999/xhtml' === b1.namespaceURI);
assert(() => null === b1.nextSibling);
assert(() => null === b1.nextElementSibling);
assert(() => 'BODY' === b1.nodeName);
assert(() => 'BODY' === b1.tagName);
assert(() => 1 === b1.nodeType);
assert(() => null === b1.nodeValue);
assert(() => d === b1.ownerDocument);
assert(() => d.documentElement === b1.parentNode);
assert(() => d.documentElement === b1.parentElement);
assert(() => d.documentElement.isSameNode(b1.parentNode));
assert(() => d.documentElement.isSameNode(b1.parentElement));
assert(() => d.head === b1.previousSibling);
assert(() => d.head === b1.previousElementSibling);

assert(() => throws(() => b1.toJSON(), "<BODY> cannot be serialized"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Head)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const h1 = d.head;
const h2 = d.head;
assert(() => h1 === h2);
assert(() => h1 instanceof window.Element);
assert(() => h1 instanceof window.HTMLElement);
assert(() => 'about:blank' === h1.baseURI);
assert(() => h1.isConnected);
assert(() => null === h1.firstChild);
assert(() => null === h1.lastChild);
assert(() => 'http://www.w3.org/1999/xhtml' === h1.namespaceURI);
assert(() => d.body === h1.nextSibling);
assert(() => d.body === h1.nextElementSibling);
assert(() => 'HEAD' === h1.nodeName);
assert(() => 'HEAD' === h1.tagName);
assert(() => 1 === h1.nodeType);
assert(() => null === h1.nodeValue);
assert(() => d === h1.ownerDocument);
assert(() => d.documentElement === h1.parentNode);
assert(() => d.documentElement === h1.parentElement);
assert(() => null === h1.previousSibling);
assert(() => null === h1.previousElementSibling);

assert(() => throws(() => h1.toJSON(), "<HEAD> cannot be serialized"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Title)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => null === d.title);
assert(() => '<!DOCTYPE html><html><head></head><body></body></html>' == d.toString());

d.title = 'A';
assert(() => 'A' === d.title);
assert(() => '<!DOCTYPE html><html><head><title>A</title></head><body></body></html>' == d.toString());

d.title = null;
assert(() => '' === d.title);
assert(() => '<!DOCTYPE html><html><head><title></title></head><body></body></html>' == d.toString());

assert(() => 'TITLE' == d.head.firstChild.nodeName);
assert(() => throws(() => d.head.firstChild.toJSON(), "<TITLE> cannot be serialized"));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(createElement)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const a = d.createElement('a');
assert(() => a instanceof window.Element);
assert(() => a instanceof window.HTMLElement);
assert(() => 'about:blank' === a.baseURI);
assert(() => !a.isConnected);
assert(() => null === a.firstChild);
assert(() => null === a.lastChild);
assert(() => 'http://www.w3.org/1999/xhtml' === a.namespaceURI);
assert(() => null === a.nextSibling);
assert(() => null === a.nextElementSibling);
assert(() => 'A' === a.nodeName);
assert(() => 'A' === a.tagName);
assert(() => 1 === a.nodeType);
assert(() => null == a.nodeValue);
assert(() => d === a.ownerDocument);
assert(() => null == a.parentElement);
assert(() => null == a.parentNode);
assert(() => null === a.previousSibling);
assert(() => null === a.previousElementSibling);
assert(() => '<a></a>' === a.toJSON().data);
assert(() => 'notojs.HTML' === a.toJSON().type);
print(a);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.HTML"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), R"HTML(<a></a>)HTML"));
}

BOOST_AUTO_TEST_CASE(createElementNS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

assert(() => throws(() => d.createElementNS('http://', 'a'), 'NamespaceError'));

const a = d.createElementNS('http://www.w3.org/1999/xhtml', 'a');
assert(() => 'http://www.w3.org/1999/xhtml' === a.namespaceURI);

const m = d.createElementNS('http://www.w3.org/1998/Math/MathML', 'math');
assert(() => 'http://www.w3.org/1998/Math/MathML' === m.namespaceURI);

const s = d.createElementNS('http://www.w3.org/2000/svg', 'svg');
assert(() => 'http://www.w3.org/2000/svg' === s.namespaceURI);
assert(() => s instanceof window.SVGElement);
assert(() => s instanceof window.SVGSVGElement);

const p = d.createElementNS('http://www.w3.org/2000/svg', 'path');
assert(() => 'http://www.w3.org/2000/svg' === p.namespaceURI);
assert(() => p instanceof window.SVGElement);
assert(() => !(p instanceof window.SVGSVGElement));

assert(() => 0 < s.toString().indexOf('xmlns='));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(attributes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const a = d.createElement('a');
const n = a.attributes;

assert(() => !a.hasAttribute('href'));
assert(() => null === a.getAttribute('href'));
assert(() => null === n.getNamedItem('href'));

a.setAttribute('href', 'http://apple.com');
assert(() => a.hasAttribute('href'));
assert(() => 'http://apple.com' == a.getAttribute('href'));

let href = n.getNamedItem('href');
assert(() => null !== href);
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

assert(() => null === a.id);
assert(() => !a.hasAttribute('id'));

a.id = 'test';
assert(() => a.hasAttribute('id'));
assert(() => 'test' === a.id);
assert(() => 'test' === a.getAttribute('id'));

a.id = false;
assert(() => a.hasAttribute('id'));
assert(() => 'false' === a.id);
assert(() => 'false' === a.getAttribute('id'));

let arr = [...a.attributes];

assert(() => 2 === arr.length);
for(const attr of a.attributes)
    assert(() => attr === arr.shift());
assert(() => 0 === arr.length);

a.removeAttribute('id');
a.removeAttribute('href');
assert(() => null === a.id);

arr = [...a.attributes];
assert(() => 0 === arr.length);

a.id = true;
print(a);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.HTML"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), R"HTML(<a id="true"></a>)HTML"));
}

BOOST_AUTO_TEST_CASE(attributesNS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const a = d.createElementNS('http://www.w3.org/2000/svg', 'text');

assert(() => !a.hasAttributeNS('http://www.w3.org/2000/svg', 'fill'));
assert(() => null == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

assert(() => throws(() => a.setAttributeNS('', 'href', 'http://apple.com')), 'NamespaceError');
assert(() => throws(() => a.setAttributeNS('', 'href', 1)), 'NamespaceError');
assert(() => throws(() => a.setAttributeNS('', 'href', null)), 'NamespaceError');
assert(() => throws(() => a.setAttributeNS('', 'href', true)), 'NamespaceError');
assert(() => throws(() => a.setAttributeNS('', 'href', false)), 'NamespaceError');

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', 'http://apple.com');
assert(() => !a.hasAttribute('href'));
assert(() => null == a.getAttribute('href'));

assert(() => throws(() => a.hasAttributeNS('', 'href')), 'NamespaceError');
assert(() => a.hasAttributeNS('http://www.w3.org/2000/svg', 'href'));
assert(() => 'http://apple.com' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.removeAttribute('href');
assert(() => a.hasAttributeNS('http://www.w3.org/2000/svg', 'href'));

assert(() => throws(() => removeAttributeNS('', 'href')), 'NamespaceError');
a.removeAttributeNS('http://www.w3.org/2000/svg', 'href');
assert(() => null == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', 1);
assert(() => '1' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', null);
assert(() => 'null' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', true);
assert(() => 'true' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', false);
assert(() => 'false' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(namedNodeMap)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => null === d.body.getAttributeNode('attr'));

const a = d.body.appendChild(d.createElement('div')).attributes;
const b = d.body.appendChild(d.createElement('div')).attributes;

assert(() => 0 === a.length);
assert(() => 0 === b.length);
assert(() => throws(() => a.setNamedItem('a', 'b'), 'No matching function overload found'));

d.body.children[0].setAttribute('foo', 'bar');
d.body.children[1].setAttribute('foo', 'baz');

const bar = a.getNamedItem('foo');
const baz = b.getNamedItem('foo');
assert(() => !bar.hasChildNodes());
assert(() => !baz.hasChildNodes());
assert(() => bar === d.body.children[0].getAttributeNode('foo'));
assert(() => baz === d.body.children[1].getAttributeNode('foo'));
assert(() => d.body.children[0] === bar.ownerElement);
assert(() => d.body.children[1] === baz.ownerElement);
assert(() => 'http://www.w3.org/1999/xhtml' === bar.namespaceURI);
assert(() => 'http://www.w3.org/1999/xhtml' === baz.namespaceURI);
assert(() => baz.isDefaultNamespace(baz.namespaceURI));
assert(() => baz === b.setNamedItem(bar));
assert(() => !baz.isConnected);
assert(() => 'http://www.w3.org/1999/xhtml' === baz.namespaceURI);
assert(() => null === baz.ownerElement);
assert(() => null === a.setNamedItem(baz));
assert(() => baz.isConnected);
assert(() => d.body.children[1] === bar.ownerElement);
assert(() => d.body.children[0] === baz.ownerElement);
assert(() => d === bar.getRootNode());
assert(() => d === baz.getRootNode());
assert(() => !bar.contains(baz));
assert(() => !bar.contains(d.body.children[1]));
assert(() => d.body.children[1].contains(bar));
assert(() => d.body.children[0].contains(baz));

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
assert(() => !bar.isSameNode(d.body.children[0]));
assert(() => !d.body.children[0].isSameNode(bar));
assert(() => !bar.isEqualNode(d.body.children[0]));
assert(() => !d.body.children[0].isEqualNode(bar));
assert(() => !bar.isSameNode(baz));

assert(() => bar.compareDocumentPosition(baz) === d.body.children[1].compareDocumentPosition(d.body.children[0]));
assert(() => baz.compareDocumentPosition(bar) === d.body.children[0].compareDocumentPosition(d.body.children[1]));

d.body.children[0].setAttribute('bar', 'foo');
const foo = a.getNamedItem('bar');

assert(() => 2 === foo.compareDocumentPosition(baz));
assert(() => 4 === baz.compareDocumentPosition(foo));

foo.normalize();
assert(() => 'foo' === foo.value);
assert(() => foo === d.body.children[0].removeAttributeNode(foo));
assert(() => !foo.isConnected);
assert(() => throws(() => d.body.children[0].removeAttributeNode(foo), 'NotFoundError'));
assert(() => null === d.body.children[0].setAttributeNode(foo));
assert(() => foo.isConnected);

const buz = bar.cloneNode();
assert(() => !buz.isConnected);
assert(() => buz.isEqualNode(bar));
assert(() => !buz.isSameNode(bar));
assert(() => null === buz.getRootNode());

assert(() => throws(() => buz.appendChild(d.body), 'HierarchyRequestError'));
assert(() => throws(() => buz.removeChild(d.body), 'HierarchyRequestError'));
assert(() => throws(() => buz.replaceChild(d.body, d.body), 'HierarchyRequestError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(namedNodeMapNS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => null === d.body.getAttributeNodeNS(null, 'class'));

const a = d.body.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'svg')).attributes;
const b = d.body.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'svg')).attributes;

assert(() => 0 === a.length);
assert(() => 0 === b.length);

assert(() => throws(() => a.getNamedItemNS('http://', 'a'), 'NamespaceError'));
d.body.children[0].setAttributeNS('http://www.w3.org/2000/svg', 'viewBox', '0 0 100 100');
d.body.children[1].setAttributeNS('http://www.w3.org/2000/svg', 'viewBox', '0 0 200 200');

assert(() => null === a.getNamedItem('viewBox'));
assert(() => null === b.getNamedItem('viewBox'));

const avb = a.getNamedItemNS('http://www.w3.org/2000/svg', 'viewBox');
const bvb = b.getNamedItemNS('http://www.w3.org/2000/svg', 'viewBox');
assert(() => d.body.children[0] === avb.ownerElement);
assert(() => d.body.children[1] === bvb.ownerElement);
assert(() => 'http://www.w3.org/2000/svg' === avb.namespaceURI);
assert(() => 'http://www.w3.org/2000/svg' === bvb.namespaceURI);
assert(() => avb === d.body.children[0].getAttributeNodeNS('http://www.w3.org/2000/svg', 'viewBox'));
assert(() => bvb === d.body.children[1].getAttributeNodeNS('http://www.w3.org/2000/svg', 'viewBox'));

assert(() => bvb === b.setNamedItemNS(avb));
assert(() => !bvb.isConnected);
assert(() => null === bvb.ownerElement);
assert(() => null === a.setNamedItem(bvb));
assert(() => bvb.isConnected);
assert(() => d.body.children[1] === avb.ownerElement);
assert(() => d.body.children[0] === bvb.ownerElement);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Text)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const t = d.createTextNode('Some text');
assert(() => t instanceof window.Text);
assert(() => 'about:blank' === t.baseURI);
assert(() => !t.isConnected);
assert(() => null === t.firstChild);
assert(() => null === t.lastChild);
assert(() => 'http://www.w3.org/1999/xhtml' === t.namespaceURI);
assert(() => null === t.nextSibling);
assert(() => '#text' == t.nodeName);
assert(() => 3 == t.nodeType);
assert(() => 'Some text' == t.nodeValue);
assert(() => d === t.ownerDocument);
assert(() => null === t.parentElement);
assert(() => null == t.parentNode);
assert(() => null === t.previousSibling);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(childNodes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const c = d.body.childNodes;
assert(() => c === d.body.childNodes);
assert(() => c instanceof window.NodeList);

assert(() => 0 === c.length);
assert(() => null === c.item(0));
assert(() => undefined === c[0]);
assert(() => undefined === c['a']);

const a = d.body.appendChild(d.createElement('a'));
assert(() => 1 === c.length);
assert(() => 'A' == c[0].nodeName);
assert(() => 'A' == c[0].tagName);
assert(() => d.body.firstChild === c[0]);
assert(() => d.body.firstChild === c.item(0));
assert(() => null === c.item(1));
assert(() => undefined === c[1]);

const b = d.body.appendChild(d.createElement('b'));
assert(() => 2 === c.length);
assert(() => 'B' == c[1].nodeName);
assert(() => 'B' == c[1].tagName);
assert(() => d.body.firstChild === c[0]);
assert(() => d.body.lastChild === c[1]);
assert(() => d.body.firstChild === c.item(0));
assert(() => d.body.lastChild === c.item(1));
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

d.body.removeChild(d.body.firstChild);
assert(() => 1 === c.length);
assert(() => 'B' == c[0].nodeName);
assert(() => 'B' == c[0].tagName);
assert(() => d.body.firstChild === c[0]);
assert(() => d.body.firstChild === c.item(0));
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
    d.body.appendChild(d.createElement('h' + i));

assert(() => 101 == c.length);

while(c.length) d.body.removeChild(c[0]);
assert(() => 0 == c.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(clone)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const a = d.createElement('a');
a.setAttribute('href', 'https://apple.co.uk');
a.appendChild(d.createElement('b')).appendChild(d.createTextNode('Apple'));
d.body.appendChild(a);

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
assert(() => 'B' == e.firstChild.nodeName);
assert(() => 'B' == e.firstChild.tagName);
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

const d1 = Document.html();
const d2 = Document.html();

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

const d1 = Document.html();
const d2 = Document.html();

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

assert(() => !d1.body.contains(a));
assert(() => !d1.body.contains(b));
assert(() => !d1.body.contains(c));
assert(() => !d1.body.contains(d));

a.appendChild(b);
assert(() => a.contains(b));

d1.body.appendChild(a);
assert(() => a.contains(b));
assert(() => d1.contains(a));
assert(() => d1.contains(b));
assert(() => d1.body.contains(a));
assert(() => d1.body.contains(b));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getRootNode)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const a = d.createElement('a');
const b = d.createElement('b');
assert(() => a === a.getRootNode());
assert(() => b === b.getRootNode());

a.appendChild(b);
assert(() => a === a.getRootNode());
assert(() => a === b.getRootNode());

d.body.appendChild(a);
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

const d = Document.html();

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

const d = Document.html();

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

const d = Document.html();

const a = d.createElement('a');
d.body.insertBefore(a, null);

const b = d.createElement('b');

d.body.insertBefore(b, a);
assert(() => b == d.body.firstChild);
assert(() => a == b.nextSibling);
assert(() => a == b.nextElementSibling);

d.body.insertBefore(a, b);
assert(() => a == d.body.firstChild);
assert(() => b == a.nextSibling);
assert(() => b == a.nextElementSibling);

assert(() => '<body><a></a><b></b></body>' == d.body.toString().trim());
assert(() => '<a></a><b></b>' == d.body.getHTML().trim());
assert(() => throws(() => a.insertBefore(b, d.body), 'NotFoundError'));
assert(() => throws(() => d.body.insertBefore(a, d.body), 'NotFoundError'));
assert(() => throws(() => d.body.insertBefore(d.body, a), 'HierarchyRequestError'));
assert(() => throws(() => d.body.insertBefore(a, d.createElement('x')), 'NotFoundError'));

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

const d = Document.html();
const t1 = d.body.appendChild(d.createTextNode('foo'));
const t2 = d.body.appendChild(d.createTextNode('bar'));

assert(() => t1 == d.body.firstChild);
assert(() => t2 == d.body.lastChild);

d.body.normalize();
assert(() => t1.isConnected);
assert(() => !t2.isConnected);
assert(() => 'foobar' == t1.nodeValue);
assert(() => 'bar' == t2.nodeValue);
assert(() => t1 == d.body.firstChild);
assert(() => t1 == d.body.lastChild);

const t3 = d.body.appendChild(d.createTextNode('baz'));

const p = d.body.appendChild(d.createElement('p'));
const t4 = p.appendChild(d.createTextNode('foo'));
const t5 = p.appendChild(d.createTextNode('bar'));
assert(() => t4 == p.firstChild);
assert(() => t5 == p.lastChild);

d.body.normalize();
assert(() => t1.isConnected);
assert(() => !t3.isConnected);
assert(() => 'foobarbaz' == t1.nodeValue);
assert(() => 'baz' == t3.nodeValue);
assert(() => t1 == d.body.firstChild);
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

const d = Document.html();
assert(() => throws(() => d.body.removeChild(null), 'No matching function overload found'));
assert(() => throws(() => d.body.removeChild(d.body), 'NotFoundError'));

const a = d.body.appendChild(d.createElement('a'));
const b = d.body.appendChild(d.createElement('b'));

assert(() => a === d.body.removeChild(a));
assert(() => b === d.body.removeChild(b));
assert(() => !a.isConnected);
assert(() => !b.isConnected);

assert(() => throws(() => d.body.removeChild(a), 'NotFoundError'));
assert(() => throws(() => d.body.removeChild(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChild)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const a = d.body.appendChild(d.createElement('a'));
assert(() => throws(() => d.body.replaceChild(d.body, a), 'HierarchyRequestError'));
assert(() => throws(() => d.body.replaceChild(Document.html().body, a), 'WrongDocumentError'));

assert(() => a === d.body.replaceChild(d.createElement('b'), a));
assert(() => !a.isConnected);

assert(() => throws(() => d.body.replaceChild(d.createElement('h1'), a), 'NotFoundError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(textContent)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => null === d.textContent);

const p1 = d.body.appendChild(d.createElement('p'));
p1.textContent = 'first paragraph';

const p2 = d.body.appendChild(d.createElement('p'));
p2.textContent = 'second paragraph';

assert(() => 'first paragraph' === p1.textContent);
assert(() => 'second paragraph' === p2.textContent);
assert(() => 'first paragraphsecond paragraph' === d.body.textContent);

p1.firstChild.textContent = 'changed';
assert(() => 'changed' === p1.textContent);
assert(() => 'changedsecond paragraph' === d.body.textContent);

p2.firstChild.textContent = null;
assert(() => '' === p2.textContent);
assert(() => 'changed' === d.body.textContent);

d.body.textContent = 'body content';
assert(() => !p1.isConnected);
assert(() => !p2.isConnected);

assert(() => 'body content' === d.body.textContent);
    )JS");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => null === d.textContent);
assert(() => '' === d.documentElement.textContent);

const p1 = d.body.appendChild(d.createElement('p'));
const p2 = d.body.appendChild(d.createElement('p'));

d.documentElement.textContent = null;
assert(() => '' === d.documentElement.textContent);
assert(() => !d.body.isConnected);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(elementChildren)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const c = d.body.children;
assert(() => !(c instanceof window.NodeList));
assert(() => c instanceof window.HTMLCollection);

assert(() => c === d.body.children);
assert(() => null === d.body.firstElementChild);
assert(() => null === d.body.lastElementChild);
assert(() => 0 === d.body.childElementCount);
assert(() => 0 === c.length);

d.body.appendChild(d.createTextNode('text'));
assert(() => null === d.body.firstElementChild);
assert(() => null === d.body.lastElementChild);
assert(() => 0 === d.body.childElementCount);
assert(() => 0 === c.length);

const a = d.body.appendChild(d.createElement('a'));
assert(() => a === d.body.firstElementChild);
assert(() => a === d.body.lastElementChild);
assert(() => 1 === d.body.childElementCount);
assert(() => 1 === c.length);
assert(() => a === c.item(0));
assert(() => a === c[0]);

d.body.appendChild(d.createTextNode('text'));
assert(() => a === d.body.firstElementChild);
assert(() => a === d.body.lastElementChild);
assert(() => 1 === d.body.childElementCount);
assert(() => 1 === c.length);
assert(() => a === c.item(0));
assert(() => a === c[0]);

const b = d.body.appendChild(d.createElement('b'));
assert(() => a === d.body.firstElementChild);
assert(() => b === d.body.lastElementChild);
assert(() => null === a.previousElementSibling);
assert(() => a === b.previousElementSibling);
assert(() => b === a.nextElementSibling);
assert(() => null === b.nextElementSibling);
assert(() => 2 === d.body.childElementCount);
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

const x = Array.from(c);
assert(() => 2 == x.length);
assert(() => a == x[0]);
assert(() => b == x[1]);


b.remove();
assert(() => 1 === d.body.childElementCount);
assert(() => 1 === c.length);
assert(() => null === c.item(1));
assert(() => undefined === c[1]);
b.remove();

assert(() => null === c.namedItem('foo'));
a.id = 'foo';

assert(() => a === c.namedItem('foo'));
a.id = null;

assert(() => null === c.namedItem('foo'));

a.setAttribute('name', 'bar');
assert(() => a === c.namedItem('bar'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(innerHTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => '' == d.body.innerHTML);

d.body.appendChild(d.createElement('a'));
assert(() => '<a></a>' === d.body.innerHTML);

d.body.firstElementChild.appendChild(d.createElement('b'));
assert(() => '<a><b></b></a>' === d.body.innerHTML);

d.body.firstElementChild.firstElementChild.appendChild(d.createTextNode('bold'));
assert(() => '<a><b>bold</b></a>' === d.body.innerHTML);

d.body.appendChild(d.createTextNode('after'));
assert(() => '<a><b>bold</b></a>after' === d.body.innerHTML);

let a = d.body.firstChild;
assert(() => a.isConnected);

d.body.innerHTML = '<a>link</a>';
assert(() => '<a>link</a>' === d.body.innerHTML);
assert(() => 'A' === d.body.firstChild.nodeName);
assert(() => 'A' === d.body.lastChild.nodeName);
assert(() => !a.isConnected);

a = d.body.firstChild;
assert(() => a.isConnected);

d.body.innerHTML = null;
assert(() => 'null' === d.body.innerHTML);
assert(() => '#text' === d.body.firstChild.nodeName);
assert(() => !a.isConnected);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(outerHTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => '<body></body>' === d.body.outerHTML);

const div = d.createElement('div');
div.outerHTML = 'foo';

d.body.appendChild(div);
assert(() => '<body><div></div></body>' === d.body.outerHTML);

div.outerHTML = '<b>bold</b>';
assert(() => '<body><b>bold</b></body>' === d.body.outerHTML);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(innerText)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

d.body.innerText = 'a\nb';
assert(() => 'a' === d.body.childNodes[0].nodeValue);
assert(() => 'BR' === d.body.childNodes[1].nodeName);
assert(() => 'b' === d.body.childNodes[2].nodeValue);

d.body.innerText = '\n\na\n\nb\n\n';
assert(() => 'BR' === d.body.childNodes[0].nodeName);
assert(() => 'a' === d.body.childNodes[1].nodeValue);
assert(() => 'BR' === d.body.childNodes[2].nodeName);
assert(() => 'b' === d.body.childNodes[3].nodeValue);
assert(() => 'BR' === d.body.childNodes[4].nodeName);

assert(() => '\na\nb\n' == d.body.innerText);
assert(() => '\n' == d.body.firstElementChild.innerText);

d.body.innerHTML = '<p>foo</p><p>bar</p>';
assert(() => 'foo\nbar' == d.body.innerText);
assert(() => 'foo' == d.body.firstChild.innerText);
assert(() => 'bar' == d.body.lastChild.innerText);

d.body.innerHTML = '<ul><li>one</li><li>two</li></ul>';
assert(() => 'one\ntwo' == d.body.innerText);
assert(() => 'one' == d.body.firstChild.firstChild.innerText);
assert(() => 'two' == d.body.firstChild.lastChild.innerText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(outerText)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

d.body.innerHTML = '<div></div>';
d.body.firstChild.outerText = 'a\nb';
assert(() => 'a' === d.body.childNodes[0].nodeValue);
assert(() => 'BR' === d.body.childNodes[1].nodeName);
assert(() => 'b' === d.body.childNodes[2].nodeValue);
assert(() => 'a\nb' == d.body.innerText);
assert(() => d.body.innerText === d.body.outerText);

d.body.innerHTML = '<div></div>';
d.body.firstChild.outerText = '\n\na\n\nb\n\n';
assert(() => 'BR' === d.body.childNodes[0].nodeName);
assert(() => 'a' === d.body.childNodes[1].nodeValue);
assert(() => 'BR' === d.body.childNodes[2].nodeName);
assert(() => 'b' === d.body.childNodes[3].nodeValue);
assert(() => 'BR' === d.body.childNodes[4].nodeName);
assert(() => '\na\nb\n' == d.body.innerText);
assert(() => d.body.innerText === d.body.outerText);

d.body.innerHTML = '<div></div>';
d.body.firstChild.outerText = null;
assert(() => 1 === d.body.childNodes.length);
assert(() => 'null' === d.body.childNodes[0].nodeValue);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(after)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const e = d.body.appendChild(d.createElement('div'));
e.after(d.createElement('p'), 'foo', d.createElement('a'));

let n = e.nextSibling;
assert(() => 'P' == n.nodeName);
n = n.nextSibling;
assert(() => 'foo' == n.nodeValue);
n = n.nextSibling;
assert(() => 'A' == n.nodeName);

e.nextElementSibling.after(d.createElement('p'), 'bar', d.createElement('b'));

n = e.nextSibling;
assert(() => 'P' == n.nodeName);
n = n.nextSibling;
assert(() => 'P' == n.nodeName);
n = n.nextSibling;
assert(() => 'bar' == n.nodeValue);
n = n.nextSibling;
assert(() => 'B' == n.nodeName);
n = n.nextSibling;
assert(() => 'foo' == n.nodeValue);
n = n.nextSibling;
assert(() => 'A' == n.nodeName);

assert(() => throws(() => e.after(d.body), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.after(''), 'HierarchyRequestError'));
assert(() => throws(() => e.after(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(append)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
d.body.append(d.createElement('p'), 'foo', d.createElement('a'));

assert(() => 'P' == d.body.childNodes[0].nodeName);
assert(() => 'foo' == d.body.childNodes[1].nodeValue);
assert(() => 'A' == d.body.childNodes[2].nodeName);

assert(() => throws(() => d.body.append(d.body), 'HierarchyRequestError'));
assert(() => throws(() => d.body.append(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(before)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const e = d.body.appendChild(d.createElement('div'));
e.before(d.createElement('p'), 'foo', d.createElement('a'));

assert(() => 'P' == d.body.childNodes[0].nodeName);
assert(() => 'foo' == d.body.childNodes[1].nodeValue);
assert(() => 'A' == d.body.childNodes[2].nodeName);

assert(() => throws(() => e.before(d.body), 'HierarchyRequestError'));
assert(() => throws(() => d.documentElement.before(''), 'HierarchyRequestError'));
assert(() => throws(() => e.before(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(closest)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
assert(() => d.documentElement === d.body.closest('html'));

const div = d.body.appendChild(d.createElement('div'));
assert(() => d.documentElement === div.closest('html'));

div.id = 'div';
div.className = 'foo';

d.body.id = 'body';
d.body.className = 'bar';
assert(() => div === div.closest('div'));
assert(() => div === div.closest('#div'));
assert(() => div === div.closest('.foo'));
assert(() => div === div.closest('div.foo'));
assert(() => d.body === div.closest('body'));
assert(() => d.body === div.closest('#body'));
assert(() => d.body === div.closest('.bar'));
assert(() => d.body === div.closest('body.bar'));

const p = div.appendChild(d.createElement('p'));
assert(() => p === p.closest('p'));
assert(() => div === p.closest('div'));
assert(() => div === p.closest('#div'));
assert(() => div === p.closest('.foo'));
assert(() => div === p.closest('div.foo'));
assert(() => d.body === p.closest('body'));
assert(() => d.body === p.closest('#body'));
assert(() => d.body === p.closest('.bar'));
assert(() => d.body === p.closest('body.bar'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(matches)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const e = d.body.appendChild(d.createElement('div'));
e.className = 'foo';

assert(() => e.matches('div'));
assert(() => e.matches('div.foo'));
assert(() => e.matches('body div'));
assert(() => e.matches('body div.foo'));
assert(() => e.matches('body > div'));
assert(() => e.matches('body > div.foo'));

const p = e.appendChild(d.createElement('p'));
p.id = 'bar';
p.className = 'bar';

assert(() => p.matches('p'));
assert(() => p.matches('#bar'));
assert(() => p.matches('p.bar'));
assert(() => p.matches('body p'));
assert(() => p.matches('body p.bar'));
assert(() => p.matches('body > div > p'));
assert(() => p.matches('body > div > p.bar'));
assert(() => p.matches('body > div.foo > p'));
assert(() => p.matches('body > div.foo > p.bar'));
assert(() => !p.matches('body > p'));
assert(() => !p.matches('body > p.bar'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(classList)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const c = d.body.classList;

assert(() => c instanceof window.DOMTokenList);
assert(() => c === d.body.classList);
assert(() => 0 === c.length);
assert(() => '' === c.value);
assert(() => undefined === c[0]);
assert(() => null === c.item(0));
assert(() => null === c.item(-1));

d.body.setAttribute('class', '    abc ');
assert(() => 1 === c.length);
assert(() => 'abc' == c[0]);
assert(() => 'abc' == c.item(0));
assert(() => '    abc ' === c.value);
assert(() => '    abc ' === c.toString());
assert(() => !c.contains('ab'));
assert(() => c.contains('abc'));

d.body.setAttribute('class', ' a bc cde ');
assert(() => 3 === c.length);
assert(() => ' a bc cde ' === c.value);
assert(() => ' a bc cde ' === c.toString());
assert(() => ' a bc cde ' === d.body.getAttribute('class'));
assert(() => 'a' === c[0]);
assert(() => 'a' === c.item(0));
assert(() => 'bc' === c[1]);
assert(() => 'bc' === c.item(1));
assert(() => 'cde' === c[2]);
assert(() => 'cde' === c.item(2));
assert(() => undefined === c[3]);
assert(() => null === c.item(3));
assert(() => !c.contains('a bc'));
assert(() => c.contains('a'));
assert(() => !c.contains('b'));
assert(() => c.contains('bc'));
assert(() => !c.contains('bcd'));
assert(() => c.contains('cde'));
assert(() => !c.contains('c'));

c.value = 'a bc cde defg 1';
assert(() => 5 === c.length);
assert(() => 'a bc cde defg 1' === c.value);
assert(() => 'a bc cde defg 1' === c.toString());
assert(() => 'a bc cde defg 1' === d.body.getAttribute('class'));
assert(() => 'a' === c.item(0));
assert(() => 'bc' === c.item(1));
assert(() => 'cde' === c.item(2));
assert(() => 'defg' === c.item(3));
assert(() => '1' === c.item(4));
assert(() => null === c.item(5));
assert(() => !c.contains('a bc cde'));
assert(() => c.contains('a'));
assert(() => !c.contains('b'));
assert(() => c.contains('bc'));
assert(() => !c.contains('bcd'));
assert(() => c.contains('cde'));
assert(() => !c.contains('cd'));
assert(() => c.contains('defg'));
assert(() => !c.contains('ddd'));
assert(() => c.contains('1'));

assert(() => throws(() => c.add(''), 'SyntaxError'));
assert(() => throws(() => c.add('a b'), 'InvalidCharacterError'));

c.value = 'c1 c2';
c.add('c1', 'c2');
assert(() => 2 === c.length);
assert(() => c.contains('c1'));
assert(() => c.contains('c2'));

c.add(3, 4);
assert(() => 4 === c.length);
assert(() => c.contains('c1'));
assert(() => c.contains('c2'));
assert(() => c.contains(3));
assert(() => c.contains(4));
assert(() => 'c1' == c.item(0));
assert(() => 'c2' == c.item(1));
assert(() => '3' == c.item(2));
assert(() => '4' == c.item(3));
assert(() => null === c.item(4));
assert(() => 'c1 c2 3 4' === c.value);
assert(() => 'c1 c2 3 4' === c.toString());
assert(() => 'c1 c2 3 4' === d.body.className);

c.remove('c1', 3);
assert(() => 2 === c.length);
assert(() => !c.contains('c1'));
assert(() => c.contains('c2'));
assert(() => !c.contains(3));
assert(() => c.contains(4));
assert(() => 'c2' == c.item(0));
assert(() => '4' == c.item(1));
assert(() => null === c.item(4));
assert(() => 'c2 4' === c.value);
assert(() => 'c2 4' === c.toString());
assert(() => 'c2 4' === d.body.className);

assert(() => throws(() => c.remove(''), 'SyntaxError'));
assert(() => throws(() => c.remove('a b'), 'InvalidCharacterError'));

assert(() => !c.replace(1, 2));
assert(() => c.replace('c2', 2));
assert(() => '2 4' === c.value);
assert(() => '2 4' === c.toString());
assert(() => '2 4' === d.body.className);

assert(() => c.replace(4, 'c3'));
assert(() => '2 c3' === c.value);
assert(() => '2 c3' === c.toString());
assert(() => '2 c3' === d.body.className);

assert(() => c.replace(2, 1));
assert(() => '1 c3' === c.value);
assert(() => '1 c3' === c.toString());
assert(() => '1 c3' === d.body.className);

assert(() => throws(() => c.replace('', 1), 'SyntaxError'));
assert(() => throws(() => c.replace(1, ''), 'SyntaxError'));
assert(() => throws(() => c.replace('a b', 2), 'InvalidCharacterError'));
assert(() => throws(() => c.replace(2, 'a b'), 'InvalidCharacterError'));

assert(() => c.toggle(1));
assert(() => 'c3' === c.value);
assert(() => 'c3' === c.toString());
assert(() => 'c3' === d.body.className);

assert(() => c.toggle(1));
assert(() => 'c3 1' === c.value);
assert(() => 'c3 1' === c.toString());
assert(() => 'c3 1' === d.body.className);

assert(() => c.toggle('c3'));
assert(() => '1' === c.value);
assert(() => '1' === c.toString());
assert(() => '1' === d.body.className);

assert(() => c.toggle('c3'));
assert(() => '1 c3' === c.value);
assert(() => '1 c3' === c.toString());
assert(() => '1 c3' === d.body.className);

assert(() => throws(() => c.toggle(''), 'SyntaxError'));
assert(() => throws(() => c.toggle('a b'), 'InvalidCharacterError'));

assert(() => !c.toggle(1, true));
assert(() => c.toggle(1, false));
assert(() => 'c3' === c.value);
assert(() => !c.toggle(1, false));

assert(() => !c.toggle('c3', true));
assert(() => c.toggle('c3', false));
assert(() => '' === c.value);
assert(() => 0 === c.length);
assert(() => !c.toggle('c3', false));

assert(() => !c.toggle(1, false));
assert(() => c.toggle(1, true));
assert(() => '1' === c.value);
assert(() => 1 === c.length);
assert(() => !c.toggle(1, true));

assert(() => !c.toggle('c1', false));
assert(() => c.toggle('c1', true));
assert(() => '1 c1' === c.value);
assert(() => 2 === c.length);
assert(() => !c.toggle('c1', true));

let n = 0;
for(const e of c.keys()) {
    assert(() => n === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.values()) {
    if(0 === n) assert(() => '1' === e);
    if(1 === n) assert(() => 'c1' === e);
    ++n;
}
assert(() => 2 == n);

n = 0;
for(const e of c.entries()) {
    if(0 === n) assert(() => '1' === e[1]);
    if(1 === n) assert(() => 'c1' === e[1]);
    assert(() => n === e[0]);
    ++n;
}
assert(() => 2 == n);

n = 0;
c.forEach((e, i, s) => {
    assert(() => s == c);
    assert(() => i == n);
    if(0 === n) assert(() => '1' === e);
    if(1 === n) assert(() => 'c1' === e);
    ++n;
});
assert(() => 2 == n);

const v = new Array();
c.forEach(function(e, i, s) {
    assert(() => s == c);
    assert(() => i == this.length);
    if(0 === i) assert(() => '1' === e);
    if(1 === i) assert(() => 'c1' === e);
    this.push(e);
}, v);

assert(() => 2 == v.length);
assert(() => '1' == v[0]);
assert(() => 'c1' == v[1]);

assert(() => throws(() => c.forEach(() => { throw 1 })));
assert(() => throws(() => c.forEach(() => { throw 1 }, 0)));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(dataset)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const ds = d.body.dataset;

assert(() => ds === d.body.dataset);
ds.a = 1;

assert(() => d.body.hasAttribute('data-a'));
assert(() => '1' === d.body.getAttribute('data-a'));
assert(() => '1' === ds.a);

assert(() => 1 === Object.getOwnPropertyNames(ds).length);
assert(() => 'a' === Object.getOwnPropertyNames(ds)[0]);

let n = 0;
for(const k in ds) {
    assert(() => k === 'a');
    n += 1;
}
assert(() => 1 === n);

ds.xY = 'test';

assert(() => d.body.hasAttribute('data-x-y'));
assert(() => 'test' === d.body.getAttribute('data-x-y'));
assert(() => 'test' === ds.xY);

assert(() => 2 === Object.getOwnPropertyNames(ds).length);
assert(() => 'a' === Object.getOwnPropertyNames(ds)[0]);
assert(() => 'xY' === Object.getOwnPropertyNames(ds)[1]);

n = 0;
for(const k in ds) {
    if(0 === n) assert(() => k === 'a');
    else if(1 === n) assert(() => k === 'xY');
    n += 1;
}
assert(() => 2 === n);

d.body.setAttribute('data-foo--bar', 0);
assert(() => 'foo-Bar' in ds);

d.body.setAttribute('data--foo-bar', 0);
assert(() => 'FooBar' in ds);

d.body.setAttribute('data--foo--bar', 0);
assert(() => 'Foo-Bar' in ds);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(insertAdjacentElement)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));

const a = div.insertAdjacentElement('beforebegin', d.createElement('a'));
const b = div.insertAdjacentElement('afterbegin', d.createElement('b'));
const p = div.insertAdjacentElement('beforeend', d.createElement('p'));
const u = div.insertAdjacentElement('afterend', d.createElement('u'));

assert(() => 'A' === a.nodeName);
assert(() => 'B' === b.nodeName);
assert(() => 'P' === p.nodeName);
assert(() => 'U' === u.nodeName);

assert(() => div === a.nextSibling);
assert(() => b === div.firstChild);
assert(() => p === div.lastChild);
assert(() => u === div.nextSibling);

const h1 = div.insertAdjacentElement('beforebegin', d.createElement('h1'));
const li = div.insertAdjacentElement('afterbegin', d.createElement('li'));
const ul = div.insertAdjacentElement('beforeend', d.createElement('ul'));
const h2 = div.insertAdjacentElement('afterend', d.createElement('h2'));

assert(() => 'H1' === h1.nodeName);
assert(() => 'LI' === li.nodeName);
assert(() => 'UL' === ul.nodeName);
assert(() => 'H2' === h2.nodeName);

assert(() => a === h1.previousSibling);
assert(() => h1 === div.previousSibling);
assert(() => li === div.firstChild);
assert(() => b === li.nextSibling);
assert(() => ul === div.lastChild);
assert(() => h2 === div.nextSibling);
assert(() => u === h2.nextSibling);
assert(() => p === ul.previousSibling);

assert(() => throws(() => div.insertAdjacentElement('a', a), 'Wrong position argument [a]'));
assert(() => throws(() => div.insertAdjacentElement('beforebegin', Document.html().body), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('afterbegin', Document.html().body), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('beforeend', Document.html().body), 'WrongDocumentError'));
assert(() => throws(() => div.insertAdjacentElement('afterend', Document.html().body), 'WrongDocumentError'));

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

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));

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

BOOST_AUTO_TEST_CASE(insertAdjacentHTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));
div.insertAdjacentHTML('beforebegin', '<a></a>');
div.insertAdjacentHTML('afterbegin', '<b></b>');
div.insertAdjacentHTML('beforeend', '<u></u>');
div.insertAdjacentHTML('afterend', '<i></i>');

assert(() => 'A' === div.previousSibling.nodeName);
assert(() => 'B' === div.firstChild.nodeName);
assert(() => 'U' === div.lastChild.nodeName);
assert(() => 'I' === div.nextSibling.nodeName);

div.insertAdjacentHTML('beforebegin', '<h1></h1>');
div.insertAdjacentHTML('afterbegin', '<li></li>');
div.insertAdjacentHTML('beforeend', '<ul></ul>');
div.insertAdjacentHTML('afterend', '<h2></h2>');

assert(() => 'A' === div.previousSibling.previousSibling.nodeName);
assert(() => 'H1' === div.previousSibling.nodeName);
assert(() => 'LI' === div.firstChild.nodeName);
assert(() => 'UL' === div.lastChild.nodeName);
assert(() => 'H2' === div.nextSibling.nodeName);
assert(() => 'I' === div.nextSibling.nextSibling.nodeName);

assert(() => throws(() => div.insertAdjacentHTML('a', ''), 'Wrong position argument [a]'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(prepend)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const div = d.body.appendChild(d.createElement('div'));

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

assert(() => throws(() => div.prepend(Document.html().body), 'WrongDocumentError'));
assert(() => throws(() => div.prepend(d.body), 'HierarchyRequestError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(querySelector)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`
<body>
    <div id="side">
        <p class="foo">foo</p>
        <p class="bar">bar</p>
    </div>
    <div id="main">
    </div>
</body>
`);

const q1 = d.querySelectorAll('div');
const q2 = d.body.querySelectorAll('div');

assert(() => 2 === q1.length);
assert(() => 2 === q2.length);

assert(() => 'side' === q1[0].id);
assert(() => 'side' === q2[0].id);
assert(() => 'main' === q1[1].id);
assert(() => 'main' === q2[1].id);

assert(() => q1[0] === q2[0]);
assert(() => q1[1] === q2[1]);

assert(() => q1[0] === d.querySelector('div'));
assert(() => q1[0] === d.body.querySelector('div'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getElementById)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`
<body>
  <div id="side">
    <p class="foo">foo</p>
    <p class="bar">bar</p>
  </div>
  <div id="main">
  </div>
</body>
`);

const a = d.getElementById('side');
assert(() => d.body.firstElementChild === a);
assert(() => d.body.firstElementChild === d.querySelector('#side'));
assert(() => 'DIV' === a.nodeName);

const b = d.getElementById('main');
assert(() => d.body.lastElementChild === b);
assert(() => d.body.lastElementChild === d.querySelector('#main'));
assert(() => 'DIV' === b.nodeName);

assert(() => null === d.getElementById('foo'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getElementsByTagName)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`
<body>
    <div>
        <p>foo</p>
        <p>bar</p>
    </div>
    <div>
    </div>
</body>
`);

(function(){
    const d0 = d.getElementsByTagName('div');
    assert(() => 1 === d._live);
    assert(() => 2 === d0.length);

    const d1 = d.getElementsByTagName('div');
    assert(() => 1 === d._live);
    assert(() => 2 === d1.length);

    const d2 = d.body.getElementsByTagName('div');
    assert(() => 2 === d._live);
    assert(() => 2 === d2.length);
})();

assert(() => 0 === d._live);

const e = d.getElementsByTagName('div');
assert(() => 2 === e.length);

const p = e[0].getElementsByTagName('p');
assert(() => 2 === p.length);

const q = e[1].getElementsByTagName('p');
assert(() => 0 === q.length);

const z = d.body.appendChild(d.createElement('div'));
assert(() => 3 === e.length);
assert(() => z === e[2]);

const y = e[1].appendChild(d.createElement('p'));
assert(() => 1 === q.length);
assert(() => y === q[0]);

y.remove();
assert(() => 0 === q.length);
assert(() => 3 === d._live);

assert(() => 0 === d.getElementsByTagName('div body').length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getElementsByTagNameNS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`
<body>
    <svg xmlns="http://www.w3.org/2000/svg"></svg>
</body>
`);

assert(() => throws(() => d.getElementsByTagNameNS('http://', 'a'), 'NamespaceError'));
assert(() => throws(() => d.body.getElementsByTagNameNS('http://', 'a'), 'NamespaceError'));

const a1 = d.getElementsByTagName('svg');
const b1 = d.getElementsByTagNameNS(null, 'svg');
const c1 = d.getElementsByTagNameNS('http://www.w3.org/2000/svg', 'svg');
const d1 = d.getElementsByTagNameNS('http://www.w3.org/1999/xhtml', 'svg');

const a2 = d.body.getElementsByTagName('svg');
const b2 = d.body.getElementsByTagNameNS(null, 'svg');
const c2 = d.body.getElementsByTagNameNS('http://www.w3.org/2000/svg', 'svg');
const d2 = d.body.getElementsByTagNameNS('http://www.w3.org/1999/xhtml', 'svg');

assert(() => 1 === a1.length);
assert(() => 1 === b1.length);
assert(() => 1 === c1.length);
assert(() => 0 === d1.length);

assert(() => 1 === a2.length);
assert(() => 1 === b2.length);
assert(() => 1 === c2.length);
assert(() => 0 === d2.length);

d.body.appendChild(d.createElement('svg'));

assert(() => 2 === a1.length);
assert(() => 2 === b1.length);
assert(() => 1 === c1.length);
assert(() => 1 === d1.length);

assert(() => 2 === a2.length);
assert(() => 2 === b2.length);
assert(() => 1 === c2.length);
assert(() => 1 === d2.length);

d.body.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'svg'));

assert(() => 3 === a1.length);
assert(() => 3 === b1.length);
assert(() => 2 === c1.length);
assert(() => 1 === d1.length);

assert(() => 3 === a2.length);
assert(() => 3 === b2.length);
assert(() => 2 === c2.length);
assert(() => 1 === d2.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(getElementsByClassName)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`
<body>
    <div class="a">
        <p class="a b">foo</p>
        <p>bar</p>
    </div>
    <div class="b">
    </div>
</body>
`);

(function(){
    const c0 = d.getElementsByClassName('a');
    assert(() => 1 === d._live);
    assert(() => 2 === c0.length);

    const c1 = d.getElementsByClassName('a');
    assert(() => 1 === d._live);
    assert(() => 2 === c1.length);

    const c2 = d.body.getElementsByClassName('b');
    assert(() => 2 === d._live);
    assert(() => 2 === c2.length);
})();

assert(() => 0 === d._live);

const a = d.getElementsByClassName('a');
const b = d.getElementsByClassName('b');
const c = d.getElementsByClassName('a b');

assert(() => 2 === a.length);
assert(() => a[0] === a[1].parentElement);

assert(() => 2 === b.length);
assert(() => a[0] === b[0].parentElement);
assert(() => d.body === b[1].parentElement);

assert(() => 1 === c.length);
assert(() => a[0] === c[0].parentElement);

b[0].classList.remove('b');
assert(() => 1 === b.length);
assert(() => d.body === b[0].parentElement);
assert(() => 0 === c.length);

a[0].classList.add('b');
assert(() => 2 === b.length);
assert(() => d.body === b[0].parentElement);
assert(() => d.body === b[1].parentElement);
assert(() => 1 === c.length);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(toggleAttribute)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

assert(() => d.body.toggleAttribute('a'));
assert(() => '' === d.body.getAttribute('a'));

assert(() => !d.body.toggleAttribute('a'));
assert(() => !d.body.hasAttribute('a'));

assert(() => !d.body.toggleAttribute('a', false));
assert(() => !d.body.hasAttribute('a'));

assert(() => d.body.toggleAttribute('a', true));
assert(() => '' === d.body.getAttribute('a'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(style)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const s = d.body.style;

assert(() => s instanceof window.CSSStyleDeclaration);
assert(() => s instanceof window.CSSStyleProperties);

assert(() => s === d.body.style);
assert(() => 0 === s.length);
assert(() => '' === s.item(0));
assert(() => null === s.getPropertyValue('width'));
assert(() => undefined === s.width);
assert(() => !('width' in s));

d.body.setAttribute('style', 'width: 100px');
assert(() => 1 === s.length);
assert(() => 'width' === s.item(0));
assert(() => '100px' === s.width);
assert(() => '100px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));
assert(() => '' === s.getPropertyPriority('height'));

d.body.setAttribute('style', 'width: 200px; height: 1000px !important');
assert(() => 2 === s.length);
assert(() => 'width' === s.item(0));
assert(() => '200px' === s.width);
assert(() => '200px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));
assert(() => 'height' === s.item(1));
assert(() => '1000px' === s.height);
assert(() => '1000px' === s.getPropertyValue('height'));
assert(() => 'important' === s.getPropertyPriority('height'));
assert(() => '1000px' === s.removeProperty('height'));
assert(() => 1 === s.length);

s.setProperty('width', '300px');
assert(() => 1 === s.length);
assert(() => 'width' in s);
assert(() => 'width' === s.item(0));
assert(() => '300px' === s.width);
assert(() => '300px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));

s.setProperty('height', '2000px', 'asdf');
assert(() => 1 === s.length);

s.setProperty('height', '1000px', '');
assert(() => 2 === s.length);
assert(() => 'width' in s);
assert(() => 'height' in s);
assert(() => 'height' === s.item(1));
assert(() => '1000px' === s.height);
assert(() => '1000px' === s.getPropertyValue('height'));
assert(() => '' === s.getPropertyPriority('height'));

s.setProperty('width', '200px', 'important');
assert(() => 2 === s.length);
assert(() => 'width' === s.item(1));
assert(() => '200px' === s.width);
assert(() => '200px' === s.getPropertyValue('width'));
assert(() => 'important' === s.getPropertyPriority('width'));

d.body.removeAttribute('style');
assert(() => 0 === s.length);
assert(() => !('width' in s));
assert(() => !('height' in s));

s.cssText = 'width: 20px; float: right';
assert(() => 2 === s.length);
assert(() => 'width' in s);
assert(() => 'cssFloat' in s);
assert(() => !('height' in s));
assert(() => 'width: 20px; float: right' === s.cssText);
assert(() => '20px' === s.width);
assert(() => '20px' === s.getPropertyValue('width'));
assert(() => 'right' === s.cssFloat);
assert(() => 'right' === s.getPropertyValue('float'));

s.cssFloat = 'left';
assert(() => 'left' === s.cssFloat);
assert(() => 'left' === s.getPropertyValue('float'));

s.backgroundColor = 'red';
assert(() => 'backgroundColor' in s);
assert(() => 'red' === s.backgroundColor);
assert(() => 'red' === s.getPropertyValue('background-color'));
assert(() => 'width: 20px; float: left; background-color: red;' === s.cssText);

const arr = Array.from(s);
assert(() => 3 === arr.length);
assert(() => arr.indexOf('width') >= 0);
assert(() => arr.indexOf('float') >= 0);
assert(() => arr.indexOf('background-color') >= 0);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(computedStyle)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const c = window.getComputedStyle(d.body);
const s = d.body.style;

assert(() => 'transparent' === c.getPropertyValue('background-color'));
assert(() => '16px' === c.getPropertyValue('font-size'));
assert(() => null === s.getPropertyValue('background-color'));
assert(() => null === s.getPropertyValue('font-size'));
assert(() => 'transparent' === c.backgroundColor);
assert(() => '16px' === c.fontSize);
assert(() => undefined === s.backgroundColor);

s.backgroundColor = 'red';
assert(() => '16px' === c.getPropertyValue('font-size'));
assert(() => 'red' === c.getPropertyValue('background-color'));
assert(() => 'red' === s.getPropertyValue('background-color'));
assert(() => 'red' === c.backgroundColor);
assert(() => '16px' === c.fontSize);
assert(() => 'red' === s.backgroundColor);

const style = d.body.appendChild(d.createElement('style'));
style.textContent = 'body {font-size: 10px}';

assert(() => '10px' === c.getPropertyValue('font-size'));
assert(() => '10px' === c.fontSize);

style.textContent += 'body.body {font-size: 12px}';

assert(() => '10px' === c.getPropertyValue('font-size'));
assert(() => '10px' === c.fontSize);

d.body.className = 'body';

assert(() => '12px' === c.getPropertyValue('font-size'));
assert(() => '12px' === c.fontSize);

style.textContent += '#body { font-size: 8px }';

assert(() => '12px' === c.getPropertyValue('font-size'));
assert(() => '12px' === c.fontSize);

d.body.id = 'body';

assert(() => '8px' === c.getPropertyValue('font-size'));
assert(() => '8px' === c.fontSize);

const d1 = d.body.appendChild(d.createElement('div'));
const dc = window.getComputedStyle(d1);

assert(() => '8px' === c.getPropertyValue('font-size'));
assert(() => '8px' === c.fontSize);

d.body.id = null;

assert(() => '12px' === dc.getPropertyValue('font-size'));
assert(() => '12px' === dc.fontSize);

d.body.classList.remove('body');
assert(() => '10px' === dc.getPropertyValue('font-size'));
assert(() => '10px' === dc.fontSize);

const svg = d.body.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'svg'));
const rect = svg.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'rect'));
const rcst = window.getComputedStyle(rect);

rect.setAttribute('fill', 'red');
assert(() => 'red' === rcst.fill);

style.sheet.replaceSync('rect { fill: blue; } #rect { fill: yellow }');
assert(() => 'blue' === rcst.fill);

rect.id = 'rect';
assert(() => 'yellow' === rcst.fill);

const N = c.length;
assert(() => N === rcst.length);

s.setProperty('all-style', 'custom');
assert(() => N + 1 === c.length);
assert(() => 'all-style' === c.item(N));

style.textContent = 'body { one-style: one }';
assert(() => N + 2 === c.length);
assert(() => 'all-style' === c.item(N));
assert(() => 'one-style' === c.item(N + 1));

const arr = Array.from(c);
assert(() => N + 2 === arr.length);
assert(() => arr.indexOf('all-style') == N);
assert(() => arr.indexOf('one-style') == N + 1);

const sheet = style.sheet;
await sheet.replace('body {font-size: 12px} div{font-size: 10px}');

const div = d.body.appendChild(d.createElement('div'));
assert(() => '10px' == window.getComputedStyle(div).fontSize);

style.textContent = 'body {font-size: 12px !important} div {font-size: 10px}';
assert(() => '12px' == window.getComputedStyle(div).fontSize);

sheet.deleteRule(0);
sheet.insertRule('body {font-size: 14px}');

assert(() => '10px' == window.getComputedStyle(div).fontSize);
assert(() => '14px' == c.fontSize);

assert(() => 2 == sheet.cssRules.length);
assert(() => 'body {font-size: 14px}' == sheet.cssRules[0].cssText);
assert(() => 'div {font-size: 10px}' == sheet.cssRules[1].cssText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(replaceChildren)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html('<body><div></div>foo<p></p></body>');
const c = [...d.body.childNodes];

assert(() => 3 === c.length);
for(const n of c) assert(() => n.isConnected);

d.body.replaceChildren(d.createElement('a'), 'bar');
for(const n of c) assert(() => !n.isConnected);

assert(() => 2 === d.body.childNodes.length);
assert(() => 'A' === d.body.childNodes[0].tagName);
assert(() => 'bar' === d.body.childNodes[1].nodeValue);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
