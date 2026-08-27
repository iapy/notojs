#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(HTML, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(DOMException)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { DOMException, Document } from 'noto:dom';

const document = Document.html();
let exception;
try {
    document.appendChild(document.createElement('extra'));
} catch(error) {
    exception = error;
}

assert(() => exception instanceof DOMException);
assert(() => exception instanceof Error);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

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

BOOST_AUTO_TEST_CASE(Require)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert } from 'noto:assert';
import * as dom from 'noto:dom';
require('dom');

assert(() => window === dom.window);
assert(() => document instanceof dom.Document);
assert(() => document instanceof dom.window.HTMLDocument);
    )JS", context.get(), "cell-001");

    eval(R"JS(
import { assert } from 'noto:assert';
import * as dom from 'noto:dom';

assert(() => window === dom.window);
assert(() => document instanceof dom.Document);
assert(() => document instanceof dom.window.HTMLDocument);

require('dom', {scope: 'cell'});
    )JS", context.get(), "cell-002");

    eval(R"JS(
import { assert } from 'noto:assert';
import * as dom from 'noto:dom';

assert(() => undefined == globalThis.window);
assert(() => undefined == globalThis.document);
require('dom', {scope: 'notebook'});
    )JS", context.get(), "cell-003");

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import * as dom from 'noto:dom';

assert(() => window === dom.window);
assert(() => document instanceof dom.Document);
assert(() => document instanceof dom.window.HTMLDocument);

assert(() => throws(() => require('dom', {scope: 1}), 'No matching function overload found'));
assert(() => throws(() => require('dom', {scope: 'foo'}), 'invalid scope [foo]'));
    )JS", context.get(), "cell-004");

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
assert(() => d.firstChild instanceof window.Node);
assert(() => d.firstChild instanceof window.DocumentType);
assert(() => window.Node.DOCUMENT_TYPE_NODE === d.firstChild.nodeType);
assert(() => 'html' === d.firstChild.nodeName);
assert(() => null === d.firstChild.nodeValue);
assert(() => 'html' === d.firstChild.name);
assert(() => null === d.firstChild.namespaceURI);
assert(() => '' === d.firstChild.publicId);
assert(() => '' === d.firstChild.systemId);

const leafParents = [d.doctype, d.createTextNode('text'), d.createComment('comment')];
for(const parent of leafParents)
{
    assert(() => throws(() => parent.appendChild(d.createElement('div')), 'HierarchyRequestError'));
    assert(() => throws(() => parent.insertBefore(d.createElement('div'), null), 'HierarchyRequestError'));
    assert(() => throws(() => parent.insertBefore(d.createElement('div'), d.documentElement), 'HierarchyRequestError'));
    assert(() => throws(() => parent.replaceChild(d.createElement('div'), d.documentElement), 'HierarchyRequestError'));
    assert(() => !parent.hasChildNodes());
}

const text = d.createTextNode('document text');
assert(() => throws(() => d.appendChild(text), 'HierarchyRequestError'));
assert(() => throws(() => d.insertBefore(text, d.documentElement), 'HierarchyRequestError'));
assert(() => throws(() => d.replaceChild(text, d.documentElement), 'HierarchyRequestError'));
assert(() => null === text.parentNode);
assert(() => d.documentElement === d.lastChild);

const holder = d.createElement('div');
const held = holder.appendChild(d.createElement('span'));
assert(() => throws(() => holder.appendChild(d.doctype), 'HierarchyRequestError'));
assert(() => throws(() => holder.insertBefore(d.doctype, held), 'HierarchyRequestError'));
assert(() => throws(() => holder.replaceChild(d.doctype, held), 'HierarchyRequestError'));
assert(() => held === holder.firstChild);
assert(() => d === d.doctype.parentNode);

const fragment = d.createDocumentFragment();
assert(() => throws(() => fragment.appendChild(d.doctype), 'HierarchyRequestError'));
assert(() => !fragment.hasChildNodes());
assert(() => d === d.doctype.parentNode);

assert(() => d.documentElement === d.lastChild);
assert(() => null === d.namespaceURI);
assert(() => 'http://www.w3.org/1999/xhtml' === d.documentElement.namespaceURI);
assert(() => d.head === d.documentElement.firstChild);
assert(() => d.body === d.documentElement.lastChild);
assert(() => d.head instanceof window.HTMLHeadElement);
assert(() => d.body instanceof window.HTMLBodyElement);
assert(() => '#document' === d.nodeName);
assert(() => undefined === d.tagName);
assert(() => 'HTML' === d.documentElement.nodeName);
assert(() => 'HTML' === d.documentElement.tagName);
assert(() => window.Node.DOCUMENT_NODE === d.nodeType);
assert(() => window.Node.ELEMENT_NODE === d.documentElement.nodeType);
assert(() => d === d.documentElement.parentNode);
assert(() => null === d.nodeValue);
assert(() => null === d.parentElement);
assert(() => null === d.parentNode);

assert(() => throws(() => d.toJSON(), "HTMLDocument cannot be serialized"));
assert(() => throws(() => d.documentElement.toJSON(), "<HTML> cannot be serialized"));

assert(() => d.isDefaultNamespace('http://www.w3.org/1999/xhtml'));
assert(() => !d.isDefaultNamespace('http://www.w3.org/1998/Math/MathML'));

assert(() => d.doctype === d.firstChild);
assert(() => d.doctype instanceof window.DocumentType);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentHierarchy)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';

const d = Document.html();
const root = d.documentElement;
const doctype = d.doctype;
const extra = d.createElement('extra');
assert(() => throws(() => d.appendChild(extra), 'HierarchyRequestError'));
assert(() => throws(() => d.insertBefore(extra, root), 'HierarchyRequestError'));
assert(() => null === extra.parentNode);
assert(() => root === d.documentElement);
assert(() => doctype === d.doctype);

const secondDoctype = doctype.cloneNode();
assert(() => throws(() => d.appendChild(secondDoctype), 'HierarchyRequestError'));
assert(() => null === secondDoctype.parentNode);
assert(() => doctype === d.doctype);

assert(() => throws(() => d.insertBefore(root, doctype), 'HierarchyRequestError'));
assert(() => throws(() => d.appendChild(doctype), 'HierarchyRequestError'));
assert(() => doctype === d.firstChild);
assert(() => root === d.documentElement);

const elements = d.createDocumentFragment();
const first = elements.appendChild(d.createElement('first'));
const second = elements.appendChild(d.createElement('second'));
assert(() => throws(() => d.appendChild(elements), 'HierarchyRequestError'));
assert(() => 2 === elements.childNodes.length);
assert(() => first === elements.firstChild);
assert(() => second === elements.lastChild);
assert(() => root === d.documentElement);

const text = d.createDocumentFragment();
const textNode = text.appendChild(d.createTextNode('text'));
assert(() => throws(() => d.appendChild(text), 'HierarchyRequestError'));
assert(() => textNode === text.firstChild);
assert(() => root === d.documentElement);

const replacementDocument = Document.html();
const oldRoot = replacementDocument.documentElement;
const newRoot = replacementDocument.createElement('html');
assert(() => oldRoot === replacementDocument.replaceChild(newRoot, oldRoot));
assert(() => newRoot === replacementDocument.documentElement);
assert(() => null === oldRoot.parentNode);

const oldDoctype = replacementDocument.doctype;
const newDoctype = oldDoctype.cloneNode();
assert(() => oldDoctype === replacementDocument.replaceChild(newDoctype, oldDoctype));
assert(() => newDoctype === replacementDocument.doctype);
assert(() => null === oldDoctype.parentNode);

const fragmentDocument = Document.html();
fragmentDocument.removeChild(fragmentDocument.documentElement);
const fragment = fragmentDocument.createDocumentFragment();
const fragmentRoot = fragment.appendChild(fragmentDocument.createElement('html'));
assert(() => fragment === fragmentDocument.appendChild(fragment));
assert(() => fragmentRoot === fragmentDocument.documentElement);
assert(() => !fragment.hasChildNodes());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentBatchHierarchy)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';

{
    const d = Document.html();
    const children = [...d.childNodes];
    const comment = d.createComment('comment');
    const secondRoot = d.createElement('html');

    assert(() => throws(() => d.append(comment, secondRoot), 'HierarchyRequestError'));
    assert(() => null === comment.parentNode);
    assert(() => null === secondRoot.parentNode);
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const root = d.documentElement;
    const doctype = d.doctype;
    d.removeChild(root);

    assert(() => throws(() => d.prepend(root), 'HierarchyRequestError'));
    assert(() => null === root.parentNode);
    assert(() => doctype === d.firstChild);
    assert(() => 1 === d.childNodes.length);
}

{
    const d = Document.html();
    const root = d.documentElement;
    const doctype = d.doctype;
    d.removeChild(root);

    assert(() => throws(() => doctype.before(root), 'HierarchyRequestError'));
    assert(() => null === root.parentNode);
    assert(() => doctype === d.firstChild);
    assert(() => 1 === d.childNodes.length);
}

{
    const d = Document.html();
    const oldRoot = d.documentElement;
    const newRoot = d.createElement('html');

    d.replaceChildren(newRoot);
    assert(() => null === oldRoot.parentNode);
    assert(() => newRoot === d.documentElement);
    assert(() => newRoot === d.firstChild);
    assert(() => 1 === d.childNodes.length);
}

{
    const d = Document.html();
    const root = d.documentElement;

    d.replaceChildren(root);
    assert(() => root === d.documentElement);
    assert(() => root === d.firstChild);
    assert(() => 1 === d.childNodes.length);
}

{
    const d = Document.html();
    const children = [...d.childNodes];
    const firstRoot = d.createElement('html');
    const secondRoot = d.createElement('html');

    assert(() => throws(() => d.replaceChildren(firstRoot, secondRoot), 'HierarchyRequestError'));
    assert(() => null === firstRoot.parentNode);
    assert(() => null === secondRoot.parentNode);
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const children = [...d.childNodes];

    assert(() => throws(() => d.replaceChildren('text'), 'HierarchyRequestError'));
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const children = [...d.childNodes];
    const fragment = d.createDocumentFragment();
    const firstRoot = fragment.appendChild(d.createElement('html'));
    const secondRoot = fragment.appendChild(d.createElement('html'));

    assert(() => throws(() => d.replaceChildren(fragment), 'HierarchyRequestError'));
    assert(() => firstRoot === fragment.firstChild);
    assert(() => secondRoot === fragment.lastChild);
    assert(() => 2 === fragment.childNodes.length);
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const children = [...d.childNodes];
    const fragment = d.createDocumentFragment();
    const text = fragment.appendChild(d.createTextNode('text'));

    assert(() => throws(() => d.replaceChildren(fragment), 'HierarchyRequestError'));
    assert(() => text === fragment.firstChild);
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const oldRoot = d.documentElement;
    const fragment = d.createDocumentFragment();
    const newRoot = fragment.appendChild(d.createElement('html'));

    d.replaceChildren(fragment);
    assert(() => null === oldRoot.parentNode);
    assert(() => !fragment.hasChildNodes());
    assert(() => newRoot === d.documentElement);
    assert(() => newRoot === d.firstChild);
}

{
    const d = Document.html();
    const oldRoot = d.documentElement;
    const newRoot = d.createElement('html');

    oldRoot.replaceWith(newRoot);
    assert(() => null === oldRoot.parentNode);
    assert(() => newRoot === d.documentElement);
    assert(() => d.doctype === d.firstChild);
    assert(() => newRoot === d.lastChild);

    const oldDoctype = d.doctype;
    const newDoctype = oldDoctype.cloneNode();
    oldDoctype.replaceWith(newDoctype);
    assert(() => null === oldDoctype.parentNode);
    assert(() => newDoctype === d.doctype);
    assert(() => newDoctype === d.firstChild);
    assert(() => newRoot === d.lastChild);
}

{
    const d = Document.html();
    const children = [...d.childNodes];
    const root = d.documentElement;
    const comment = d.createComment('comment');
    const firstRoot = d.createElement('html');
    const secondRoot = d.createElement('html');

    assert(() => throws(
        () => root.replaceWith(comment, firstRoot, secondRoot),
        'HierarchyRequestError'
    ));
    assert(() => null === comment.parentNode);
    assert(() => null === firstRoot.parentNode);
    assert(() => null === secondRoot.parentNode);
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const doctype = d.doctype;
    const root = d.documentElement;
    const comment = d.createComment('after doctype');

    doctype.after(comment, root);
    assert(() => doctype === d.childNodes[0]);
    assert(() => comment === d.childNodes[1]);
    assert(() => root === d.childNodes[2]);

    const beforeRoot = d.createComment('before root');
    root.before(beforeRoot);
    assert(() => beforeRoot === root.previousSibling);

    const children = [...d.childNodes];
    assert(() => throws(() => root.before('text'), 'HierarchyRequestError'));
    assert(() => children.length === d.childNodes.length);
    for(let i = 0; i < children.length; ++i)
        assert(() => children[i] === d.childNodes[i]);
}

{
    const d = Document.html();
    const parent = d.createElement('parent');
    const existing = parent.appendChild(d.createElement('existing'));

    parent.replaceChildren(existing);
    assert(() => existing === parent.firstChild);
    assert(() => 1 === parent.childNodes.length);

    parent.append(existing, existing);
    assert(() => existing === parent.firstChild);
    assert(() => 1 === parent.childNodes.length);
}
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
assert(() => window.Node.ELEMENT_NODE === b1.nodeType);
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
assert(() => window.Node.ELEMENT_NODE === h1.nodeType);
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
assert(() => '' === d.title);
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
assert(() => window.Node.ELEMENT_NODE === a.nodeType);
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
assert(() => 0 < s.toString().indexOf('xmlns='));

const p = d.createElementNS('http://www.w3.org/2000/svg', 'path');
assert(() => 'http://www.w3.org/2000/svg' === p.namespaceURI);
assert(() => p instanceof window.SVGElement);
assert(() => !(p instanceof window.SVGSVGElement));

