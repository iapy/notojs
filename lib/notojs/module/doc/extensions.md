## Renderer extensions

Renderer extensions add custom notebook output types to **NotoJS**. They are split into two parts:

- a **server module** imported by notebook code, which creates typed output objects
- a **client renderer** loaded by the browser, which turns those objects into DOM nodes

The built-in renderer extensions are:

- `chart.js` — chart output powered by Apache ECharts
- `table.js` — table output for arrays and objects

## `table.js`

`table.js` renders JavaScript objects and arrays as HTML tables.

Import it with:

```javascript
import table from 'table.js';
```

### Arrays of objects

An array of objects becomes a table with one row per object. By default, column names are taken from the first object:

```javascript
import table from 'table.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
]));
```

Pass a column list to choose column order:

```javascript
import table from 'table.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
], ['score', 'name']));
```

Use `[key, title]` pairs to rename headers:

```javascript
import table from 'table.js';
print(table([
  { name: 'Alice', score: 12 },
  { name: 'Bob', score: 10 }
], [['score', 'Score'], ['name', 'Name']]));
```

### Objects

A plain object becomes a two-column key/value table:

```javascript
import table from 'table.js';
print(table({ name: 'Alice', score: 12 }));
```

A column list can choose and rename keys:

```javascript
import table from 'table.js';
print(table({ name: 'Alice', score: 12 }, [
  ['name', 'Name'],
  ['score', 'Score']
]));
```

### Nested renderable values

If a table cell value is an object, the table renderer delegates it to the normal **NotoJS** renderer. This allows nested HTML, Markdown, images, charts, or other renderer objects inside table cells.

```javascript
import { markdown } from 'noto:core';
import table from 'table.js';

print(table([
  { item: 'docs', note: markdown('**done**') }
]));
```

### Column widths and alignment

`table` uses the regex overload of `render()` from `noto:render`. It returns a callable proxy: accessing a valid view property returns a table function with fixed column widths and optional alignment:

```javascript
import table from 'table.js';
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

## `chart.js`

`chart.js` renders **Apache ECharts** charts. It exports the callable `chart` renderer as both a named and default export:

```javascript
import chart from 'chart.js';
```

The callable accepts complete ECharts options directly and provides convenience methods for common chart types:

- `chart.area(data, options)`
- `chart.bar(data, options)`
- `chart.donut(data, options)`
- `chart.heatmap(data, options)`
- `chart.line(data, options)`
- `chart.pie(data, options)`
- `chart.scatter(data, options)`
- `chart(options)` — direct ECharts option passthrough

### XY charts

Area, bar, and line charts accept an object containing `x` and `y`. Both values may be any iterable. A single series uses an iterable of finite numbers:

```javascript
import chart from 'chart.js';

print(chart.line({
  x: [1, 2, 10],
  y: [5, 8, 12]
}));
```

For line and area charts, finite numeric X values with finite numeric series values produce continuous value axes and `[x, y]` data points. Numeric spacing is therefore preserved in the example above. Both value axes use automatic scaled bounds instead of being forced to include zero. ECharts may expand either axis beyond the data extent to produce regular tick intervals. Strings produce a category axis instead:

```javascript
import chart from 'chart.js';

print(chart.line({
  x: ['1', '2', '10'],
  y: [5, 8, 12]
}));
```

Set `continuous` to `false` to keep numeric X values as category labels:

```javascript
import chart from 'chart.js';

print(chart.line({
  x: [1, 2, 10],
  y: [5, 8, 12],
  continuous: false
}));
```

When omitted, `continuous` is detected automatically. Setting it to `true` requires all X and series values to be finite numbers; setting it to `false` forces category behavior. Converting numeric labels with `x.map(String)` also forces category behavior. Bar charts always treat X values as categories. Series containing advanced ECharts data-item objects remain categorical during automatic detection and are rejected when continuous behavior is explicitly required.

The X and Y collections must have the same length. Multiple series are represented by series descriptors whose `data` collections also match the X-axis length:

```javascript
import chart from 'chart.js';

const plot = chart.bar({
  x: ['Q1', 'Q2', 'Q3'],
  y: [
    {name: 'Revenue', data: [10, 12, 14]},
    {name: 'Cost', data: [7, 9, 11]}
  ]
});

