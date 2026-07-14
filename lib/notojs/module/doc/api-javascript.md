## JavaScript API{text-decoration=underline}

The **NotoJS** JavaScript API lets you include rendered notebook output in ordinary web pages outside the **NotoJS** editor.
A third-party HTML page can load `client.js`, point an element at a notebook, and let the **NotoJS** server execute or fetch the notebook result and render it into that element.

The client script expects to be served from the same bundle directory as:

- `client.js`
- `notojs.js`
- `notojs.css`

When `client.js` loads, it automatically loads `notojs.css`, imports `notojs.js`, and exposes a global `window.NotoJS` object.

###### See also
- `doc('topic:config')`

### Include notebook output

Add the client script to the page and create one or more `div[data-notojs]` elements:

```html
<script src="/notojs/client.js"></script>
<div data-notojs="report"></div>
```

For each `div[data-notojs]`, the client fetches:

```text
/notojs/r/report.notojs
```

and renders the returned notebook output into the `div`.

### Execute with text input

If a `div[data-notojs]` contains text and has no child elements, that text is sent to the notebook with a `POST` request.
The text becomes the request body and can be used by the notebook as its input data.

```html
<script src="/notojs/client.js"></script>
<div data-notojs="summarize">
Text to process in the notebook.
</div>
```

In this example, the client posts the text to:

```text
/notojs/r/summarize.notojs
```

After rendering succeeds, the original text node is replaced with the notebook output.

### Run code when rendering is ready

Use `data-notojs-onready` to run JavaScript after automatic rendering completes. The code runs with `this` set to the rendered element.

```html
<div
  data-notojs="report"
  data-notojs-onready="console.log('rendered', this)"
></div>
```

### Center and constrain output width

If the page contains an element with `data-notojs-style`, `client.js` injects a small helper style:

```css
div[data-notojs-style] {
  margin: auto;
  max-width: var(--nj-max-editor-width);
}
```

Example:

```html
<div data-notojs="report" data-notojs-style></div>
```

### Render programmatically

After `client.js` loads, use `NotoJS.render(element, notebook, config)` to render output yourself.

```html
<script src="/notojs/client.js"></script>
<div id="target"></div>

<script>
  NotoJS.render(document.getElementById('target'), 'report');
</script>
```

The method returns a `Promise` resolving to the rendered element:

```js!noplay
NotoJS.render(document.getElementById('target'), 'report')
  .then(el => console.log('rendered', el));
```

#### `NotoJS.render(element, notebook, config)`

Arguments:

- `element` — DOM element that receives rendered notebook output.
- `notebook` — notebook path without the `.notojs` suffix.
- `config` — optional object controlling execution and rendering.

Supported `config` fields:

- `text` — sends a `POST` request with this string as the request body.
- `json` — sends a `POST` request with this value serialized as JSON and `Content-Type: application/json`.
- `grid` — optional rendering grid object. Use this to customize where renderer output is inserted instead of using the default NotoJS output grid. The object may define `container(type)`, where `type` is the renderer type string, such as `notojs.Render/charts.js`. Return a DOM element to receive that renderer's output.

Examples:

```js!noplay
await NotoJS.render(target, 'report');
```

```js!noplay
await NotoJS.render(target, 'summarize', {
  text: 'Text to process'
});
```

```js!noplay
await NotoJS.render(target, 'chart', {
  json: {
    title: 'Sales',
    values: [10, 20, 30]
  }
});
```

Custom renderer placement with `grid.container(type)`:

```js!noplay
NotoJS.render(document.querySelector('#Charts'), 'Charts', {
  json: { max: 20 },
  grid: new class {
    container(type) {
      if (type === 'notojs.Render/charts.js') {
        return document
          .querySelector('#Charts')
          .appendChild(document.createElement('div'));
      }
    }
  }
}).then(e => e.style.opacity = 1);
```

In this example, chart renderer output is placed into a newly created `div` inside `#Charts`.
If `container(type)` does not return an element for a renderer type, **NotoJS** falls back to its normal behavior.

### Server and browser requirements

The page must be able to reach the NotoJS server that serves `:base/client.js` and the `:base/r/:path` result endpoint.
If the page is hosted on a different origin, configure the surrounding web server and browser security policy accordingly.

Notebook output may include renderer extensions. The client renderer loads those extension bundles from the same **NotoJS** base URL, so the page must allow those module imports as well.

###### See also
- `doc('topic:api')`
- `doc('topic:extensions')`
- `doc('topic:ui')`
