## Renderer extensions

Renderer extensions add custom notebook output types to **NotoJS**. They are split into two parts:

- a **server module** imported by notebook code, which creates typed output objects
- a **client renderer** loaded by the browser, which turns those objects into DOM nodes

The built-in renderer extensions are:

- `charts.js` — chart output powered by Apache ECharts
- `tables.js` — table output for arrays and objects

### `tables.js`

`tables.js` renders JavaScript objects and arrays as HTML tables.

Import it with:

```javascript
import table from 'tables.js';
```

#### Arrays of objects

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

`table` is a proxy. Accessing a valid view property returns a table function with fixed column widths and optional alignment:

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

Both `chart` and `echart` are proxies. Access a `W/H` property to set the chart aspect ratio:

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

### How rendering works

A server extension creates objects with a custom `type`:

```javascript!noplay
{
  type: 'notojs.Render/charts.js',
  data: { /* renderer data */ }
}
```

The type is generated with:

```javascript!noplay
$.__renderer('notojs.Render/charts.js')
```

Calling `$.__renderer(name)` also records the renderer name in the current execution context. When notebook output is serialized, **NotoJS** includes a `notojs.Render` part listing the renderer bundles required by that output.

In the browser, `notojs.js` sees the `notojs.Render` part, dynamically imports each renderer client bundle from:

```text
/notojs.Render/:name
```

and calls:

```javascript!noplay
register(window.Handlers)
```

The renderer client registers a handler for its output type:

```javascript!noplay
export function register(handlers) {
  handlers['notojs.Render/example.js'] = function(grid, part) {
    const target = grid.get('html stacked');
    target.textContent = JSON.stringify(part.data);
  };
}
```

After registration, any printed object with the matching `type` is rendered by that handler.

### Loading renderer bundles

Renderer client bundles are served from the JavaScript module search path (`[engine].jspath`). For a renderer named `charts.js`, **NotoJS** looks for:

```text
{jspath}/charts.js/client.js
```

The browser loads it through:

```http
GET /notojs.Render/charts.js
```

Standalone HTML export also embeds any renderer client bundles used by the notebook output, so exported reports can render charts and tables without the live editor.

## Writing a renderer extension

A renderer extension directory contains:

```text
example.js/
  bundle.ini
  server.js
  client.js
```

### `server.js`

The server module exports functions used by notebooks. These functions return typed objects. `$RENDERER` is injected at build time and expands to the renderer type name.

```javascript!noplay
export function example(data) {
  return {
    type: $.__renderer($RENDERER),
    data
  };
}
```

Notebook code imports and prints the server function result:

```javascript
import { example } from 'example.js';
print(example({ message: 'Hello' }));
```

### `client.js`

The client module exports `register(handlers)`. It receives the global renderer handler table and must assign a function for `$RENDERER`.

```javascript!noplay
export function register(handlers) {
  handlers[$RENDERER] = function(grid, part) {
    const el = grid.get('html stacked');
    el.textContent = part.data.message;
  };
}
```

The handler receives:

- `grid` — output layout helper. Call `grid.get(classNames)` to allocate an output block.
- `part` — the printed object, including `part.type`, `part.data`, and any extra fields such as `part.view`.

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
3. bundles `client.js` with esbuild into `target/name.js/client.js`
4. bundles `server.js` with esbuild into `target/name.js/server.js`
5. injects `$RENDERER` as `"notojs.Render/name.js"` in both bundles

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

If the browser cannot load a renderer client bundle, ****NotoJS**** logs the failure and falls back to rendering the object as JSON. The notebook output still contains the original data, but the custom visual renderer is not available.

If the server module throws while creating the render object, the cell fails like any other JavaScript error.

###### See also
- `doc('topic:packages')`
- `doc('topic:ui')`
- `doc('print')`