plot.stacked = true;
print(plot);
```

`stacked` accepts only a boolean and applies to bar and area series. Reading it returns `true` when the chart has compatible series and all of them currently have a stack assignment.

### Heatmap charts

Heatmaps accept categorical `x` and `y` axes plus `data` points in `[xIndex, yIndex, value]` form. Axis values must be strings or finite numbers, indices must address their corresponding axes, and values must be finite numbers:

```javascript
import chart from 'chart.js';

print(chart.heatmap({
  x: ['Morning', 'Afternoon', 'Evening'],
  y: ['Monday', 'Tuesday'],
  data: [
    [0, 0, 5], [1, 0, 8], [2, 0, 3],
    [0, 1, 7], [1, 1, 2], [2, 1, 9]
  ]
}));
```

The visual scale defaults to the minimum and maximum values in the dataset and is hidden by default. Pass `{visualMap: {show: true}}` as the second argument to display it. Other ECharts options can be overridden the same way. Like other chart factories, heatmaps support aspect-ratio views such as `chart.heatmap['1/1'](data)`.

### Scatter, pie, and donut charts

Scatter charts accept one series descriptor or an iterable of descriptors:

```javascript
import chart from 'chart.js';

print(chart.scatter({
  name: 'Samples',
  data: [[1, 2], [2, 4], [3, 3]]
}));
```

A scatter series descriptor may set `size` to a fixed number, a `[width, height]` pair, or a string expression. `size` is translated to the ECharts `symbolSize` option. String expressions receive the point data as `value`; use `value[index]` to read a dimension:

```javascript
print(chart.scatter({
  name: 'Samples',
  data: [[1, 2, 6], [2, 4, 10], [3, 3, 14]],
  size: 'Math.max(8, value[2] * 2)'
}));
```

Unlike chart formatters, `size` strings are raw expressions and must not be enclosed in `{...}`. They support the same restricted expression operations, including arithmetic, comparisons, conditionals, and supported `Math` functions and constants. When passing complete ECharts options to `chart(options)`, use `symbolSize` instead of the `size` convenience alias; string `symbolSize` values use the same expression syntax.

Pie and donut charts accept a non-empty object with finite numeric values:

```javascript
import chart from 'chart.js';

print(chart.donut({
  Apples: 12,
  Oranges: 8,
  Pears: 5
}));
```

### Modifying a chart

Configure a chart before printing it. Its `series`, `xaxis`, and `yaxis` properties provide convenient access to the generated ECharts options:

```javascript
import chart from 'chart.js';

const plot = chart.line({
  x: ['Jan', 'Feb', 'Mar'],
  y: [1200, 1800, 1500]
});

plot.series[0].name = 'Revenue';
plot.series[0].smooth = true;
plot.xaxis.bounds = [0, 2];
plot.yaxis.bounds = [0, 2000];
plot.yaxis.format = '${value.toFixed(2)}';

print(plot);
```

On a category axis, numeric bounds are zero-based category indices, not numeric label values, and must be within the axis data range. A category axis already displays its complete data range by default. Continuous value axes interpret numeric bounds as coordinates.

Assigning `area` to a series type creates an ECharts line series with area styling. The wrapper exposes its logical type as `area`, while the serialized ECharts option remains `type: 'line'` with `areaStyle`:

```javascript
import chart from 'chart.js';

const plot = chart.line({
  x: ['Jan', 'Feb', 'Mar'],
  y: [1200, 1800, 1500]
});
plot.series[0].type = 'area';
console.log(plot.series[0].type); // "area"
print(plot);
```

Use `yaxis.add(series, position)` to move one or more series to an additional Y axis. Select a series by its integer index or unique name, or pass an array of selectors to move several series together. The position defaults to `right` for a Y axis and `top` for an X axis. Added axes disable ECharts' zero-crossing placement by default so the original and added axes remain on their configured sides; an explicit `axisLine.onZero` setting is preserved. The returned axis supports `format`, `bounds`, and a finite numeric `offset`:

```javascript
import chart from 'chart.js';