const sa =  d.createElementNS('http://www.w3.org/2000/svg', 'circle');
assert(() => sa instanceof window.SVGCircleElement);

const sb =  d.createElementNS('http://www.w3.org/2000/svg', 'ellipse');
assert(() => sb instanceof window.SVGEllipseElement);

const sc =  d.createElementNS('http://www.w3.org/2000/svg', 'line');
assert(() => sc instanceof window.SVGLineElement);

const sd =  d.createElementNS('http://www.w3.org/2000/svg', 'rect');
assert(() => sd instanceof window.SVGRectElement);

const se =  d.createElementNS('http://www.w3.org/2000/svg', 'text');
assert(() => se instanceof window.SVGTextElement);

const sf =  d.createElementNS('http://www.w3.org/2000/svg', 'use');
assert(() => sf instanceof window.SVGUseElement);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(attributes)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const created = d.createAttribute('Data-ID');
assert(() => created instanceof window.Attr);
assert(() => created instanceof window.Node);
assert(() => 'data-id' == created.name);
assert(() => 'data-id' == created.nodeName);
assert(() => window.Node.ATTRIBUTE_NODE == created.nodeType);
assert(() => '' == created.value);
assert(() => '' == created.nodeValue);
assert(() => !created.isConnected);
assert(() => null === created.ownerElement);
assert(() => d === created.ownerDocument);

const sameName = d.createAttribute('Data-ID');
const differentName = d.createAttribute('other');
const differentNamespace = d.createAttributeNS('http://www.w3.org/2000/svg', 'data-id');
assert(() => created.isSameNode(created));
assert(() => !created.isSameNode(sameName));
assert(() => created.isEqualNode(sameName));
assert(() => !created.isEqualNode(differentName));
assert(() => !created.isEqualNode(differentNamespace));
sameName.value = 'different';
assert(() => !created.isEqualNode(sameName));
assert(() => throws(() => d.createAttribute(''), 'InvalidCharacterError'));
assert(() => throws(() => d.createAttribute('*'), 'InvalidCharacterError'));
assert(() => throws(() => d.createAttribute('bad name'), 'InvalidCharacterError'));

const detachedOwner = d.createElement('div');
detachedOwner.setAttribute('owned', 'value');
const owned = detachedOwner.getAttributeNode('owned');
assert(() => !owned.isConnected);
d.body.appendChild(detachedOwner);
assert(() => owned.isConnected);
detachedOwner.remove();
assert(() => !owned.isConnected);
assert(() => detachedOwner === owned.ownerElement);

const a = d.createElement('a');
const n = a.attributes;

assert(() => !a.hasAttribute('href'));
assert(() => null === a.getAttribute('href'));
assert(() => null === n.getNamedItem('href'));

a.setAttribute('HREF', 'http://apple.com');
assert(() => a.hasAttribute('href'));
assert(() => a.hasAttribute('HREF'));
assert(() => 'http://apple.com' == a.getAttribute('href'));
assert(() => 'http://apple.com' == a.getAttribute('HREF'));
assert(() => null !== a.getAttributeNode('href'));
assert(() => a.getAttributeNode('href') === a.getAttributeNode('HREF'));
assert(() => null !== n.getNamedItem('href'));
assert(() => n.getNamedItem('href') === n.getNamedItem('HREF'));
assert(() => 'href' == n.getNamedItem('HREF').name);

a.removeAttribute('HREF');
assert(() => !a.hasAttribute('href'));
assert(() => null === a.getAttribute('href'));
assert(() => null === n.getNamedItem('href'));

a.setAttribute('href', 'http://apple.com');
assert(() => a.hasAttribute('href'));
assert(() => 'http://apple.com' == a.getAttribute('href'));

let href = n.getNamedItem('href');
assert(() => null !== href);
assert(() => !href.isConnected);
assert(() => href === n.getNamedItem('href'));
assert(() => href instanceof window.Attr);
assert(() => href instanceof window.Node);

assert(() => 'http://apple.com' == href.value);
assert(() => 'http://apple.com' == href.nodeValue);
assert(() => 'http://apple.com' == href.textContent);

a.removeAttribute('alt');
assert(() => throws(() => n.removeNamedItem('alt'), 'NotFoundError'));
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

assert(() => '' === a.id);
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
assert(() => '' === a.id);

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

const html = d.createElement('div');
html.setAttributeNS(null, 'Data-ID', '1');
assert(() => !html.hasAttributeNS(null, 'data-id'));
assert(() => html.hasAttributeNS(null, 'Data-ID'));
assert(() => null === html.getAttributeNS(null, 'data-id'));
assert(() => '1' == html.getAttributeNS(null, 'Data-ID'));
assert(() => null === html.getAttributeNodeNS(null, 'data-id'));
assert(() => null !== html.getAttributeNodeNS(null, 'Data-ID'));
assert(() => 'Data-ID' == html.getAttributeNodeNS(null, 'Data-ID').name);
html.removeAttributeNS(null, 'data-id');
assert(() => html.hasAttributeNS(null, 'Data-ID'));
html.removeAttributeNS(null, 'Data-ID');
assert(() => !html.hasAttributeNS(null, 'Data-ID'));
assert(() => null === html.getAttributeNodeNS(null, 'Data-ID'));

const a = d.createElementNS('http://www.w3.org/2000/svg', 'text');

