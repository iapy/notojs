## Renderer extensions

Renderer extensions add custom notebook output types to **NotoJS**. They are split into two parts:

- a **server module** imported by notebook code, which creates typed output objects
- a **client renderer** loaded by the browser, which turns those objects into DOM nodes

The built-in renderer extensions are:

- `charts.js` — chart output powered by Apache ECharts
- `tables.js` — table output for arrays and objects

## `tables.js`

`tables.js` renders JavaScript objects and arrays as HTML tables.

Import it with:

```javascript
import table from 'tables.js';
```

### Arrays of objects

An array of objects becomes a table with one row per object. By default, column names are taken from the first object:

```javascript
import table from 'tables.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
]));
```

Pass a column list to choose column order:

```javascript
import table from 'tables.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
], ['score', 'name']));
```

Use `[key, title]` pairs to rename headers:

```javascript
import table from 'tables.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
], [['score', 'Score'], ['name', 'Name']]));
```

### Objects

A plain object becomes a two-column key/value table:

```javascript
import table from 'tables.js';
print(table({ name: 'Alice', score: 12 }));
```

A column list can choose and rename keys:

```javascript
import table from 'tables.js';
print(table({ name: 'Alice', score: 12 }, [
  ['name', 'Name'],
  ['score', 'Score']
]));
```

### Nested renderable values

If a table cell value is an object, the table renderer delegates it to the normal **NotoJS** renderer. This allows nested HTML, Markdown, images, charts, or other renderer objects inside table cells.

```javascript
import { markdown } from 'noto:core';
import table from 'tables.js';

print(table([
  { item: 'docs', note: markdown('**done**') }
]));
```

### Column widths and alignment

`table` uses the regex overload of `render()` from `noto:render`. It returns a callable proxy: accessing a valid view property returns a table function with fixed column widths and optional alignment:

```javascript
import table from 'tables.js';
const rows = [
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
];
print(table['30% 70%'](rows));
print(table['30%< 70%>'](rows));
print(table['25%| *'](rows));
```

View syntax:

- widths are percentages such as `30%`, or `*` for remaining space
- append `<` for left alignment
- append `>` for right alignment
- append `|` for centered alignment
- separate columns with spaces

Invalid view strings throw `TypeError`.

## `charts.js`

`charts.js` renders **Apache ECharts** charts. It exports:

- `chart(type, options)` — helper for common area, bar, and line charts
- `echart(options)` — direct **ECharts** option passthrough

Import it with:

```javascript
import { chart, echart } from 'charts.js';
```

### Common charts

`chart(type, options)` supports:

- `area`
- `bar`
- `line`

Example:

```javascript
import { chart } from 'charts.js';

const x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
print(chart('line', {
  x,
  y: x.map(x => x ** 2)
}));
```

Multiple series are passed as an array of arrays in `y`:

```javascript
import { chart } from 'charts.js';

print(chart('bar', {
  x: ['Q1', 'Q2', 'Q3'],
  y: [
    [10, 12, 14],
    [7, 9, 11]
  ],
  labels: ['Revenue', 'Cost']
}));
```

Area and bar/line options can use `stack`:

```javascript
import { chart } from 'charts.js';

print(chart('area', {
  x: ['Jan', 'Feb', 'Mar'],
  y: [
    [1, 2, 3],
    [2, 1, 4]
  ],
  labels: ['A', 'B'],
  stack: 'total'
}));
```

Formatters can be supplied as strings containing `{value}`:

```javascript
import { chart } from 'charts.js';

print(chart('line', {
  x: ['A', 'B'],
  y: [1000, 2000],
  format: { y: '${value}' }
}));
```

### Direct ECharts options

Use `echart(options)` when you need full ECharts control:

```javascript
import { echart } from 'charts.js';

print(echart({
  xAxis: { type: 'category', data: ['A', 'B', 'C'] },
  yAxis: { type: 'value' },
  series: [{ type: 'line', data: [5, 8, 6] }]
}));
```

Maps are supported by passing ECharts map registration data as `[name, data]` in `geo.map` or series `map`. The client registers the map before rendering.

### Aspect ratio

Both `chart` and `echart` use the regex overload of `render()` from `noto:render`. Access a `W/H` property on either callable proxy to set the chart aspect ratio:

