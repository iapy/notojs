## XML selectors

For XML documents, NotoJS supports a small CSS-like selector subset. XML selectors are translated to XPath internally and evaluated with `pugixml`.

HTML documents use the **lexbor** CSS selector engine. The subset below describes XML documents created with `Document.xml(...)` or `DOMParser` using an XML MIME type.

### Supported selector forms

#### Element names

Select elements by tag name:

```js!noplay
const items = doc.querySelectorAll('item');
```

Supported tag names start with a letter and may contain letters, digits, `-`, and `_`:

```text
book
book-title
book_title
h1
```

Namespaced element names are supported in the form `prefix:name`:

```js!noplay
doc.querySelectorAll('svg:path');
```

Only one namespace prefix segment is supported.

#### Wildcard

Use `*` to match any element:

```js!noplay
doc.querySelectorAll('*');
```

#### Descendant combinator

A space selects descendants:

```js!noplay
doc.querySelectorAll('book title');
```

This matches `title` elements anywhere under a `book` element.

#### Child combinator

Use `>` to select direct children:

```js!noplay
doc.querySelectorAll('book > title');
```

Whitespace around `>` is optional:

```js!noplay
doc.querySelectorAll('book>title');
```

#### Selector lists

Use commas to combine selectors:

```js!noplay
doc.querySelectorAll('title, author, year');
```

Each selector is evaluated from the current node, and results are combined with XPath union semantics.

#### Attribute existence

Attributes can be matched by existence:

```js!noplay
doc.querySelectorAll('book[id]');
doc.querySelectorAll('*[href]');
```

Namespaced attributes are supported in the form `prefix:name`:

```js!noplay
doc.querySelectorAll('svg:path[xlink:href]');
```

#### Attribute value syntax

The XML selector parser accepts quoted attribute values:

```js!noplay
doc.querySelectorAll('book[type="fiction"]');
doc.querySelectorAll("book[type='fiction']");
```

#### Positional pseudo-classes

The following pseudo-classes are supported:

```text
:first-child
:last-child
:eq(N)
```

Examples:

```js!noplay
doc.querySelector('item:first-child');
doc.querySelector('item:last-child');
doc.querySelector('item:eq(0)');
```

`:eq(N)` is zero-based in selector syntax. It is translated to XPath's one-based positional predicate, so `:eq(0)` selects the first matched element at that step.

Pseudo-classes may be used before or after attribute selectors:

```js!noplay
doc.querySelectorAll('item:first-child[id]');
doc.querySelectorAll('item[id]:last-child');
```

###### Examples

```javascript
import { Document } from 'noto:dom';

const doc = Document.xml(`
<library>
  <book id="a" type="fiction">
    <title>The Name of the Rose</title>
    <author>Umberto Eco</author>
  </book>
  <book id="b" type="nonfiction">
    <title>Steppenwolf</title>
    <author>Herman Hesse</author>
  </book>
</library>
`);

print(doc.querySelector('book:first-child title').textContent);
print([...doc.querySelectorAll('book > title')].map(e => e.textContent));
print([...doc.querySelectorAll('title, author')].map(e => e.textContent));
print([...doc.querySelectorAll('book[id] > title')].map(e => e.textContent));
print(doc.querySelector('book[id="b"] > title').textContent);
```

### Translation model

Selectors are translated to XPath starting from the current node with `.//`.

| Selector | XPath shape |
| --- | --- |
| `book` | `.//book` |
| `book title` | `.//book//title` |
| `book > title` | `.//book/title` |
| `book[id]` | `.//book[@id]` |
| `book[id="a"]` | `.//book[@id="a"]` |
| `book:first-child` | `.//book[not(preceding-sibling::*)]` |
| `book:last-child` | `.//book[not(following-sibling::*)]` |
| `book:eq(0)` | `.//book[1]` |
| `title, author` | `.//title \| .//author` |

### Unsupported selector features

XML selectors intentionally support only the subset above. Unsupported features include:

- class selectors such as `.active`
- ID selectors such as `#main`
- pseudo-classes other than `:first-child`, `:last-child`, and `:eq(N)`
- pseudo-elements
- sibling combinators `+` and `~`
- `:not(...)`, `:has(...)`, `:is(...)`, and similar functional selectors

If an XML selector cannot be parsed, a `DOMException (SyntaxError)` is thrown.

###### See also
- `doc('topic:dom')`