assert(() => !a.hasAttributeNS('http://www.w3.org/2000/svg', 'fill'));
assert(() => null == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

assert(() => throws(() => a.setAttributeNS('', 'href', 'http://apple.com'), 'NamespaceError'));
assert(() => throws(() => a.setAttributeNS('', 'href', 1), 'NamespaceError'));
assert(() => throws(() => a.setAttributeNS('', 'href', null), 'NamespaceError'));
assert(() => throws(() => a.setAttributeNS('', 'href', true)), 'NamespaceError');
assert(() => throws(() => a.setAttributeNS('', 'href', false)), 'NamespaceError');

a.setAttributeNS('http://www.w3.org/2000/svg', 'href', 'http://apple.com');
assert(() => a.hasAttributeNS(null, 'href'));
assert(() => 'http://apple.com' == a.getAttributeNS(null, 'href'));

assert(() => throws(() => a.hasAttributeNS('', 'href')), 'NamespaceError');
assert(() => a.hasAttributeNS('http://www.w3.org/2000/svg', 'href'));
assert(() => 'http://apple.com' == a.getAttributeNS('http://www.w3.org/2000/svg', 'href'));

a.removeAttributeNS('http://www.w3.org/2000/svg', 'href');
assert(() => !a.hasAttributeNS('http://www.w3.org/2000/svg', 'href'));

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

BOOST_AUTO_TEST_CASE(createAttributeNS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const plain = d.createAttributeNS(null, 'Data-ID');
assert(() => plain instanceof window.Attr);
assert(() => plain instanceof window.Node);
assert(() => null === plain.namespaceURI);
assert(() => 'Data-ID' == plain.name);
assert(() => 'Data-ID' == plain.nodeName);
assert(() => window.Node.ATTRIBUTE_NODE == plain.nodeType);
assert(() => '' == plain.value);
assert(() => '' == plain.nodeValue);
assert(() => !plain.isConnected);
assert(() => null === plain.ownerElement);
assert(() => d === plain.ownerDocument);

const svg = d.createAttributeNS('http://www.w3.org/2000/svg', 'viewBox');
assert(() => svg instanceof window.Attr);
assert(() => 'http://www.w3.org/2000/svg' === svg.namespaceURI);
assert(() => 'viewBox' == svg.name);
assert(() => 'viewBox' == svg.nodeName);
assert(() => '' == svg.value);

svg.value = '0 0 100 100';
const el = d.createElementNS('http://www.w3.org/2000/svg', 'svg');
assert(() => null === el.setAttributeNodeNS(svg));
assert(() => !svg.isConnected);
assert(() => el === svg.ownerElement);
assert(() => svg === el.getAttributeNodeNS('http://www.w3.org/2000/svg', 'viewBox'));
assert(() => '0 0 100 100' == el.getAttributeNS('http://www.w3.org/2000/svg', 'viewBox'));

assert(() => throws(() => d.createAttributeNS('http://', 'href'), 'NamespaceError'));
assert(() => throws(() => d.createAttributeNS('http://www.w3.org/2000/svg', ''), 'InvalidCharacterError'));
assert(() => throws(() => d.createAttributeNS('http://www.w3.org/2000/svg', '*'), 'InvalidCharacterError'));
assert(() => throws(() => d.createAttributeNS('http://www.w3.org/2000/svg', 'bad name'), 'InvalidCharacterError'));
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
assert(() => null === bar.namespaceURI);
assert(() => null === baz.namespaceURI);
assert(() => throws(() => b.setNamedItem(bar), 'InUseAttributeError'));
assert(() => throws(() => d.body.children[1].setAttributeNode(bar), 'InUseAttributeError'));
assert(() => d.body.children[0] === bar.ownerElement);
assert(() => d.body.children[1] === baz.ownerElement);
assert(() => bar === a.removeNamedItem('foo'));
assert(() => baz === b.setNamedItem(bar));
assert(() => !baz.isConnected);
assert(() => null === baz.namespaceURI);
assert(() => null === baz.ownerElement);
assert(() => null === a.setNamedItem(baz));
assert(() => baz.isConnected);
assert(() => d.body.children[1] === bar.ownerElement);
assert(() => d.body.children[0] === baz.ownerElement);
assert(() => 20 === d.body.children[1].compareDocumentPosition(bar));
assert(() => 10 === bar.compareDocumentPosition(d.body.children[1]));
assert(() => 20 === d.body.children[0].compareDocumentPosition(baz));
assert(() => 10 === baz.compareDocumentPosition(d.body.children[0]));
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

assert(() => 4 === foo.compareDocumentPosition(baz));
assert(() => 2 === baz.compareDocumentPosition(foo));

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

assert(() => null !== a.getNamedItem('viewBox'));
assert(() => null !== b.getNamedItem('viewBox'));

const avb = a.getNamedItemNS('http://www.w3.org/2000/svg', 'viewBox');
const bvb = b.getNamedItemNS('http://www.w3.org/2000/svg', 'viewBox');
assert(() => d.body.children[0] === avb.ownerElement);
assert(() => d.body.children[1] === bvb.ownerElement);
assert(() => 'http://www.w3.org/2000/svg' === avb.namespaceURI);
assert(() => 'http://www.w3.org/2000/svg' === bvb.namespaceURI);
assert(() => avb === d.body.children[0].getAttributeNodeNS('http://www.w3.org/2000/svg', 'viewBox'));
assert(() => bvb === d.body.children[1].getAttributeNodeNS('http://www.w3.org/2000/svg', 'viewBox'));

assert(() => throws(() => b.setNamedItemNS(avb), 'InUseAttributeError'));
assert(() => throws(() => d.body.children[1].setAttributeNodeNS(avb), 'InUseAttributeError'));
assert(() => d.body.children[0] === avb.ownerElement);
assert(() => d.body.children[1] === bvb.ownerElement);
assert(() => avb === a.removeNamedItemNS('http://www.w3.org/2000/svg', 'viewBox'));
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

BOOST_AUTO_TEST_CASE(DocumentFragment)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const f = d.createDocumentFragment();

assert(() => f instanceof window.DocumentFragment);
assert(() => f instanceof window.Node);
assert(() => 'about:blank' === f.baseURI);
assert(() => !f.isConnected);
assert(() => null === f.firstChild);
assert(() => null === f.lastChild);
assert(() => null === f.namespaceURI);
assert(() => null === f.nextSibling);
assert(() => '#document-fragment' === f.nodeName);
assert(() => window.Node.DOCUMENT_FRAGMENT_NODE === f.nodeType);
assert(() => null === f.nodeValue);
assert(() => d === f.ownerDocument);
assert(() => null === f.parentElement);
assert(() => null === f.parentNode);
assert(() => null === f.previousSibling);
assert(() => '' === f.textContent);
assert(() => !f.hasChildNodes());
assert(() => 0 === f.childNodes.length);
assert(() => 0 === f.childElementCount);
assert(() => 0 === f.children.length);
assert(() => null === f.firstElementChild);
assert(() => null === f.lastElementChild);
assert(() => null === f.querySelector('a'));
assert(() => 0 === f.querySelectorAll('a').length);

const a = d.createElement('a');
const t = d.createTextNode('text');
assert(() => a === f.appendChild(a));
assert(() => t === f.appendChild(t));
assert(() => f.hasChildNodes());
assert(() => 2 === f.childNodes.length);
assert(() => a === f.firstChild);
assert(() => t === f.lastChild);
assert(() => f === a.parentNode);
assert(() => f === t.parentNode);
assert(() => 'text' === f.textContent);
assert(() => a === f.childNodes[0]);
assert(() => t === f.childNodes[1]);
assert(() => t === a.nextSibling);
assert(() => a === t.previousSibling);
assert(() => 1 === f.childElementCount);
assert(() => 1 === f.children.length);
assert(() => a === f.firstElementChild);
assert(() => a === f.lastElementChild);
assert(() => a === f.children[0]);
assert(() => a === f.querySelector('a'));
assert(() => 1 === f.querySelectorAll('a').length);
assert(() => a === f.querySelectorAll('a')[0]);

const clone = f.cloneNode(true);
assert(() => clone instanceof window.DocumentFragment);
assert(() => clone instanceof window.Node);
assert(() => 2 === clone.childNodes.length);
assert(() => clone.firstChild !== a);
assert(() => 'text' === clone.textContent);

const p = d.createElement('p');
f.prepend('start ', p);
assert(() => 4 === f.childNodes.length);
assert(() => 'start text' === f.textContent);
assert(() => 2 === f.childElementCount);
assert(() => p === f.firstElementChild);
assert(() => a === f.lastElementChild);
assert(() => p === f.children[0]);
assert(() => a === f.children[1]);

f.replaceChildren(d.createElement('section'), 'body');
assert(() => 2 === f.childNodes.length);
assert(() => 1 === f.childElementCount);
assert(() => 'SECTION' === f.firstElementChild.tagName);
assert(() => 'body' === f.textContent);

const target = d.createElement('div');
const moved = d.createDocumentFragment();
const b = moved.appendChild(d.createElement('b'));
const i = moved.appendChild(d.createElement('i'));
assert(() => moved === target.appendChild(moved));
assert(() => !moved.hasChildNodes());
assert(() => 0 === moved.childNodes.length);
assert(() => 2 === target.childNodes.length);
assert(() => b === target.firstChild);
assert(() => i === target.lastChild);
assert(() => target === b.parentNode);
assert(() => target === i.parentNode);
assert(() => i === b.nextSibling);
assert(() => b === i.previousSibling);

const target2 = d.createElement('p');
const moved2 = d.createDocumentFragment();
const s = moved2.appendChild(d.createElement('span'));
const em = moved2.appendChild(d.createElement('em'));
target2.append(moved2);
assert(() => !moved2.hasChildNodes());
assert(() => 0 === moved2.childNodes.length);
assert(() => 2 === target2.childNodes.length);
assert(() => s === target2.firstChild);
assert(() => em === target2.lastChild);
assert(() => target2 === s.parentNode);
assert(() => target2 === em.parentNode);
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
assert(() => t instanceof window.CharacterData);
assert(() => t instanceof window.Text);
assert(() => 'about:blank' === t.baseURI);
assert(() => !t.isConnected);
assert(() => null === t.firstChild);
assert(() => null === t.lastChild);
assert(() => null === t.namespaceURI);
assert(() => null === t.nextSibling);
assert(() => '#text' == t.nodeName);
assert(() => window.Node.TEXT_NODE == t.nodeType);
assert(() => 'Some text' == t.nodeValue);
assert(() => 'Some text' == t.data);
assert(() => 9 == t.length);
assert(() => 'text' == t.substringData(5, 4));
t.appendData(' data');
assert(() => 'Some text data' == t.data);
t.insertData(5, 'nice ');
assert(() => 'Some nice text data' == t.data);
t.deleteData(5, 5);
assert(() => 'Some text data' == t.data);
t.replaceData(5, 4, 'TEXT');
assert(() => 'Some TEXT data' == t.data);
t.data = 'Some text';
assert(() => 'Some text' == t.nodeValue);
assert(() => d === t.ownerDocument);
assert(() => null === t.parentElement);
assert(() => null == t.parentNode);
assert(() => null === t.previousSibling);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(CDATASection)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const h = Document.html();
const d = Document.xml('<root><![CDATA[Some cdata]]></root>');
const c = d.documentElement.firstChild;

assert(() => throws(() => h.createCDATASection('Created cdata'), 'NotSupportedError'));
assert(() => c instanceof window.Node);
assert(() => c instanceof window.CharacterData);
assert(() => c instanceof window.Text);
assert(() => c instanceof window.CDATASection);
assert(() => !(c instanceof window.Comment));
assert(() => '#cdata-section' == c.nodeName);
assert(() => window.Node.CDATA_SECTION_NODE == c.nodeType);
assert(() => 'Some cdata' == c.nodeValue);
assert(() => 'Some cdata' == c.data);
assert(() => 10 == c.length);
assert(() => 'cdata' == c.substringData(5, 5));

c.appendData(' data');
assert(() => 'Some cdata data' == c.data);
c.insertData(5, 'nice ');
assert(() => 'Some nice cdata data' == c.data);
c.deleteData(5, 5);
assert(() => 'Some cdata data' == c.data);
c.replaceData(5, 5, 'CDATA');
assert(() => 'Some CDATA data' == c.data);
c.data = 'Some cdata';
assert(() => 'Some cdata' == c.nodeValue);
assert(() => d === c.ownerDocument);
assert(() => d.documentElement === c.parentElement);
assert(() => d.documentElement === c.parentNode);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ProcessingInstruction)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.xml('<root><?target data?></root>');
const p = d.documentElement.firstChild;

assert(() => p instanceof window.Node);
assert(() => p instanceof window.CharacterData);
assert(() => p instanceof window.ProcessingInstruction);
assert(() => !(p instanceof window.Text));
assert(() => !(p instanceof window.CDATASection));
assert(() => !(p instanceof window.Comment));
assert(() => 'target' == p.nodeName);
assert(() => window.Node.PROCESSING_INSTRUCTION_NODE == p.nodeType);
assert(() => 'target' == p.target);
assert(() => null === p.sheet);

const h = Document.html();
const n = h.createProcessingInstruction('created', 'value');
assert(() => n instanceof window.Node);
assert(() => n instanceof window.CharacterData);
assert(() => n instanceof window.ProcessingInstruction);
assert(() => 'created' == n.nodeName);
assert(() => window.Node.PROCESSING_INSTRUCTION_NODE == n.nodeType);
assert(() => 'created' == n.target);
assert(() => 'value' == n.nodeValue);
assert(() => 'value' == n.data);
assert(() => !n.isConnected);
assert(() => null === n.sheet);
assert(() => throws(() => h.createProcessingInstruction('', 'value'), 'InvalidCharacterError'));
assert(() => throws(() => h.createProcessingInstruction('bad name', 'value'), 'InvalidCharacterError'));
assert(() => throws(() => h.createProcessingInstruction('created', 'bad ?> value'), 'InvalidCharacterError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Comment)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const c = d.createComment('Some comment');
assert(() => c instanceof window.Node);
assert(() => c instanceof window.CharacterData);
assert(() => c instanceof window.Comment);
assert(() => 'about:blank' === c.baseURI);
assert(() => !c.isConnected);
assert(() => null === c.firstChild);
assert(() => null === c.lastChild);
assert(() => null === c.namespaceURI);
assert(() => null === c.nextSibling);
assert(() => '#comment' == c.nodeName);
assert(() => window.Node.COMMENT_NODE == c.nodeType);
assert(() => 'Some comment' == c.nodeValue);
assert(() => 'Some comment' == c.data);
assert(() => 12 == c.length);
assert(() => 'comment' == c.substringData(5, 7));
c.appendData(' data');
assert(() => 'Some comment data' == c.data);
c.insertData(5, 'nice ');
assert(() => 'Some nice comment data' == c.data);
c.deleteData(5, 5);
assert(() => 'Some comment data' == c.data);
c.replaceData(5, 7, 'COMMENT');
assert(() => 'Some COMMENT data' == c.data);
c.data = 'Some comment';
assert(() => 'Some comment' == c.nodeValue);
assert(() => d === c.ownerDocument);
assert(() => null === c.parentElement);
assert(() => null == c.parentNode);
assert(() => null === c.previousSibling);

c.nodeValue = 'Other comment';
assert(() => 'Other comment' == c.nodeValue);

d.body.appendChild(c);
assert(() => c.isConnected);
assert(() => c === d.body.firstChild);

const x = Document.xml('<root><!--xml--></root>');
assert(() => x.documentElement.firstChild instanceof window.Comment);
assert(() => 'xml' == x.documentElement.firstChild.nodeValue);
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
assert(() => 20 === div.compareDocumentPosition(a));
assert(() => 10 === a.compareDocumentPosition(div));

div.appendChild(b);

assert(() => 4 === a.compareDocumentPosition(b));
assert(() => 2 === b.compareDocumentPosition(a));
assert(() => 20 === div.compareDocumentPosition(b));
assert(() => 10 === b.compareDocumentPosition(div));

const d = d1.createElement('d');
a.appendChild(d);

assert(() => 2 === b.compareDocumentPosition(d));
assert(() => 4 === d.compareDocumentPosition(b));
assert(() => 20 === a.compareDocumentPosition(d));
assert(() => 10 === d.compareDocumentPosition(a));
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

const root = d.documentElement;
const rootHTML = root.outerHTML;
assert(() => throws(() => { root.outerHTML = '<main></main>'; }, 'NoModificationAllowedError'));
assert(() => root === d.documentElement);
assert(() => rootHTML === root.outerHTML);

const div = d.createElement('div');
div.outerHTML = 'foo';

d.body.appendChild(div);
assert(() => '<body><div></div></body>' === d.body.outerHTML);

div.outerHTML = '<b>bold</b>';
assert(() => '<body><b>bold</b></body>' === d.body.outerHTML);

const table = d.body.appendChild(d.createElement('table'));
const tbody = table.appendChild(d.createElement('tbody'));
const row = tbody.appendChild(d.createElement('tr'));
row.outerHTML = '<tr><td>cell</td></tr>';
assert(() => 'TR' === tbody.firstElementChild.tagName);
assert(() => 'TD' === tbody.firstElementChild.firstElementChild.tagName);
assert(() => 'cell' === tbody.firstElementChild.firstElementChild.textContent);

const fragment = d.createDocumentFragment();
const replaced = fragment.appendChild(d.createElement('div'));
replaced.outerHTML = '<span>fragment</span>';
assert(() => 'SPAN' === fragment.firstElementChild.tagName);
assert(() => 'fragment' === fragment.firstElementChild.textContent);
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

const detached = d.createElement('div');
assert(() => throws(() => { detached.outerText = 'detached'; }, 'NoModificationAllowedError'));
assert(() => '<div></div>' === detached.outerHTML);

const root = d.documentElement;
assert(() => throws(() => { root.outerText = 'root'; }, 'HierarchyRequestError'));
assert(() => root === d.documentElement);

const fragment = d.createDocumentFragment();
const fragmentChild = fragment.appendChild(d.createElement('div'));
fragmentChild.outerText = 'fragment';
assert(() => null === fragmentChild.parentNode);
assert(() => 'fragment' === fragment.firstChild.nodeValue);

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

BOOST_AUTO_TEST_CASE(replaceWith)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();

const e = d.body.appendChild(d.createElement('div'));
e.replaceWith(d.createElement('p'), 'foo', d.createElement('a'));
assert(() => 3 === d.body.childNodes.length);
assert(() => 'P' == d.body.childNodes[0].nodeName);
assert(() => 'foo' == d.body.childNodes[1].nodeValue);
assert(() => 'A' == d.body.childNodes[2].nodeName);
assert(() => e.parentNode === null);

const s = d.body.appendChild(d.createElement('span'));
const beforeSelf = s.previousSibling;
s.replaceWith('before', s, 'after');
assert(() => s.parentNode === d.body);
assert(() => 'before' === s.previousSibling.nodeValue);
assert(() => beforeSelf === s.previousSibling.previousSibling);
assert(() => 'after' === s.nextSibling.nodeValue);
assert(() => null === s.nextSibling.nextSibling);

const t = d.body.appendChild(d.createTextNode('text'));
t.replaceWith(d.createElement('b'), 'bar');
assert(() => t.parentNode === null);
assert(() => 'B' == d.body.childNodes[d.body.childNodes.length - 2].nodeName);
assert(() => 'bar' == d.body.lastChild.nodeValue);

const c = d.body.appendChild(d.createComment('comment'));
c.before('before comment');
c.after('after comment');
assert(() => 'before comment' == c.previousSibling.nodeValue);
assert(() => 'after comment' == c.nextSibling.nodeValue);
c.replaceWith('comment text');
assert(() => c.parentNode === null);
assert(() => 'comment text' == d.body.childNodes[d.body.childNodes.length - 2].nodeValue);
assert(() => 'after comment' == d.body.lastChild.nodeValue);

d.body.lastChild.remove();
assert(() => 'comment text' == d.body.lastChild.nodeValue);

const detached = d.createElement('i');
detached.replaceWith('ignored');
assert(() => detached.parentNode === null);

assert(() => throws(() => d.body.firstChild.replaceWith(d.body), 'HierarchyRequestError'));
assert(() => throws(() => d.body.firstChild.replaceWith(Document.html().body), 'WrongDocumentError'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(documentTypeChildNode)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const doctype = d.doctype;

assert(() => doctype instanceof window.DocumentType);
doctype.after(d.createComment('after'));
assert(() => doctype.nextSibling.nodeValue === 'after');

doctype.nextSibling.before(d.createComment('before'));
assert(() => doctype.nextSibling.nodeValue === 'before');
assert(() => doctype.nextSibling.nextSibling.nodeValue === 'after');

doctype.nextSibling.replaceWith(d.createComment('replaced'));
assert(() => doctype.nextSibling.nodeValue === 'replaced');
assert(() => doctype.nextSibling.nextSibling.nodeValue === 'after');

doctype.nextSibling.remove();
assert(() => doctype.nextSibling.nodeValue === 'after');
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

assert(() => throws(() => p.closest('['), 'SyntaxError'));
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

assert(() => throws(() => p.matches('['), 'SyntaxError'));
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

assert(() => !c.toggle(1));
assert(() => 'c3' === c.value);
assert(() => 'c3' === c.toString());
assert(() => 'c3' === d.body.className);

assert(() => c.toggle(1));
assert(() => 'c3 1' === c.value);
assert(() => 'c3 1' === c.toString());
assert(() => 'c3 1' === d.body.className);

assert(() => !c.toggle('c3'));
assert(() => '1' === c.value);
assert(() => '1' === c.toString());
assert(() => '1' === d.body.className);

assert(() => c.toggle('c3'));
assert(() => '1 c3' === c.value);
assert(() => '1 c3' === c.toString());
assert(() => '1 c3' === d.body.className);

assert(() => throws(() => c.toggle(''), 'SyntaxError'));
assert(() => throws(() => c.toggle('a b'), 'InvalidCharacterError'));

assert(() => c.toggle(1, true));
assert(() => !c.toggle(1, false));
assert(() => 'c3' === c.value);
assert(() => !c.toggle(1, false));

assert(() => c.toggle('c3', true));
assert(() => !c.toggle('c3', false));
assert(() => '' === c.value);
assert(() => 0 === c.length);
assert(() => !c.toggle('c3', false));

assert(() => !c.toggle(1, false));
assert(() => c.toggle(1, true));
assert(() => '1' === c.value);
assert(() => 1 === c.length);
assert(() => c.toggle(1, true));

assert(() => !c.toggle('c1', false));
assert(() => c.toggle('c1', true));
assert(() => '1 c1' === c.value);
assert(() => 2 === c.length);
assert(() => c.toggle('c1', true));

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

BOOST_AUTO_TEST_CASE(tokenListCacheGC)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const document = Document.html();
const element = document.body.appendChild(document.createElement('a'));
element.className = 'alpha';
element.rel = 'noopener';
const classList = element.classList;
const relList = element.relList;

assert(() => classList instanceof window.DOMTokenList);
assert(() => relList instanceof window.DOMTokenList);
assert(() => classList === element.classList);
assert(() => relList === element.relList);
assert(() => classList !== relList);
assert(() => classList.contains('alpha'));
assert(() => relList.contains('noopener'));

globalThis.tokenListCache = { element, classList, relList };
    )JS", context.get(), "token-list-cache-1");

    eval(R"JS(
tokenListCache.classList = null;
tokenListCache.relList = null;
    )JS", context.get(), "token-list-cache-2");

    JS_RunGC(JS_GetRuntime(context.get()));

    eval(R"JS(
import { assert } from 'noto:assert';
import { window } from 'noto:dom';

const element = tokenListCache.element;
const classList = element.classList;
const relList = element.relList;

assert(() => classList instanceof window.DOMTokenList);
assert(() => relList instanceof window.DOMTokenList);
assert(() => classList === element.classList);
assert(() => relList === element.relList);
assert(() => classList !== relList);
assert(() => classList.contains('alpha'));
assert(() => relList.contains('noopener'));

classList.add('beta');
relList.add('nofollow');
assert(() => 'alpha beta' === element.className);
assert(() => 'noopener nofollow' === element.rel);
delete globalThis.tokenListCache;
    )JS", context.get(), "token-list-cache-3");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(inlineStyleSetPropertyDelimiter)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Document } from 'noto:dom';

const d = Document.html();
const e = d.createElement('div');

e.setAttribute('style', 'color: red');
e.style.setProperty('background-color', 'blue', 'important');
assert(() => 'red' === e.style.getPropertyValue('color'));
assert(() => 'blue' === e.style.getPropertyValue('background-color'));
assert(() => 'important' === e.style.getPropertyPriority('background-color'));
assert(() => 2 === e.style.length);

e.setAttribute('style', 'color: green   ');
e.style.setProperty('margin-left', '10px');
assert(() => 'green' === e.style.getPropertyValue('color'));
assert(() => '10px' === e.style.getPropertyValue('margin-left'));
assert(() => '' === e.style.getPropertyPriority('margin-left'));
assert(() => 2 === e.style.length);

e.setAttribute('style', 'width: 3px; height: 4px');
e.style.setProperty('height', '2px', 'important');
e.style.setProperty('height', '1px', 'important');
assert(() => '3px' === e.style.getPropertyValue('width'));
assert(() => '1px' === e.style.getPropertyValue('height'));
assert(() => 'important' === e.style.getPropertyPriority('height'));
assert(() => 2 === e.style.length);
assert(() => 'width: 3px; height: 1px !important;' === e.style.cssText);
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

BOOST_AUTO_TEST_CASE(atomicNodeMutations)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Document } from 'noto:dom';

const d = Document.html();
const other = Document.html();

for(const method of ['append', 'prepend']) {
    const target = d.createElement('target');
    const valid = d.createElement('valid');
    const foreign = other.createElement('foreign');

    assert(() => throws(() => target[method](valid, foreign), 'WrongDocumentError'));
    assert(() => null === valid.parentNode);
    assert(() => !target.hasChildNodes());

    const cycleValid = d.createElement('cycle-valid');
    assert(() => throws(() => target[method](cycleValid, target), 'HierarchyRequestError'));
    assert(() => null === cycleValid.parentNode);
    assert(() => !target.hasChildNodes());
}

for(const method of ['before', 'after']) {
    const parent = d.createElement('parent');
    const target = parent.appendChild(d.createElement('target'));
    const valid = d.createElement('valid');
    const foreign = other.createElement('foreign');

    assert(() => throws(() => target[method](valid, foreign), 'WrongDocumentError'));
    assert(() => null === valid.parentNode);
    assert(() => 1 === parent.childNodes.length);
    assert(() => target === parent.firstChild);

    const cycleValid = d.createElement('cycle-valid');
    assert(() => throws(() => target[method](cycleValid, parent), 'HierarchyRequestError'));
    assert(() => null === cycleValid.parentNode);
    assert(() => 1 === parent.childNodes.length);
    assert(() => target === parent.firstChild);
}

{
    const parent = d.createElement('parent');
    const a = parent.appendChild(d.createElement('a'));
    const b = parent.appendChild(d.createElement('b'));
    const c = parent.appendChild(d.createElement('c'));

    parent.prepend(a, b);
    assert(() => a === parent.childNodes[0]);
    assert(() => b === parent.childNodes[1]);
    assert(() => c === parent.childNodes[2]);

    a.after(c);
    assert(() => a === parent.childNodes[0]);
    assert(() => c === parent.childNodes[1]);
    assert(() => b === parent.childNodes[2]);

    const x = d.createElement('x');
    c.before(c, x);
    assert(() => a === parent.childNodes[0]);
    assert(() => c === parent.childNodes[1]);
    assert(() => x === parent.childNodes[2]);
    assert(() => b === parent.childNodes[3]);

    parent.append(a, b, a);
    assert(() => c === parent.childNodes[0]);
    assert(() => x === parent.childNodes[1]);
    assert(() => b === parent.childNodes[2]);
    assert(() => a === parent.childNodes[3]);

    b.replaceWith('left', b, 'right');
    assert(() => 'left' === b.previousSibling.nodeValue);
    assert(() => 'right' === b.nextSibling.nodeValue);

    parent.replaceChildren(a, a);
    assert(() => a === parent.firstChild);
    assert(() => 1 === parent.childNodes.length);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(fragmentIdentityMutations)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Document } from 'noto:dom';

const d = Document.html();

{
    const parent = d.createElement('parent');
    const fragment = d.createDocumentFragment();
    const a = fragment.appendChild(d.createElement('a'));
    const b = fragment.appendChild(d.createElement('b'));

    parent.append(fragment, fragment);
    assert(() => !fragment.hasChildNodes());
    assert(() => 2 === parent.childNodes.length);
    assert(() => a === parent.childNodes[0]);
    assert(() => b === parent.childNodes[1]);
}

{
    const parent = d.createElement('parent');
    const fragment = d.createDocumentFragment();
    const a = fragment.appendChild(d.createElement('a'));
    const b = fragment.appendChild(d.createElement('b'));

    parent.append(fragment, a);
    assert(() => !fragment.hasChildNodes());
    assert(() => 2 === parent.childNodes.length);
    assert(() => b === parent.childNodes[0]);
    assert(() => a === parent.childNodes[1]);
}

{
    const parent = d.createElement('parent');
    const marker = parent.appendChild(d.createElement('marker'));
    const fragment = d.createDocumentFragment();
    const a = fragment.appendChild(d.createElement('a'));
    const b = fragment.appendChild(d.createElement('b'));

    parent.prepend(a, fragment);
    assert(() => !fragment.hasChildNodes());
    assert(() => 3 === parent.childNodes.length);
    assert(() => a === parent.childNodes[0]);
    assert(() => b === parent.childNodes[1]);
    assert(() => marker === parent.childNodes[2]);
}
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
assert(() => throws(() => d.querySelector('['), 'SyntaxError'));
assert(() => throws(() => d.querySelectorAll('['), 'SyntaxError'));
assert(() => throws(() => d.body.querySelector('['), 'SyntaxError'));
assert(() => throws(() => d.body.querySelectorAll('['), 'SyntaxError'));
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

d.body.setAttribute('a', 'value');
const attribute = d.body.getAttributeNode('a');
assert(() => d.body.toggleAttribute('a', true));
assert(() => 'value' === d.body.getAttribute('a'));
assert(() => 'value' === attribute.value);
assert(() => attribute === d.body.getAttributeNode('a'));
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

const first = d.body.firstChild;
const valid = d.createElement('valid');
const foreign = Document.html().createElement('foreign');
assert(() => throws(() => d.body.replaceChildren(valid, foreign), 'WrongDocumentError'));
assert(() => null === valid.parentNode);
assert(() => 2 === d.body.childNodes.length);
assert(() => first === d.body.firstChild);

const cycleValid = d.createElement('cycle-valid');
assert(() => throws(() => d.body.replaceChildren(cycleValid, d.body), 'HierarchyRequestError'));
assert(() => null === cycleValid.parentNode);
assert(() => 2 === d.body.childNodes.length);
assert(() => first === d.body.firstChild);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SVG)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

assert(() => Object.getPrototypeOf(window.SVGElement.prototype) === window.Element.prototype);
const d = Document.html(`
    <svg xmlns="http://www.w3.org/2000/svg" x="1" y="2" width="300" height="200" viewBox="0 0 300 200" preserveAspectRatio="xMidYMid meet">
        <title>Main SVG</title>
        <metadata>metadata</metadata>
        <style>.shape { fill: red; }</style>
        <script>void 0;</script>
        <view id="view" viewBox="0 0 10 10"/>
        <defs>
            <circle cx="10" cy="20" r="5"/>
            <animate id="animate" attributeName="opacity" from="0" to="1" dur="1s" repeatCount="indefinite" fill="freeze"/>
            <animateMotion id="animate-motion" begin="0s" dur="2s"><mpath id="mpath" href="#motion-path"/></animateMotion>
            <animateTransform id="animate-transform" attributeName="transform" type="rotate" from="0" to="360" dur="3s"/>
            <set id="set" attributeName="visibility" to="hidden" begin="4s"/>
            <solidColor id="paint" solid-color="red" solid-opacity="0.5"/>
            <linearGradient id="gradient" x1="1" y1="2" x2="3" y2="4" gradientUnits="userSpaceOnUse" spreadMethod="reflect" href="#base-gradient">
                <stop offset="0" stop-color="red"/>
                <stop offset="1" stop-color="blue"/>
            </linearGradient>
            <radialGradient id="radial" cx="5" cy="6" r="7" fx="8" fy="9" fr="1" gradientUnits="objectBoundingBox" spreadMethod="repeat" href="#gradient"/>
            <pattern id="pattern" x="11" y="12" width="13" height="14" patternUnits="userSpaceOnUse" patternContentUnits="objectBoundingBox" preserveAspectRatio="xMidYMid meet" viewBox="0 0 10 20" href="#base-pattern"/>
            <clipPath id="clip" clipPathUnits="userSpaceOnUse"/>
            <mask id="mask" x="15" y="16" width="17" height="18" maskUnits="userSpaceOnUse" maskContentUnits="objectBoundingBox"/>
            <marker id="marker" refX="19" refY="20" markerWidth="21" markerHeight="22" markerUnits="strokeWidth" orient="auto" preserveAspectRatio="none" viewBox="0 0 5 5"/>
            <filter id="filter" x="1" y="2" width="30" height="40" filterUnits="userSpaceOnUse" primitiveUnits="objectBoundingBox" href="#base-filter">
                <feBlend in="SourceGraphic" in2="BackgroundImage" mode="multiply" result="blend"/>
                <feColorMatrix in="blend" type="matrix" values="1 0 0 0 0" result="matrix"/>
                <feComponentTransfer in="matrix" result="component"><feFuncR type="linear" slope="2"/><feFuncG type="gamma" amplitude="1" exponent="2" offset="0.5"/><feFuncB type="table" tableValues="0 1"/><feFuncA type="identity"/></feComponentTransfer>
                <feComposite in="component" in2="SourceAlpha" operator="arithmetic" k1="1" k2="2" k3="3" k4="4" result="composite"/>
                <feConvolveMatrix in="composite" order="3" kernelMatrix="0 1 0 1 -4 1 0 1 0" divisor="1" bias="0" targetX="1" targetY="1" edgeMode="duplicate" kernelUnitLength="2" preserveAlpha="true" result="convolve"/>
                <feDiffuseLighting in="convolve" surfaceScale="2" diffuseConstant="1" kernelUnitLength="1" result="diffuse"><feDistantLight azimuth="45" elevation="60"/></feDiffuseLighting>
                <feDisplacementMap in="diffuse" in2="SourceGraphic" scale="5" xChannelSelector="R" yChannelSelector="G" result="displace"/>
                <feDropShadow dx="2" dy="3" stdDeviation="4" result="drop"/>
                <feFlood result="flood"/>
                <feGaussianBlur in="flood" stdDeviation="2" result="blur"/>
                <feImage href="image.png" preserveAspectRatio="none" crossOrigin="anonymous" result="image"/>
                <feMerge result="merge"><feMergeNode in="blur"/><feMergeNode in="image"/></feMerge>
                <feMorphology in="merge" operator="dilate" radius="2" result="morph"/>
                <feOffset in="morph" dx="6" dy="7" result="offset"/>
                <feSpecularLighting in="offset" surfaceScale="3" specularConstant="2" specularExponent="4" kernelUnitLength="1" result="specular"><fePointLight x="1" y="2" z="3"/></feSpecularLighting>
                <feSpecularLighting in="offset" result="spot"><feSpotLight x="1" y="2" z="3" pointsAtX="4" pointsAtY="5" pointsAtZ="6" specularExponent="7" limitingConeAngle="8"/></feSpecularLighting>
                <feTile in="spot" result="tile"/>
                <feTurbulence baseFrequency="0.5" numOctaves="2" seed="3" stitchTiles="stitch" type="fractalNoise" result="turbulence"/>
            </filter>
        </defs>
        <g id="group">
            <circle cx="1" cy="2" r="3"/>
        </g>
        <switch id="switch"><circle cx="4" cy="5" r="6"/></switch>
        <a href="#target" target="_blank"><rect width="1" height="1"/></a>
        <foreignObject x="35" y="36" width="37" height="38"/>
        <ellipse cx="5" cy="6" rx="7" ry="8"/>
        <line x1="1" x2="11" y1="2" y2="22"/>
        <polyline points="0,0 10,10 20,0"/>
        <mpath id="motion-path" d="M 0 0 L 10 0"/>
        <path d="M 0 0 L 3 4 H 6 V 8 Z" pathLength="20"/>
        <polygon points="0,0 10,10 20,0"/>
        <rect x="10" y="20" width="30" height="40" rx="2" ry="3"/>
        <image href="image.png" x="7" y="8" width="90" height="100"/>
        <text x="1" y="2" rotate="30" textLength="100">
            <desc>A text</desc>
            <tspan x="3" y="4" dx="5" dy="6" rotate="45" textLength="50">span</tspan>
            <textPath href="#path" startOffset="7" method="align" spacing="auto">path text</textPath>
        </text>
        <symbol id="shape" viewBox="0 0 10 10">
            <rect width="10" height="10"/>
        </symbol>
        <use href="#shape" x="3" y="4" width="5" height="6"/>
    </svg>
`);

assert(() => null == d.querySelector('svg').ownerSVGElement);

const svg = d.querySelector('svg');
assert(() => svg instanceof window.SVGSVGElement);
assert(() => svg instanceof window.SVGElement);
assert(() => !(svg instanceof window.HTMLElement));
d.querySelectorAll('svg, svg *').forEach(element => {
    assert(() => element instanceof window.Element);
    assert(() => element instanceof window.SVGElement);
    assert(() => !(element instanceof window.HTMLElement));
});
assert(() => svg.x instanceof window.SVGLength);
assert(() => svg.y instanceof window.SVGLength);
assert(() => svg.width instanceof window.SVGLength);
assert(() => svg.height instanceof window.SVGLength);
assert(() => 1 == svg.x);
assert(() => 2 == svg.y);
assert(() => 300 == svg.width);
assert(() => 200 == svg.height);
assert(() => 0 == svg.viewBox.x);
assert(() => 0 == svg.viewBox.y);
assert(() => 300 == svg.viewBox.width);
assert(() => 200 == svg.viewBox.height);
assert(() => 'xMidYMid meet' == svg.preserveAspectRatio);
svg.preserveAspectRatio = 'none';
assert(() => 'none' == svg.getAttribute('preserveAspectRatio'));

const title = d.querySelector('svg > title');
assert(() => title instanceof window.SVGTitleElement);
assert(() => !(title instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === title.ownerSVGElement);
assert(() => 'Main SVG' == title.textContent);

const metadata = d.querySelector('metadata');
assert(() => metadata instanceof window.SVGMetadataElement);
assert(() => !(metadata instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === metadata.ownerSVGElement);
assert(() => 'metadata' == metadata.textContent);

const style = d.querySelector('svg > style');
assert(() => style instanceof window.SVGStyleElement);
assert(() => style instanceof window.SVGElement);
assert(() => !(style instanceof window.HTMLStyleElement));
assert(() => !(style instanceof window.SVGGraphicsElement));
assert(() => style.sheet instanceof window.CSSStyleSheet);

const script = d.querySelector('svg > script');
assert(() => script instanceof window.SVGScriptElement);
assert(() => !(script instanceof window.SVGGraphicsElement));

const view = d.querySelector('view');
assert(() => view instanceof window.SVGViewElement);
assert(() => !(view instanceof window.SVGGraphicsElement));
assert(() => 10 == view.viewBox.width);
assert(() => 10 == view.viewBox.height);

const animate = d.querySelector('#animate');
assert(() => animate instanceof window.SVGAnimateElement);
assert(() => animate instanceof window.SVGAnimationElement);
assert(() => 'opacity' == animate.attributeName);
assert(() => '0' == animate.from);
assert(() => '1' == animate.to);
assert(() => '1s' == animate.dur);
assert(() => 'indefinite' == animate.repeatCount);
assert(() => 'freeze' == animate.fill);

const animateMotion = d.querySelector('#animate-motion');
assert(() => animateMotion instanceof window.SVGAnimateMotionElement);
assert(() => animateMotion instanceof window.SVGAnimationElement);
assert(() => '0s' == animateMotion.begin);
assert(() => '2s' == animateMotion.dur);

const mpath = d.querySelector('#mpath');
assert(() => mpath instanceof window.SVGMPathElement);
assert(() => '#motion-path' == mpath.href);

const animateTransform = d.querySelector('#animate-transform');
assert(() => animateTransform instanceof window.SVGAnimateTransformElement);
assert(() => animateTransform instanceof window.SVGAnimationElement);
assert(() => 'transform' == animateTransform.attributeName);
assert(() => 'rotate' == animateTransform.type);

const set = d.querySelector('#set');
assert(() => set instanceof window.SVGSetElement);
assert(() => set instanceof window.SVGAnimationElement);
assert(() => 'visibility' == set.attributeName);
assert(() => 'hidden' == set.to);

const solidColor = d.querySelector('#paint');
assert(() => solidColor instanceof window.SVGSolidColorElement);
assert(() => !(solidColor instanceof window.SVGGraphicsElement));

const q = (function(){
    return d.querySelector('circle').r;
})();

assert(() => 5 == q);
assert(() => q == q.baseVal);

const f = d.querySelector('defs');
assert(() => f instanceof window.SVGDefsElement);
assert(() => f instanceof window.SVGGraphicsElement);

const lg = d.querySelector('linearGradient');
assert(() => lg instanceof window.SVGLinearGradientElement);
assert(() => !(lg instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === lg.ownerSVGElement);
assert(() => lg.x1 instanceof window.SVGLength);
assert(() => lg.y1 instanceof window.SVGLength);
assert(() => lg.x2 instanceof window.SVGLength);
assert(() => lg.y2 instanceof window.SVGLength);
assert(() => 1 == lg.x1);
assert(() => 2 == lg.y1);
assert(() => 3 == lg.x2);
assert(() => 4 == lg.y2);
assert(() => 'userSpaceOnUse' == lg.gradientUnits);
assert(() => 'reflect' == lg.spreadMethod);
assert(() => '#base-gradient' == lg.href);
lg.x2.value = 30;
lg.href = '#other-gradient';
assert(() => 30 == lg.x2);
assert(() => 30 == parseInt(lg.getAttribute('x2')));
assert(() => '#other-gradient' == lg.getAttribute('href'));

const rg = d.querySelector('radialGradient');
assert(() => rg instanceof window.SVGRadialGradientElement);
assert(() => !(rg instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === rg.ownerSVGElement);
assert(() => rg.cx instanceof window.SVGLength);
assert(() => rg.cy instanceof window.SVGLength);
assert(() => rg.r instanceof window.SVGLength);
assert(() => rg.fx instanceof window.SVGLength);
assert(() => rg.fy instanceof window.SVGLength);
assert(() => rg.fr instanceof window.SVGLength);
assert(() => 5 == rg.cx);
assert(() => 6 == rg.cy);
assert(() => 7 == rg.r);
assert(() => 8 == rg.fx);
assert(() => 9 == rg.fy);
assert(() => 1 == rg.fr);
assert(() => 'objectBoundingBox' == rg.gradientUnits);
assert(() => 'repeat' == rg.spreadMethod);
assert(() => '#gradient' == rg.href);
rg.fr.value = 2;
rg.href = '#radial-other';
assert(() => 2 == rg.fr);
assert(() => 2 == parseInt(rg.getAttribute('fr')));
assert(() => '#radial-other' == rg.getAttribute('href'));

const pattern = d.querySelector('pattern');
assert(() => pattern instanceof window.SVGPatternElement);
assert(() => !(pattern instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === pattern.ownerSVGElement);
assert(() => pattern.x instanceof window.SVGLength);
assert(() => pattern.y instanceof window.SVGLength);
assert(() => pattern.width instanceof window.SVGLength);
assert(() => pattern.height instanceof window.SVGLength);
assert(() => 11 == pattern.x);
assert(() => 12 == pattern.y);
assert(() => 13 == pattern.width);
assert(() => 14 == pattern.height);
assert(() => 'userSpaceOnUse' == pattern.patternUnits);
assert(() => 'objectBoundingBox' == pattern.patternContentUnits);
assert(() => 'xMidYMid meet' == pattern.preserveAspectRatio);
assert(() => '#base-pattern' == pattern.href);
pattern.width.value = 20;
pattern.href = '#other-pattern';
assert(() => 20 == pattern.width);
assert(() => 20 == parseInt(pattern.getAttribute('width')));
assert(() => '#other-pattern' == pattern.getAttribute('href'));

const clip = d.querySelector('clipPath');
assert(() => clip instanceof window.SVGClipPathElement);
assert(() => !(clip instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === clip.ownerSVGElement);
assert(() => 'userSpaceOnUse' == clip.clipPathUnits);
clip.clipPathUnits = 'objectBoundingBox';
assert(() => 'objectBoundingBox' == clip.clipPathUnits);
assert(() => 'objectBoundingBox' == clip.getAttribute('clipPathUnits'));

const mask = d.querySelector('mask');
assert(() => mask instanceof window.SVGMaskElement);
assert(() => !(mask instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === mask.ownerSVGElement);
assert(() => mask.x instanceof window.SVGLength);
assert(() => mask.y instanceof window.SVGLength);
assert(() => mask.width instanceof window.SVGLength);
assert(() => mask.height instanceof window.SVGLength);
assert(() => 15 == mask.x);
assert(() => 16 == mask.y);
assert(() => 17 == mask.width);
assert(() => 18 == mask.height);
assert(() => 'userSpaceOnUse' == mask.maskUnits);
assert(() => 'objectBoundingBox' == mask.maskContentUnits);
mask.height.value = 24;
mask.maskUnits = 'objectBoundingBox';
assert(() => 24 == mask.height);
assert(() => 24 == parseInt(mask.getAttribute('height')));
assert(() => 'objectBoundingBox' == mask.maskUnits);

const marker = d.querySelector('marker');
assert(() => marker instanceof window.SVGMarkerElement);
assert(() => !(marker instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === marker.ownerSVGElement);
assert(() => marker.refX instanceof window.SVGLength);
assert(() => marker.refY instanceof window.SVGLength);
assert(() => marker.markerWidth instanceof window.SVGLength);
assert(() => marker.markerHeight instanceof window.SVGLength);
assert(() => 19 == marker.refX);
assert(() => 20 == marker.refY);
assert(() => 21 == marker.markerWidth);
assert(() => 22 == marker.markerHeight);
assert(() => 'strokeWidth' == marker.markerUnits);
assert(() => 'auto' == marker.orient);
assert(() => 'none' == marker.preserveAspectRatio);
marker.markerWidth.value = 25;
marker.orient = '45';
assert(() => 25 == marker.markerWidth);
assert(() => 25 == parseInt(marker.getAttribute('markerWidth')));
assert(() => '45' == marker.orient);

const filter = d.querySelector('filter');
assert(() => filter instanceof window.SVGFilterElement);
assert(() => !(filter instanceof window.SVGGraphicsElement));
assert(() => filter.x instanceof window.SVGLength);
assert(() => 1 == filter.x);
assert(() => 2 == filter.y);
assert(() => 30 == filter.width);
assert(() => 40 == filter.height);
assert(() => 'userSpaceOnUse' == filter.filterUnits);
assert(() => 'objectBoundingBox' == filter.primitiveUnits);
assert(() => '#base-filter' == filter.href);
filter.width.value = 50;
filter.href = '#other-filter';
assert(() => 50 == filter.width);
assert(() => '#other-filter' == filter.getAttribute('href'));

const blend = d.querySelector('feBlend');
assert(() => blend instanceof window.SVGFEBlendElement);
assert(() => 'SourceGraphic' == blend.in);
assert(() => 'BackgroundImage' == blend.in2);
assert(() => 'multiply' == blend.mode);
assert(() => 'blend' == blend.result);

const colorMatrix = d.querySelector('feColorMatrix');
assert(() => colorMatrix instanceof window.SVGFEColorMatrixElement);
assert(() => 'matrix' == colorMatrix.type);
assert(() => '1 0 0 0 0' == colorMatrix.values);

const component = d.querySelector('feComponentTransfer');
assert(() => component instanceof window.SVGFEComponentTransferElement);
assert(() => 'matrix' == component.in);
assert(() => d.querySelector('feFuncR') instanceof window.SVGFEFuncRElement);
assert(() => d.querySelector('feFuncG') instanceof window.SVGFEFuncGElement);
assert(() => d.querySelector('feFuncB') instanceof window.SVGFEFuncBElement);
assert(() => d.querySelector('feFuncA') instanceof window.SVGFEFuncAElement);
assert(() => 'linear' == d.querySelector('feFuncR').type);
assert(() => '2' == d.querySelector('feFuncR').slope);

const composite = d.querySelector('feComposite');
assert(() => composite instanceof window.SVGFECompositeElement);
assert(() => 'arithmetic' == composite.operator);
assert(() => '4' == composite.k4);

const convolve = d.querySelector('feConvolveMatrix');
assert(() => convolve instanceof window.SVGFEConvolveMatrixElement);
assert(() => '3' == convolve.order);
assert(() => 'duplicate' == convolve.edgeMode);

const diffuse = d.querySelector('feDiffuseLighting');
assert(() => diffuse instanceof window.SVGFEDiffuseLightingElement);
assert(() => '2' == diffuse.surfaceScale);
assert(() => d.querySelector('feDistantLight') instanceof window.SVGFEDistantLightElement);
assert(() => '45' == d.querySelector('feDistantLight').azimuth);

const displacement = d.querySelector('feDisplacementMap');
assert(() => displacement instanceof window.SVGFEDisplacementMapElement);
assert(() => 'R' == displacement.xChannelSelector);
assert(() => 'G' == displacement.yChannelSelector);

const drop = d.querySelector('feDropShadow');
assert(() => drop instanceof window.SVGFEDropShadowElement);
assert(() => drop.dx instanceof window.SVGLength);
assert(() => 2 == drop.dx);
assert(() => 3 == drop.dy);
assert(() => '4' == drop.stdDeviation);

assert(() => d.querySelector('feFlood') instanceof window.SVGFEFloodElement);
const blur = d.querySelector('feGaussianBlur');
assert(() => blur instanceof window.SVGFEGaussianBlurElement);
assert(() => 'flood' == blur.in);
assert(() => '2' == blur.stdDeviation);

const feImage = d.querySelector('feImage');
assert(() => feImage instanceof window.SVGFEImageElement);
assert(() => 'image.png' == feImage.href);
assert(() => 'none' == feImage.preserveAspectRatio);
assert(() => 'anonymous' == feImage.crossOrigin);
feImage.crossOrigin = 'use-credentials';
assert(() => 'use-credentials' == feImage.crossOrigin);
assert(() => 'use-credentials' == feImage.getAttribute('crossorigin'));

assert(() => d.querySelector('feMerge') instanceof window.SVGFEMergeElement);
assert(() => d.querySelector('feMergeNode') instanceof window.SVGFEMergeNodeElement);
assert(() => 'blur' == d.querySelector('feMergeNode').in);

const morphology = d.querySelector('feMorphology');
assert(() => morphology instanceof window.SVGFEMorphologyElement);
assert(() => 'dilate' == morphology.operator);
assert(() => '2' == morphology.radius);

const offset = d.querySelector('feOffset');
assert(() => offset instanceof window.SVGFEOffsetElement);
assert(() => offset.dx instanceof window.SVGLength);
assert(() => 6 == offset.dx);
assert(() => 7 == offset.dy);

assert(() => d.querySelector('feSpecularLighting') instanceof window.SVGFESpecularLightingElement);
assert(() => d.querySelector('fePointLight') instanceof window.SVGFEPointLightElement);
assert(() => '3' == d.querySelector('fePointLight').z);
assert(() => d.querySelector('feSpotLight') instanceof window.SVGFESpotLightElement);
assert(() => '8' == d.querySelector('feSpotLight').limitingConeAngle);
assert(() => d.querySelector('feTile') instanceof window.SVGFETileElement);

const turbulence = d.querySelector('feTurbulence');
assert(() => turbulence instanceof window.SVGFETurbulenceElement);
assert(() => '0.5' == turbulence.baseFrequency);
assert(() => '2' == turbulence.numOctaves);
assert(() => 'fractalNoise' == turbulence.type);

const stop = d.querySelector('stop');
assert(() => stop instanceof window.SVGStopElement);
assert(() => !(stop instanceof window.SVGGraphicsElement));
assert(() => d.querySelector('svg') === stop.ownerSVGElement);
assert(() => 0 == stop.offset);
stop.offset = 0.5;
assert(() => 0.5 == stop.offset);
assert(() => 0.5 == parseFloat(stop.getAttribute('offset')));

const g = d.querySelector('g');
assert(() => g instanceof window.SVGGElement);
assert(() => g instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === g.ownerSVGElement);

const a = d.querySelector('svg > a');
assert(() => a instanceof window.SVGAElement);
assert(() => a instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === a.ownerSVGElement);
assert(() => '#target' == a.href);
assert(() => '_blank' == a.target);
a.href = '#other-target';
a.target = '_self';
assert(() => '#other-target' == a.getAttribute('href'));
assert(() => '_self' == a.target);

const sw = d.querySelector('switch');
assert(() => sw instanceof window.SVGSwitchElement);
assert(() => sw instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === sw.ownerSVGElement);

const fo = d.querySelector('foreignObject');
assert(() => fo instanceof window.SVGForeignObjectElement);
assert(() => fo instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === fo.ownerSVGElement);
assert(() => fo.x instanceof window.SVGLength);
assert(() => fo.y instanceof window.SVGLength);
assert(() => fo.width instanceof window.SVGLength);
assert(() => fo.height instanceof window.SVGLength);
assert(() => 35 == fo.x);
assert(() => 36 == fo.y);
assert(() => 37 == fo.width);
assert(() => 38 == fo.height);
fo.width.value = 39;
assert(() => 39 == fo.width);
assert(() => 39 == parseInt(fo.getAttribute('width')));

const c = d.querySelector('defs circle');
assert(() => c instanceof window.SVGCircleElement);
assert(() => c instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === c.ownerSVGElement);
assert(() => c.cx instanceof window.SVGLength);
assert(() => c.cy instanceof window.SVGLength);
assert(() => c.r  instanceof window.SVGLength);
assert(() => 15 === c.cx + c.r);
assert(() => 25 === c.cy + c.r);
assert(() => c.cx == c.cx);

c.setAttribute('r', 15);
assert(() => 15 == q);
assert(() => 15 == parseInt(c.getAttribute('r')));

q.value = 20;
assert(() => 20 == q);
assert(() => 20 == parseInt(c.getAttribute('r')));

const e = d.querySelector('ellipse');
assert(() => e instanceof window.SVGEllipseElement);
assert(() => e instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === e.ownerSVGElement);
assert(() => e.cx instanceof window.SVGLength);
assert(() => e.cy instanceof window.SVGLength);
assert(() => e.rx instanceof window.SVGLength);
assert(() => e.ry instanceof window.SVGLength);
assert(() => 12 === e.cx + e.rx);
assert(() => 14 === e.cy + e.ry);

const l = d.querySelector('line');
assert(() => l instanceof window.SVGLineElement);
assert(() => l instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === l.ownerSVGElement);
assert(() => l.x1 instanceof window.SVGLength);
assert(() => l.x2 instanceof window.SVGLength);
assert(() => l.y1 instanceof window.SVGLength);
assert(() => l.y2 instanceof window.SVGLength);
assert(() => 10 === l.x2 - l.x1);
assert(() => 20 === l.y2 - l.y1);

const pl = d.querySelector('polyline');
assert(() => pl instanceof window.SVGPolylineElement);
assert(() => pl instanceof window.SVGGraphicsElement);
assert(() => '0,0 10,10 20,0' == pl.points);
pl.points = '1,1 2,2';
assert(() => '1,1 2,2' == pl.points);
assert(() => '1,1 2,2' == pl.getAttribute('points'));

const p = d.querySelector('path');
assert(() => p instanceof window.SVGPathElement);
assert(() => p instanceof window.SVGGraphicsElement);
assert(() => 20 == p.pathLength);
p.pathLength = 30;
assert(() => 30 == p.pathLength);
assert(() => 30 == parseInt(p.getAttribute('pathLength')));
assert(() => 22 == p.getTotalLength());

const p0 = p.getPointAtLength(0);
assert(() => 0 == p0.x);
assert(() => 0 == p0.y);
const p5 = p.getPointAtLength(5);
assert(() => 3 == p5.x);
assert(() => 4 == p5.y);
const p8 = p.getPointAtLength(8);
assert(() => 6 == p8.x);
assert(() => 4 == p8.y);
const p12 = p.getPointAtLength(12);
assert(() => 6 == p12.x);
assert(() => 8 == p12.y);
const p22 = p.getPointAtLength(22);
assert(() => 0 == p22.x);
assert(() => 0 == p22.y);

p.setAttribute('d', 'm 0 0 l 3 4 h 3 v 4 z');
assert(() => 22 == p.getTotalLength());

p.setAttribute('d', 'M 0 0 Q 10 10 20 0');
assert(() => p.getTotalLength() > 20);
const qEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 20 == Math.round(qEnd.x));
assert(() => 0 == Math.round(qEnd.y));

p.setAttribute('d', 'M 0 0 T 20 0');
assert(() => 20 == Math.round(p.getTotalLength()));

p.setAttribute('d', 'M 0 0 C 0 10 20 10 20 0');
assert(() => p.getTotalLength() > 20);
const cEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 20 == Math.round(cEnd.x));
assert(() => 0 == Math.round(cEnd.y));

p.setAttribute('d', 'M 0 0 C 0 10 20 10 20 0 S 40 -10 40 0');
const sEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 40 == Math.round(sEnd.x));
assert(() => 0 == Math.round(sEnd.y));

p.setAttribute('d', 'M0 0 A 10 10 0 0 1 20 0');
assert(() => p.getTotalLength() > 20);
assert(() => p.getTotalLength() < 40);
const aEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 20 == Math.round(aEnd.x));
assert(() => 0 == Math.round(aEnd.y));

p.setAttribute('d', 'M20 0 a 10 10 0 0 1 -20 0');
const arEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 0 == Math.round(arEnd.x));
assert(() => 0 == Math.round(arEnd.y));

p.setAttribute('d', 'M0 0 A 10 5 45 0 1 20 0');
assert(() => p.getTotalLength() > 20);
const rotatedAEnd = p.getPointAtLength(p.getTotalLength());
assert(() => 20 == Math.round(rotatedAEnd.x));
assert(() => 0 == Math.round(rotatedAEnd.y));

const pg = d.querySelector('polygon');
assert(() => pg instanceof window.SVGPolygonElement);
assert(() => pg instanceof window.SVGGraphicsElement);
assert(() => '0,0 10,10 20,0' == pg.points);
pg.points = '3,3 4,4 5,3';
assert(() => '3,3 4,4 5,3' == pg.points);
assert(() => '3,3 4,4 5,3' == pg.getAttribute('points'));

const r = d.querySelector('svg > rect');
assert(() => r instanceof window.SVGRectElement);
assert(() => r instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === r.ownerSVGElement);
assert(() => r.x instanceof window.SVGLength);
assert(() => r.y instanceof window.SVGLength);
assert(() => r.width instanceof window.SVGLength);
assert(() => r.height instanceof window.SVGLength);
assert(() => r.rx instanceof window.SVGLength);
assert(() => r.ry instanceof window.SVGLength);
assert(() => 40 === r.x + r.width);
assert(() => 60 === r.y + r.height);
assert(() => 2 == r.rx);
assert(() => 3 == r.ry);

const img = d.querySelector('image');
assert(() => img instanceof window.SVGImageElement);
assert(() => img instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === img.ownerSVGElement);
assert(() => 'image.png' == img.href);
assert(() => 7 == img.x);
assert(() => 8 == img.y);
assert(() => 90 == img.width);
assert(() => 100 == img.height);
img.href = 'other.png';
assert(() => 'other.png' == img.href);
assert(() => 'other.png' == img.getAttribute('href'));

const t = d.querySelector('text');
assert(() => t instanceof window.SVGTextElement);
assert(() => t instanceof window.SVGGraphicsElement);
assert(() => t instanceof window.SVGTextContentElement);
assert(() => d.querySelector('svg') === t.ownerSVGElement);
assert(() => t.x instanceof window.SVGLength);
assert(() => t.y instanceof window.SVGLength);
assert(() => t.dx instanceof window.SVGLength);
assert(() => t.dy instanceof window.SVGLength);
assert(() => t.rotate instanceof window.SVGLength);
assert(() => t.textLength instanceof window.SVGLength);
assert(() => 100 == t.textLength);
assert(() => 0 == t.dx);
assert(() => 0 == t.dy);

const ts = d.querySelector('tspan');
assert(() => ts instanceof window.SVGTSpanElement);
assert(() => ts instanceof window.SVGTextContentElement);
assert(() => ts instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === ts.ownerSVGElement);
assert(() => ts.x instanceof window.SVGLength);
assert(() => ts.y instanceof window.SVGLength);
assert(() => ts.dx instanceof window.SVGLength);
assert(() => ts.dy instanceof window.SVGLength);
assert(() => ts.rotate instanceof window.SVGLength);
assert(() => ts.textLength instanceof window.SVGLength);
assert(() => 3 == ts.x);
assert(() => 4 == ts.y);
assert(() => 5 == ts.dx);
assert(() => 6 == ts.dy);
assert(() => 45 == ts.rotate);
assert(() => 50 == ts.textLength);
ts.dx.value = 8;
assert(() => 8 == ts.dx);
assert(() => 8 == parseInt(ts.getAttribute('dx')));

const tp = d.querySelector('textPath');
assert(() => tp instanceof window.SVGTextPathElement);
assert(() => tp instanceof window.SVGTextContentElement);
assert(() => tp instanceof window.SVGGraphicsElement);
assert(() => d.querySelector('svg') === tp.ownerSVGElement);
assert(() => '#path' == tp.href);
assert(() => tp.startOffset instanceof window.SVGLength);
assert(() => 7 == tp.startOffset);
assert(() => 'align' == tp.method);
assert(() => 'auto' == tp.spacing);
tp.startOffset.value = 9;
tp.href = '#other-path';
assert(() => 9 == tp.startOffset);
assert(() => 9 == parseInt(tp.getAttribute('startOffset')));
assert(() => '#other-path' == tp.getAttribute('href'));

assert(() => t.firstElementChild instanceof window.SVGDescElement);
assert(() => !(t.firstElementChild instanceof window.SVGGraphicsElement));
assert(() => 'A text' == t.firstElementChild.textContent);

const s = d.querySelector('symbol');
assert(() => s instanceof window.SVGSymbolElement);
assert(() => !(s instanceof window.SVGGraphicsElement));

const u = d.querySelector('use');
assert(() => u instanceof window.SVGUseElement);
assert(() => u instanceof window.SVGGraphicsElement);
assert(() => '#shape' == u.href);
assert(() => 3 == u.x);
assert(() => 4 == u.y);
assert(() => 5 == u.width);
assert(() => 6 == u.height);
u.href = '#other';
assert(() => '#other' == u.href);
assert(() => '#other' == u.getAttribute('href'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HTMLElementSubclasses)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html(`<body>
    <a href="http://google.com">a</a>
    <form method="POST">
        <button type="button" name="submit" value="value"/>
        <input type="checkbox" name="checked" value="checked"/>
        <textarea></textarea>
    </form>
    <table>
        <thead>
            <tr><th>en</th><td>de</td></tr>
        </thead>
        <tr><td>one</td><td>eins</td></tr>
        <tr><td>two</td><td>zwei</td></tr>
    </table>
</body>`);

const a = d.querySelector('a');
assert(() => a instanceof window.HTMLAnchorElement);
assert(() => 'http://google.com' == a.href);

a.href = 'https://apple.com';
assert(() => 'https://apple.com' == a.href);
assert(() => '<a href="https://apple.com">a</a>' == a.outerHTML);
assert(() => '' == d.createElement('a').href);
assert(() => '' == d.createElement('a').target);

assert(() => d.body instanceof window.HTMLBodyElement);

const button = d.querySelector('button');
assert(() => button instanceof window.HTMLButtonElement);
assert(() => button.form == d.querySelector('form'));
assert(() => 'button' == button.type);
assert(() => 'submit' == button.name);
assert(() => 'value' == button.value);

button.type = 'submit';
button.name = 'button';
button.value = 'change';

assert(() => 'submit' == button.type);
assert(() => 'button' == button.name);
assert(() => 'change' == button.value);

assert(() => 0 < button.outerHTML.indexOf('type="submit"'));
assert(() => 0 < button.outerHTML.indexOf('name="button"'));
assert(() => 0 < button.outerHTML.indexOf('value="change"'));

const defaultButton = d.createElement('button');
assert(() => defaultButton instanceof window.HTMLButtonElement);
assert(() => 'submit' == defaultButton.type);
defaultButton.type = 'RESET';
assert(() => 'reset' == defaultButton.type);
defaultButton.type = 'invalid';
assert(() => 'submit' == defaultButton.type);
assert(() => 'invalid' == defaultButton.getAttribute('type'));
assert(() => '' == defaultButton.name);
assert(() => '' == defaultButton.value);

const form = d.querySelector('form');
assert(() => '' == form.action);
form.action = 'https://example.com/post';
assert(() => 'https://example.com/post' == form.action);
assert(() => 3 == form.length);
assert(() => 3 == form.elements.length);
assert(() => button == form.elements[0]);
assert(() => d.querySelector('input') == form.elements[1]);
assert(() => d.querySelector('textarea') == form.elements[2]);

const detachedForm = d.createElement('form');
detachedForm.id = 'detached-form';
const detachedInput = detachedForm.appendChild(d.createElement('input'));
assert(() => 1 == detachedForm.length);
assert(() => detachedInput == detachedForm.elements[0]);
detachedInput.setAttribute('form', 'missing-form');
assert(() => 0 == detachedForm.length);
detachedInput.setAttribute('form', 'detached-form');
assert(() => 1 == detachedForm.length);
assert(() => detachedInput == detachedForm.elements[0]);

const detachedContainer = d.createElement('div');
const detachedExternalForm = detachedContainer.appendChild(d.createElement('form'));
detachedExternalForm.id = 'detached-external-form';
const detachedExternalInput = detachedContainer.appendChild(d.createElement('input'));
detachedExternalInput.setAttribute('form', 'detached-external-form');
assert(() => 1 == detachedExternalForm.length);
assert(() => detachedExternalInput == detachedExternalForm.elements[0]);

const externalForm = d.body.appendChild(d.createElement('form'));
externalForm.id = 'external-form';
assert(() => 0 == externalForm.length);
assert(() => 0 == externalForm.elements.length);

const fieldset = d.body.appendChild(d.createElement('fieldset'));
assert(() => fieldset instanceof window.HTMLFieldSetElement);
assert(() => null === fieldset.form);
assert(() => false === fieldset.disabled);
assert(() => 0 == fieldset.elements.length);
assert(() => '' == fieldset.name);
assert(() => 'fieldset' == fieldset.type);
fieldset.name = 'group';
fieldset.disabled = true;
assert(() => 'group' == fieldset.name);
assert(() => true === fieldset.disabled);
assert(() => '' === fieldset.getAttribute('disabled'));
const fieldsetInput = fieldset.appendChild(d.createElement('input'));
const fieldsetButton = fieldset.appendChild(d.createElement('button'));
assert(() => 2 == fieldset.elements.length);
assert(() => fieldsetInput == fieldset.elements[0]);
assert(() => fieldsetButton == fieldset.elements[1]);
fieldset.setAttribute('form', 'external-form');
assert(() => externalForm == fieldset.form);
assert(() => 1 == externalForm.length);
assert(() => fieldset == externalForm.elements[0]);
fieldset.setAttribute('form', 'missing-form');
assert(() => null === fieldset.form);
fieldset.removeAttribute('form');
fieldset.disabled = false;
assert(() => false === fieldset.disabled);
assert(() => null === fieldset.getAttribute('disabled'));
assert(() => 0 == externalForm.length);

const input = d.querySelector('input');
assert(() => input instanceof window.HTMLInputElement);
assert(() => input.form == form);
input.setAttribute('form', 'external-form');
assert(() => input.form == externalForm);
assert(() => 2 == form.length);
assert(() => 2 == form.elements.length);
assert(() => button == form.elements[0]);
assert(() => d.querySelector('textarea') == form.elements[1]);
assert(() => 1 == externalForm.length);
assert(() => input == externalForm.elements[0]);
input.setAttribute('form', 'missing-form');
assert(() => null === input.form);
assert(() => 2 == form.length);
assert(() => 0 == externalForm.length);
input.removeAttribute('form');
assert(() => input.form == form);
assert(() => 3 == form.length);
assert(() => 0 == externalForm.length);
assert(() => 'checkbox' == input.type);
assert(() => 'checked' == input.name);
assert(() => 'checked' == input.value);

const textarea = d.querySelector('textarea');
assert(() => textarea instanceof window.HTMLTextAreaElement);
assert(() => textarea.form == d.querySelector('form'));

input.type = 'text';
input.name = 'name';
input.value = 'foo';
assert(() => 'text' == input.type);
assert(() => 'name' == input.name);
assert(() => 'foo' == input.value);

assert(() => 0 < input.outerHTML.indexOf('type="text"'));
assert(() => 0 < input.outerHTML.indexOf('name="name"'));
assert(() => 0 < input.outerHTML.indexOf('value="foo"'));

const defaultInput = d.body.appendChild(d.createElement('input'));
assert(() => defaultInput instanceof window.HTMLInputElement);
assert(() => null === defaultInput.form);
assert(() => 'text' == defaultInput.type);
defaultInput.type = 'RADIO';
assert(() => 'radio' == defaultInput.type);
defaultInput.type = 'invalid';
assert(() => 'text' == defaultInput.type);
assert(() => 'invalid' == defaultInput.getAttribute('type'));
assert(() => '' == defaultInput.name);
assert(() => '' == defaultInput.value);
defaultInput.setAttribute('form', 'external-form');
assert(() => defaultInput.form == externalForm);
assert(() => 3 == form.length);
assert(() => 1 == externalForm.length);
assert(() => defaultInput == externalForm.elements[0]);

textarea.setAttribute('form', 'external-form');
assert(() => textarea.form == externalForm);
assert(() => 2 == form.length);
assert(() => 2 == externalForm.length);
assert(() => defaultInput == externalForm.elements[1]);
assert(() => textarea == externalForm.elements[0]);

const explicitLabel = d.body.appendChild(d.createElement('label'));
assert(() => explicitLabel instanceof window.HTMLLabelElement);
assert(() => '' == explicitLabel.htmlFor);
explicitLabel.htmlFor = 'default-input';
assert(() => 'default-input' == explicitLabel.htmlFor);
assert(() => 'default-input' == explicitLabel.getAttribute('for'));
assert(() => null === explicitLabel.control);
defaultInput.id = 'default-input';
assert(() => explicitLabel.control == defaultInput);
assert(() => externalForm == explicitLabel.form);
defaultInput.setAttribute('form', 'missing-form');
assert(() => null === explicitLabel.form);
defaultInput.removeAttribute('form');
assert(() => null === explicitLabel.form);

const explicitForm = d.body.appendChild(d.createElement('form'));
explicitForm.id = 'explicit-label-form';
defaultInput.setAttribute('form', 'explicit-label-form');
assert(() => explicitForm == explicitLabel.form);
defaultInput.removeAttribute('form');
explicitLabel.htmlFor = 'not-labelable';
const notLabelable = d.body.appendChild(d.createElement('div'));
notLabelable.id = 'not-labelable';
notLabelable.appendChild(defaultInput);
assert(() => null === explicitLabel.control);
assert(() => null === explicitLabel.form);

const detachedLabelContainer = d.createElement('div');
const detachedLabel = detachedLabelContainer.appendChild(d.createElement('label'));
detachedLabel.htmlFor = 'detached-label-input';
const detachedLabelInput = detachedLabelContainer.appendChild(d.createElement('input'));
detachedLabelInput.id = 'detached-label-input';
assert(() => detachedLabelInput == detachedLabel.control);
assert(() => null === detachedLabel.form);
const detachedLabelForm = detachedLabelContainer.appendChild(d.createElement('form'));
detachedLabelForm.id = 'detached-label-form';
detachedLabelInput.setAttribute('form', 'detached-label-form');
assert(() => detachedLabelForm == detachedLabel.form);
detachedLabelInput.setAttribute('form', 'missing-form');
assert(() => null === detachedLabel.form);
detachedLabelInput.removeAttribute('form');
detachedLabelForm.appendChild(detachedLabelInput);
assert(() => detachedLabelForm == detachedLabel.form);

const nestedForm = d.body.appendChild(d.createElement('form'));
const nestedLabel = nestedForm.appendChild(d.createElement('label'));
assert(() => nestedLabel instanceof window.HTMLLabelElement);
const ignoredSpan = nestedLabel.appendChild(d.createElement('span'));
const nestedButton = ignoredSpan.appendChild(d.createElement('button'));
const laterInput = nestedLabel.appendChild(d.createElement('input'));
assert(() => nestedButton == nestedLabel.control);
assert(() => nestedForm == nestedLabel.form);
nestedButton.setAttribute('form', 'external-form');
assert(() => externalForm == nestedLabel.form);
nestedButton.setAttribute('form', 'missing-form');
assert(() => null === nestedLabel.form);
nestedButton.removeAttribute('form');
nestedButton.remove();
assert(() => laterInput == nestedLabel.control);
assert(() => nestedForm == nestedLabel.form);
defaultInput.setAttribute('form', 'external-form');

textarea.setAttribute('form', 'missing-form');
assert(() => null === textarea.form);
assert(() => 2 == form.length);
assert(() => 1 == externalForm.length);
textarea.removeAttribute('form');
assert(() => textarea.form == form);
assert(() => 3 == form.length);
assert(() => 1 == externalForm.length);
textarea.value = 'hello';
assert(() => 'hello' == textarea.value);
assert(() => 'hello' == textarea.textContent);

const defaultTextarea = d.createElement('textarea');
assert(() => defaultTextarea instanceof window.HTMLTextAreaElement);
assert(() => '' == defaultTextarea.value);
assert(() => '' == defaultTextarea.defaultValue);
defaultTextarea.value = 123;
assert(() => '123' == defaultTextarea.value);
assert(() => '123' == defaultTextarea.defaultValue);
assert(() => '123' == defaultTextarea.textContent);
defaultTextarea.defaultValue = 456;
assert(() => '456' == defaultTextarea.value);
assert(() => '456' == defaultTextarea.defaultValue);
assert(() => '456' == defaultTextarea.textContent);

const image = d.createElement('img');
assert(() => image instanceof window.HTMLImageElement);
assert(() => '' == image.alt);
assert(() => '' == image.src);
image.src = 'https://example.com/image.png';
assert(() => 'https://example.com/image.png' == image.src);

const base = d.createElement('base');
assert(() => base instanceof window.HTMLBaseElement);
assert(() => '' == base.href);
assert(() => '' == base.target);
base.href = 'https://example.com/';
base.target = '_blank';
assert(() => 'https://example.com/' == base.href);
assert(() => '_blank' == base.target);
assert(() => 'https://example.com/' == base.getAttribute('href'));
assert(() => '_blank' == base.getAttribute('target'));

const link = d.createElement('link');
assert(() => link instanceof window.HTMLLinkElement);
assert(() => '' == link.href);
assert(() => '' == link.rel);
link.href = 'https://example.com/style.css';
assert(() => 'https://example.com/style.css' == link.href);

const meta = d.createElement('meta');
assert(() => meta instanceof window.HTMLMetaElement);
assert(() => '' == meta.content);
assert(() => '' == meta.httpEquiv);
assert(() => '' == meta.media);
assert(() => '' == meta.name);
meta.content = 'width=device-width';
meta.httpEquiv = 'refresh';
meta.media = 'screen';
meta.name = 'viewport';
assert(() => 'width=device-width' == meta.content);
assert(() => 'refresh' == meta.httpEquiv);
assert(() => 'screen' == meta.media);
assert(() => 'viewport' == meta.name);
assert(() => 'refresh' == meta.getAttribute('http-equiv'));

const data = d.createElement('data');
assert(() => data instanceof window.HTMLDataElement);
assert(() => '' == data.value);
data.value = '42';
assert(() => '42' == data.value);
assert(() => '42' == data.getAttribute('value'));

const time = d.createElement('time');
assert(() => time instanceof window.HTMLTimeElement);
assert(() => '' == time.dateTime);
time.dateTime = '2026-07-21';
assert(() => '2026-07-21' == time.dateTime);
assert(() => '2026-07-21' == time.getAttribute('datetime'));

const li = d.createElement('li');
assert(() => li instanceof window.HTMLLIElement);
assert(() => 0 == li.value);
li.value = 7;
assert(() => 7 == li.value);
assert(() => '7' == li.getAttribute('value'));

const ol = d.createElement('ol');
assert(() => ol instanceof window.HTMLOListElement);
assert(() => false === ol.reversed);
assert(() => 1 == ol.start);
assert(() => '' == ol.type);
ol.reversed = true;
ol.start = 3;
ol.type = 'A';
assert(() => true === ol.reversed);
assert(() => 3 == ol.start);
assert(() => 'A' == ol.type);
assert(() => '' === ol.getAttribute('reversed'));
assert(() => '3' == ol.getAttribute('start'));
assert(() => 'A' == ol.getAttribute('type'));
ol.reversed = false;
assert(() => false === ol.reversed);
assert(() => null === ol.getAttribute('reversed'));

const select = d.createElement('select');
assert(() => select instanceof window.HTMLSelectElement);
assert(() => '' == select.name);
assert(() => false === select.disabled);
assert(() => false === select.multiple);
assert(() => false === select.required);
assert(() => 0 == select.length);
assert(() => 0 == select.options.length);
assert(() => -1 == select.selectedIndex);
assert(() => '' == select.value);
assert(() => 'select-one' == select.type);
select.name = 'choice';
select.disabled = true;
select.required = true;
select.size = 3;
assert(() => 'choice' == select.name);
assert(() => true === select.disabled);
assert(() => true === select.required);
assert(() => 3 == select.size);
assert(() => '' === select.getAttribute('disabled'));
select.multiple = true;
assert(() => true === select.multiple);
assert(() => 'select-multiple' == select.type);
assert(() => -1 == select.selectedIndex);
select.multiple = false;
assert(() => false === select.multiple);

const optA = d.createElement('option');
assert(() => optA instanceof window.HTMLOptionElement);
assert(() => -1 == optA.index);
assert(() => '' == optA.text);
assert(() => '' == optA.value);
assert(() => '' == optA.label);
assert(() => false === optA.disabled);
assert(() => false === optA.defaultSelected);
assert(() => false === optA.selected);
optA.value = 'a';
optA.id = 'option-a';
optA.label = 'Alpha';
assert(() => 'a' == optA.value);
assert(() => 'Alpha' == optA.label);
assert(() => 'a' == optA.getAttribute('value'));
assert(() => 'Alpha' == optA.getAttribute('label'));
const optB = d.createElement('option');
optB.text = 'Bee';
assert(() => 'Bee' == optB.text);
assert(() => 'Bee' == optB.value);
assert(() => 'Bee' == optB.label);

const dataList = d.createElement('datalist');
assert(() => dataList instanceof window.HTMLDataListElement);
assert(() => 0 == dataList.options.length);
const dataOptionA = dataList.appendChild(d.createElement('option'));
const dataOptionB = dataList.appendChild(d.createElement('option'));
dataOptionA.value = 'data-a';
dataOptionB.value = 'data-b';
assert(() => 2 == dataList.options.length);
assert(() => dataOptionA === dataList.options[0]);
assert(() => dataOptionB === dataList.options[1]);
dataOptionA.remove();
assert(() => 1 == dataList.options.length);
assert(() => dataOptionB === dataList.options[0]);

select.add(optA);
select.add(optB);
assert(() => 2 == select.length);
assert(() => 2 == select.options.length);
assert(() => optA === select.options[0]);
assert(() => optB === select.item(1));
assert(() => optA === select.namedItem('option-a'));
assert(() => null === select.item(99));
assert(() => 0 == optA.index);
assert(() => 1 == optB.index);

const optGroup = d.createElement('optgroup');
assert(() => optGroup instanceof window.HTMLOptGroupElement);
assert(() => '' == optGroup.label);
assert(() => false === optGroup.disabled);
optGroup.label = 'Group';
optGroup.disabled = true;
assert(() => 'Group' == optGroup.label);
assert(() => true === optGroup.disabled);
assert(() => '' === optGroup.getAttribute('disabled'));
const groupedOption = optGroup.appendChild(d.createElement('option'));
groupedOption.value = 'grouped';
select.add(optGroup);
assert(() => 3 == select.length);
assert(() => 3 == select.options.length);
assert(() => groupedOption === select.options[2]);
assert(() => 2 == groupedOption.index);
optGroup.disabled = false;
assert(() => false === optGroup.disabled);
assert(() => null === optGroup.getAttribute('disabled'));
assert(() => 0 == select.selectedIndex);
assert(() => 'a' == select.value);
assert(() => false === optA.selected);
optB.selected = true;
assert(() => false === optA.selected);
assert(() => true === optB.selected);
assert(() => true === optB.defaultSelected);
assert(() => 1 == select.selectedIndex);
assert(() => 'Bee' == select.value);
assert(() => optB === select.selectedOptions[0]);
optB.defaultSelected = false;
assert(() => false === optB.selected);
assert(() => false === optB.defaultSelected);
select.value = 'a';
assert(() => 0 == select.selectedIndex);
assert(() => 'a' == select.value);
assert(() => true === optA.selected);
optA.disabled = true;
assert(() => true === optA.disabled);
assert(() => '' === optA.getAttribute('disabled'));
select.remove(0);
assert(() => 2 == select.length);
assert(() => optB === select.options[0]);
select.length = 3;
assert(() => 3 == select.length);
select.length = 1;
assert(() => 1 == select.length);

const table = d.querySelector('table');
assert(() => table instanceof window.HTMLTableElement);

assert(() => table.tHead instanceof window.HTMLTableSectionElement);
assert(() => null === table.caption);
assert(() => null === table.tFoot);
assert(() => null !== table.tBodies);

const caption = table.createCaption();
assert(() => caption instanceof window.HTMLTableCaptionElement);
assert(() => caption === table.caption);
assert(() => caption === table.createCaption());
table.deleteCaption();
assert(() => null === table.caption);

const tfoot = table.createTFoot();
assert(() => tfoot instanceof window.HTMLTableSectionElement);
assert(() => tfoot === table.tFoot);
assert(() => tfoot === table.createTFoot());
table.deleteTFoot();
assert(() => null === table.tFoot);

const tbodyCount = table.tBodies.length;
const tbody = table.createTBody();
assert(() => tbody instanceof window.HTMLTableSectionElement);
assert(() => tbodyCount + 1 == table.tBodies.length);

const rows = table.rows;
assert(() => 3 == rows.length);

const appendedRow = table.insertRow();
assert(() => appendedRow instanceof window.HTMLTableRowElement);
assert(() => 4 == rows.length);
assert(() => appendedRow === rows[3]);
table.deleteRow(-1);
assert(() => 3 == rows.length);

const firstRow = rows[0];
const insertedRow = table.insertRow(0);
assert(() => insertedRow instanceof window.HTMLTableRowElement);
assert(() => insertedRow === rows[0]);
assert(() => firstRow === rows[1]);
table.deleteRow(0);
assert(() => firstRow === rows[0]);
assert(() => throws(() => table.insertRow(999)));
assert(() => throws(() => table.deleteRow(999)));
assert(() => rows[0] instanceof window.HTMLTableRowElement);
assert(() => rows[0].parentElement instanceof window.HTMLTableSectionElement);
assert(() => rows[0] == rows[0].parentElement.rows[0]);

const firstSection = rows[0].parentElement;
const sectionRows = firstSection.rows;
const sectionRowCount = sectionRows.length;
const sectionFirstRow = sectionRows[0];
const sectionInsertedRow = firstSection.insertRow(0);
assert(() => sectionInsertedRow instanceof window.HTMLTableRowElement);
assert(() => sectionInsertedRow === sectionRows[0]);
assert(() => sectionFirstRow === sectionRows[1]);
assert(() => sectionInsertedRow === rows[0]);
firstSection.deleteRow(0);
assert(() => sectionFirstRow === sectionRows[0]);
const sectionAppendedRow = firstSection.insertRow();
assert(() => sectionAppendedRow === sectionRows[sectionRowCount]);
firstSection.deleteRow(-1);
assert(() => sectionRowCount == sectionRows.length);
assert(() => throws(() => firstSection.insertRow(999)));
assert(() => throws(() => firstSection.deleteRow(999)));

assert(() => 2 == rows[0].cells.length);
assert(() => rows[0].cells[0] instanceof window.HTMLTableCellElement);
assert(() => 'TH' == rows[0].cells[0].tagName);
assert(() => 0 == rows[0].cells[0].cellIndex);
assert(() => 1 == rows[0].cells[0].colSpan);
assert(() => 1 == rows[0].cells[0].rowSpan);
rows[0].cells[0].abbr = 'English';
rows[0].cells[0].headers = 'h1 h2';
rows[0].cells[0].scope = 'col';
rows[0].cells[0].colSpan = 2;
rows[0].cells[0].rowSpan = 3;
assert(() => 'English' == rows[0].cells[0].abbr);
assert(() => 'h1 h2' == rows[0].cells[0].headers);
assert(() => 'col' == rows[0].cells[0].scope);
assert(() => 2 == rows[0].cells[0].colSpan);
assert(() => 3 == rows[0].cells[0].rowSpan);
assert(() => '2' == rows[0].cells[0].getAttribute('colspan'));
assert(() => '3' == rows[0].cells[0].getAttribute('rowspan'));
assert(() => rows[0].cells[1] instanceof window.HTMLTableCellElement);
assert(() => 1 == rows[0].cells[1].cellIndex);
assert(() => 0 == rows[0].rowIndex);
assert(() => 0 == rows[0].sectionRowIndex);
assert(() => rows[1] instanceof window.HTMLTableRowElement);
assert(() => rows[1].cells[0] instanceof window.HTMLTableCellElement);
assert(() => 0 == rows[1].cells[0].cellIndex);
assert(() => rows[1].cells[1] instanceof window.HTMLTableCellElement);
assert(() => 1 == rows[1].cells[1].cellIndex);

const insertedCell = rows[1].insertCell(1);
assert(() => insertedCell instanceof window.HTMLTableCellElement);
assert(() => 3 == rows[1].cells.length);
assert(() => insertedCell === rows[1].cells[1]);
assert(() => 2 == rows[1].cells[2].cellIndex);
rows[1].deleteCell(1);
assert(() => 2 == rows[1].cells.length);
const appendedCell = rows[1].insertCell();
assert(() => appendedCell === rows[1].cells[2]);
rows[1].deleteCell(-1);
assert(() => 2 == rows[1].cells.length);
assert(() => throws(() => rows[1].insertCell(999)));
assert(() => throws(() => rows[1].deleteCell(999)));

assert(() => 1 == rows[1].rowIndex);
assert(() => 0 == rows[1].sectionRowIndex);
assert(() => -1 == d.createElement('td').cellIndex);
assert(() => -1 == d.createElement('tr').rowIndex);
assert(() => -1 == d.createElement('tr').sectionRowIndex);

[...table.querySelectorAll('tr')].at(-1).remove();
assert(() => 2 == rows.length);

const remainingSubclassCases = [
    ['area',       window.HTMLAreaElement],
    ['audio',      window.HTMLAudioElement],
    ['body',       window.HTMLBodyElement],
    ['br',         window.HTMLBRElement],
    ['canvas',     window.HTMLCanvasElement],
    ['details',    window.HTMLDetailsElement],
    ['dialog',     window.HTMLDialogElement],
    ['div',        window.HTMLDivElement],
    ['embed',      window.HTMLEmbedElement],
    ['h1',         window.HTMLHeadingElement],
    ['h2',         window.HTMLHeadingElement],
    ['h3',         window.HTMLHeadingElement],
    ['h4',         window.HTMLHeadingElement],
    ['h5',         window.HTMLHeadingElement],
    ['h6',         window.HTMLHeadingElement],
    ['html',       window.HTMLHtmlElement],
    ['hr',         window.HTMLHRElement],
    ['iframe',     window.HTMLIFrameElement],
    ['legend',     window.HTMLLegendElement],
    ['map',        window.HTMLMapElement],
    ['menu',       window.HTMLMenuElement],
    ['meter',      window.HTMLMeterElement],
    ['del',        window.HTMLModElement],
    ['ins',        window.HTMLModElement],
    ['object',     window.HTMLObjectElement],
    ['output',     window.HTMLOutputElement],
    ['p',          window.HTMLParagraphElement],
    ['param',      window.HTMLParamElement],
    ['picture',    window.HTMLPictureElement],
    ['pre',        window.HTMLPreElement],
    ['progress',   window.HTMLProgressElement],
    ['blockquote', window.HTMLQuoteElement],
    ['q',          window.HTMLQuoteElement],
    ['script',     window.HTMLScriptElement],
    ['source',     window.HTMLSourceElement],
    ['span',       window.HTMLSpanElement],
    ['caption',    window.HTMLTableCaptionElement],
    ['col',        window.HTMLTableColElement],
    ['colgroup',   window.HTMLTableColElement],
    ['title',      window.HTMLTitleElement],
    ['track',      window.HTMLTrackElement],
    ['video',      window.HTMLVideoElement],
    ['ul',         window.HTMLUListElement]
];

for(const [tag, ctor] of remainingSubclassCases)
    assert(() => d.createElement(tag) instanceof ctor);

assert(() => d.createElement('audio') instanceof window.HTMLMediaElement);
assert(() => d.createElement('video') instanceof window.HTMLMediaElement);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(nodeMixinChildNodes)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Document } from 'noto:dom';

const d = Document.html();
const host = d.body.appendChild(d.createElement('div'));
const text = host.appendChild(d.createTextNode('text'));
const comment = host.appendChild(d.createComment('comment'));

text.before('before', d.createElement('b'));
assert(() => 'before' === host.childNodes[0].nodeValue);
assert(() => 'B' === host.childNodes[1].nodeName);
assert(() => text === host.childNodes[2]);

text.after(d.createElement('i'), 'after');
assert(() => 'I' === text.nextSibling.nodeName);
assert(() => 'after' === text.nextSibling.nextSibling.nodeValue);

comment.before(d.createElement('u'));
comment.after('tail');
assert(() => 'U' === comment.previousSibling.nodeName);
assert(() => 'tail' === comment.nextSibling.nodeValue);

comment.replaceWith('replacement');
assert(() => null === comment.parentNode);
assert(() => 'replacement' === host.childNodes[host.childNodes.length - 2].nodeValue);

text.remove();
assert(() => null === text.parentNode);
text.remove();

const detached = d.createTextNode('detached');
detached.replaceWith('ignored');
assert(() => null === detached.parentNode);

const d2 = Document.html();
const doctype = d2.doctype;
doctype.before(d2.createComment('before doctype'));
doctype.after(d2.createComment('after doctype'));
assert(() => 'before doctype' === doctype.previousSibling.nodeValue);
assert(() => 'after doctype' === doctype.nextSibling.nodeValue);

doctype.replaceWith(d2.createComment('replacement doctype'));
assert(() => null === doctype.parentNode);
assert(() => 'replacement doctype' === d2.firstChild.nextSibling.nodeValue);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