```javascript
import { chart, echart } from 'charts.js';

print(chart['16/9']('line', {
  x: ['A', 'B', 'C'],
  y: [1, 2, 3]
}));

print(echart['1/1']({
  series: [{ type: 'pie', data: [{ value: 1, name: 'A' }] }]
}));
```

The default aspect ratio is `4/3`. Invalid aspect-ratio properties throw `TypeError`.

## How rendering works

The native `noto:render` module exports `render` as both its default and a named export. It creates renderer functions, not output records directly:

```javascript!noplay
import render from 'noto:render';
// Alternatively: import { render } from 'noto:render';

const example = render('notojs.Render/example.js', data => data);
const result = example({ message: 'Hello' });
```

Each call to the returned function invokes the callback and wraps its result in a content record:

- `data` — the callback's return value
- `type` — the renderer name passed to `render()`
- `sign` — an automatically generated eight-character lowercase hexadecimal checksum
- `view` — present only when called through a view property provided by the regex overload

Creating a renderer function does not execute the callback. Each successful invocation records the renderer name in the current execution context. When notebook output is serialized, **NotoJS** includes a `notojs.Render` part listing the renderer bundles required by that output.

In the browser, `notojs.js` sees the `notojs.Render` part, dynamically imports each renderer client bundle from:

```text
/notojs.Render/:name
```

and calls the client module's `register(handlers)` function with the renderer handler table.

The renderer client registers a handler for its output type:

```javascript!noplay
export function register(handlers) {
  handlers['notojs.Render/example.js'] = function(grid, part) {
    const target = grid.get('html stacked');
    target.textContent = JSON.stringify(part.data);
  };
}
```

After registration, **NotoJS** wraps the handler with `verify()`. A printed record with the matching `type` reaches the specialized handler only if its `sign` matches. Missing or invalid signatures fall back to the default JSON renderer, `Handlers['.obj']`.

### Content signatures

Signatures use non-cryptographic 32-bit FNV-1a:

- For string data, the checksum covers the UTF-8/WTF-8 bytes of `type + '\0' + data`.
- For non-string data, including objects and arrays, it covers **only the type**. Object properties and array elements are not hashed.
- Additional fields such as `view` are not included.

Changing string data without regenerating its signature causes verification to fail. Changing object or array contents does not invalidate the signature. Records remain ordinary mutable objects; the signature does not freeze them.

This is a content checksum, not authentication or sanitization. Anyone can recompute it, and a valid signature does not establish that HTML or extension data is safe to render.

### Loading renderer bundles

Renderer client bundles are served from the JavaScript module search path (`[engine].jspath`). For a renderer named `charts.js`, **NotoJS** looks for:

```text
{jspath}/charts.js/client.js
```

The browser loads it through:

```http
GET /notojs.Render/charts.js
```

Standalone HTML export and server-generated HTML responses embed the renderer client bundles used by the notebook output, so reports can render charts and tables without the live editor. Both paths also wrap the registered extension handlers with `verify()` before rendering output.

## Writing a renderer extension

A renderer extension directory contains:

```text
example.js/
  bundle.ini
  server.js
  client.js
```

### `server.js`

The server module defines renderer functions using `render(name, callback)`. `$THIS` is injected into both server and client bundles at build time and expands to the renderer type name, such as `"notojs.Render/example.js"`.

```javascript!noplay
import render from 'noto:render';

export const example = render($THIS, data => data);
```

The callback receives the arguments and `this` from the returned function's invocation. Return the renderer's data, not a `{type, data, sign}` record: the native wrapper creates that record and its signature.

Return JSON-serializable data synchronously. The wrapper does not await promises; an async callback's promise would itself become `data`. Prepare asynchronous data before calling the renderer.

Notebook code imports and prints the server function result:

```javascript
import { example } from 'example.js';
print(example({ message: 'Hello' }));
```

#### View properties: `render(name, regex, callback)`

Pass a `RegExp` as the second argument to enable view-specific calls without writing a proxy yourself:

```javascript!noplay
import render from 'noto:render';

export const example = render($THIS, /^(compact|wide)$/, data => data);
```

Notebook code can call the renderer with or without a view:

```javascript!noplay
import { example } from 'example.js';

print(example({ message: 'Default view' }));
print(example['compact']({ message: 'Compact view' }));
print(example['wide']({ message: 'Wide view' }));
```