const plot = chart.line({
  x: ['Jan', 'Feb', 'Mar'],
  y: [
    {name: 'Revenue', data: [1200, 1800, 1500]},
    {name: 'Margin', data: [20, 25, 22]}
  ]
});

const marginAxis = plot.yaxis.add('Margin');
marginAxis.offset = 40; // Override the automatically calculated offset.
marginAxis.format = '{axis ? value.format("%.0f") : value.format("%.1f")}%';
print(plot);
```

Additional axes on the same side are offset automatically. Each additional Y axis is placed 30 pixels beyond the largest existing offset on that side; each additional X axis uses a 20-pixel increment. The first axis on an unused side has offset zero. Assign a finite number to the returned axis' `offset` to override the calculated value.

Set `plot.horizontal` to a boolean to switch an XY chart between vertical and horizontal orientation; reading it reports the current orientation. Axis collections, positions, and series axis assignments are translated together. After rotation, additional quantitative axes are accessed through `xaxis` instead of `yaxis`.

### Formatters

Formatter strings are parsed by the chart client without evaluating JavaScript. Supported expressions include:

- `{value}`
- `{axis}` — `true` for axis labels and pointers, `false` for tooltip values
- `{value.toFixed(n)}`, where `n` is between 0 and 100
- `{value.format("format")}` for date and number formatting

Expressions may use arithmetic (`+`, `-`, `*`, `/`, `%`, `**`), ordering comparisons (`<`, `>`, `<=`, `>=`), equality comparisons (`==`, `!=`), short-circuit `&&` and `||`, unary `+`, `-`, `!`, and `~`, and the conditional `test ? consequent : alternate` operator.

The formatter language's `==` and `!=` operators are non-coercing: they use strict equality internally, so `0 == "0"` is `false`. The JavaScript spellings `===` and `!==` are not part of the formatter language.

Own `Math` functions and numeric constants are available, along with the whitelisted number methods `toExponential`, `toFixed`, `toPrecision`, and `toString`. General property access and computed members are not supported. Examples include:

- `{value == null ? "N/A" : value.format("%.2f")}`
- `{Math.abs(value) < 0.01 ? "≈0" : value.format("%.2f")}`
- `{value >= 0 && value <= 100 ? value.format("%.1f") : "out of range"}`
- `{(value / Math.PI).format("%.3f")}`

Literal text may surround an expression:

```javascript
import chart from 'chart.js';

const plot = chart.line({
  x: ['2026-10-25', '2026-10-26', '2026-10-27'],
  y: [1200, 1800, 1500]
});
plot.yaxis.format = '&dollar;{axis ? value.format("%.0f") : value.format("%.2f")}';
plot.xaxis.format = '{axis ? value.format("MMM D") : value.format("YYYY-MM-DD")}';
print(plot);
```

HTML character references in literal text are decoded when the formatter is compiled. For example, `{value.format("%.1f")} &deg;C` produces text such as `21.5 °C`; numeric references such as `&#176;` and `&#xB0;` are also supported.

Formatter output remains plain text rather than trusted markup. When a formatted value is inserted into a custom HTML tooltip, HTML-sensitive characters in the axis value, series name, and complete formatter result are escaped. This allows character references such as `&deg;` while preventing formatter values or chart data from injecting HTML elements or event handlers.

A format beginning with `%` is interpreted as a numeric format. Supported conversions are `%f`/`%F` for fixed-point, `%e`/`%E` for exponential, `%g`/`%G` for significant digits, and `%d`/`%i` for integers. Formats may include `-`, `+`, space, and `0` flags, a field width, and a precision:

- `%.2f` — two digits after the decimal point
- `%+8.2f` — sign, width eight, and two fractional digits
- `%08.2f` — zero-padded width eight
- `%.3e` — exponential notation with three fractional digits
- `%.4g` — four significant digits

A format not beginning with `%` is interpreted as a date format. Supported tokens are:

- `YYYY` — four-digit year
- `MMM`, `MM`, `M` — three-letter, two-digit, or unpadded month
- `ddd` — three-letter weekday
- `DD`, `D` — two-digit or unpadded day
- `HH`, `H` — two-digit or unpadded 24-hour time
- `hh`, `h` — two-digit or unpadded 12-hour time
- `mm`, `m` — two-digit or unpadded minute
- `ss`, `s` — two-digit or unpadded second
- `SSS` — milliseconds
- `A` — `AM` or `PM`

Wrap literal date text in square brackets, for example `YYYY-MM-DD [at] HH:mm`. Weekday and month names use fixed English abbreviations; date formatting uses the browser's local timezone.

Assigning a string to `yaxis.format` formats both the axis and tooltip. An array supplies one tooltip formatter per series and does not change the axis labels:

```javascript
import chart from 'chart.js';

const plot = chart.line({
  x: ['Jan', 'Feb', 'Mar'],
  y: [
    {name: 'Revenue', data: [1200, 1800, 1500]},
    {name: 'Margin', data: [20, 25, 22]}
  ]
});
plot.yaxis.format = [
  '&dollar;{value.format("%.2f")}',
  '{value.format("%.1f")}%'
];
print(plot);
```

For charts without value axes, such as pie, donut, and heatmap charts, assign the tooltip formatter to `plot.format`. Heatmap formatting receives the cell value rather than either category-axis index:

```javascript
const plot = chart.heatmap({
  x: ['Morning', 'Evening'],
  y: ['Monday', 'Tuesday'],
  data: [[0, 0, 5.25], [1, 0, 8.5], [0, 1, 7.75], [1, 1, 2.5]]
});
plot.format = '{value.format("%.1f")}';
print(plot);
```

### Additional ECharts options

Pass an options object as the second argument to a convenience method to customize the generated ECharts configuration:

```javascript
import chart from 'chart.js';

print(chart.line({
  x: ['A', 'B', 'C'],
  y: [5, 8, 6]
}, {
  legend: {top: 0},
  xAxis: {axisLabel: {rotate: 30}}
}));
```

Charts use the shared notebook color palette exposed in the browser as `window.palette`. Set the ECharts `color` option to provide a chart-specific palette instead.

Use `chart(options)` when complete control over the ECharts configuration is required:

```javascript
import chart from 'chart.js';

print(chart({
  xAxis: {type: 'category', data: ['A', 'B', 'C']},
  yAxis: {type: 'value'},
  series: [{type: 'line', data: [5, 8, 6]}]
}));
```

Maps are supported by passing ECharts map registration data as `[name, data]` in `geo.map` or series `map`. The client registers the map before rendering.

### Aspect ratio

The direct renderer and each convenience method use the regex overload of `render()` from `noto:render`. Access a `W/H` property on the desired callable to set the chart aspect ratio:

```javascript
import chart from 'chart.js';

print(chart.line['4/3']({
  x: ['A', 'B', 'C'],
  y: [1, 2, 3]
}));

print(chart['1/1']({
  series: [{type: 'pie', data: [{value: 1, name: 'A'}]}]
}));
```

The default aspect ratio is `16/9`. Invalid aspect-ratio properties throw `TypeError`.

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

Renderer client bundles are served from the JavaScript module search path (`[engine].jspath`). For a renderer named `chart.js`, **NotoJS** looks for:

```text
{jspath}/chart.js/client.js
```

The browser loads it through:

```http
GET /notojs.Render/chart.js
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

The bundler fetches each source into its cache and makes it available to `server.js` or `client.js` by filename. The existing `chart.js` renderer uses this to fetch `echarts.min.js`.

## Bundling standalone scripts

Use the SDK macro to bundle a JavaScript file with a sibling `bundle.ini` (which may be empty when no external sources are needed):

```cmake
notojs_script(test.js)
```

This adds the `test` target to the default build, writes `test.js` into the current binary directory, and installs it into `lib/notojs/script`.

The equivalent direct bundler invocation is:

```sh
bundler path/to/test.js \
  --cache path/to/cache \
  --target path/to/build \
  --esbuild path/to/esbuild
```

This produces `path/to/build/test.js` as an ES module bundle with legal comments removed and whitespace minified. As with renderer builds, `[sources]` entries are downloaded or reused from the cache and linked into the working directory, where a temporary copy of the entry script is bundled. Run the command from the build directory to keep these generated files out of the source tree. An esbuild failure returns a nonzero exit status and removes the failed output.

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