- A direct call returns a signed record without `view`.
- Reading a matching property returns a function; it does not invoke the callback yet.
- Calling that function forwards its arguments to the callback and adds the property name as `view` on the resulting record. It calls the underlying renderer as a plain function, without forwarding the caller's `this`.
- Property matching uses `property.match(regex)`. Use `^` and `$` anchors if the entire property name must match.
- Unmatched string properties throw `TypeError` with `render: invalid view <property>`. Symbol properties throw `TypeError` with `render: expected a string`.

The proxy interprets property reads as view selection, including ordinary function properties such as `name` or `call`; they are not automatically forwarded to the target function.

The built-in tables use `/^(\d+%|\*)([<>|])?(?: (\d+%|\*)([<>|])?)*$/` for column layouts. Charts use `/^\d+\/\d+$/` for aspect ratios. Both use this native overload rather than defining their own proxy helpers.

### `client.js`

The client module exports `register(handlers)`. It receives the renderer handler table and must assign a function for `$THIS`.

```javascript!noplay
export function register(handlers) {
  handlers[$THIS] = function(grid, part) {
    const el = grid.get('html stacked');
    el.textContent = part.data.message;
  };
}
```

The handler receives:

- `grid` — output layout helper. Call `grid.get(classNames)` to allocate an output block.
- `part` — the printed record, including `part.type`, `part.data`, `part.sign`, and optional `part.view`.

The handler decides how to interpret `part.view` and which default to use when it is absent. Register the plain handler; the standard **NotoJS** loading and HTML-generation paths apply signature verification.

The class names passed to `grid.get()` become `nj-*` classes in the DOM. For example, `grid.get('html stacked')` creates a block with `nj-html nj-stacked` classes.

### `bundle.ini`

`bundle.ini` declares external sources needed while building the renderer:

```ini
[sources]
dependency.js = https://example.com/dependency.esm.js
```

The bundler fetches each source into its cache and makes it available to `server.js` or `client.js` by filename. The existing `charts.js` renderer uses this to fetch `echarts.min.js`.

## Building renderer extensions

The render build is defined in `lib/notojs/bundle/render/CMakeLists.txt`.

Each renderer is added with:

```cmake
render(charts)
render(tables)
```

For a renderer named `name`, the build expects:

```text
lib/notojs/bundle/render/name.js/bundle.ini
lib/notojs/bundle/render/name.js/server.js
lib/notojs/bundle/render/name.js/client.js
```

The `render(name)` macro invokes the **NotoJS** bundler:

```sh
bundler path/to/name.js \
  --cache path/to/cache \
  --target path/to/output/name.js \
  --esbuild path/to/esbuild
```

The bundler:

1. reads `bundle.ini`
2. downloads and caches `[sources]`
3. bundles `client.js` with esbuild into `{target}/client.js`
4. bundles `server.js` with esbuild into `{target}/server.js`, leaving `noto:render` as an external import resolved by the runtime
5. injects `$THIS` as `"notojs.Render/name.js"` in both bundles, where `name.js` is the target directory name

To add a new renderer to the project, create the renderer directory and add it to `CMakeLists.txt`:

```cmake
render(example)
```

Then build the render targets:

```sh
cmake --build build --target example
```

or build all renderers:

```sh
cmake --build build --target charts tables
```

Configure **NotoJS** with `[engine].jspath` pointing at the directory containing the built renderer directories, so imports and browser renderer loading can find them:

```ini
[engine]
jspath = /path/to/build/lib/notojs/bundle/render
```

Then import the server bundle from notebook code:

```js!noplay
import { example } from 'example.js';
print(example({ message: 'Hello' }));
```


## Failure behavior

If the browser cannot load a renderer client bundle, **NotoJS** logs the failure and falls back to rendering the object as JSON. The notebook output still contains the original data, but the custom visual renderer is not available.

A missing or mismatched content signature also selects the default JSON renderer instead of invoking the custom handler.

Invalid arguments to `render()` throw `TypeError`. The three-argument overload requires a `RegExp` and a callable callback. Invalid view properties throw when accessed, before the callback runs.

Exceptions from a renderer callback or regex matching propagate unchanged to notebook code. They are not wrapped as content; if uncaught, the cell fails like any other JavaScript error.

###### See also
- `doc('topic:packages')`
- `doc('topic:ui')`
- `doc('print')`
