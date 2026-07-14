## NotoJS Applications

A **NotoJS** application is a notebook stored in the workspace `app/` directory and served as an HTTP handler.

Applications let you build small web services, webhooks, and HTTP endpoints directly as notebooks.
They use the same JavaScript runtime and modules as normal notebooks, but application requests are executed inside the **NotoJS** server process. They do not fork or use separate notebook kernel processes.

The final application handler receives an HTTP request and writes an HTTP response. Cell outputs are discarded, and any errors are logged to the `sys:errorlog` database.

### Creating an application

In the UI, open the sidebar and use the **Applications** section to create or open an application notebook. Application notebooks are stored as:

```text
app/:name.notojs
```

For example, an application named `hello` is stored as:

```text
app/hello.notojs
```

The application is reachable through either route:

```http
/api/v1/app/hello/:path
/a/hello/:path
```

The short route is convenient for public URLs:

```http
GET /a/hello/world
```

### Request routing

When a request targets an application route, **NotoJS**:

1. extracts the application name from the first path segment
2. loads `app/:name.notojs`
3. rewrites the request path to the remaining path, preserving the query string
4. executes the application notebook in the server process
5. lets the application code produce the HTTP response

For example:

```http
GET /a/blog/posts/123?format=json
```

loads:

```text
app/blog.notojs
```

and exposes the request path to the application as:

```text
/posts/123?format=json
```

If the application notebook does not exist, **NotoJS** returns **404 Not Found**.

### Handler cell

Application notebooks start with a handler cell. In the editor, this cell is shown with a protected wrapper. You write the body of an async function; NotoJS wraps it as:

```js!noplay
$.fetch = async function(request) {
  const response = new ServerResponse();
  response.status = 405;

  // your handler code goes here

  return response;
};
```

The default response status is **405 Method Not Allowed**. Set the response body or status in your handler before returning it.

A minimal handler:

```js!noplay
response.status = 200;
response.body = 'Hello from NotoJS';
```

A JSON handler:

```js!noplay
response.status = 200;
response.body = {
  method: request.method,
  path: request.path
};
```

### Multiple cells

Application notebooks can contain more than the handler cell. For each application request, **NotoJS** executes all cells above the handler cell, followed by the handler cell itself. Cells below the handler cell are ignored and can be used to test the request handler directly in the application notebook.

### Example:

###### First cell:
```javascript!noplay
import { html } from 'noto:core';
export const page = title => html(`<!doctype html><h1>${title}</h1>`);
```

###### Handler cell:
```javascript!noplay
if("/" == request.path) {
  if("GET" == request.method) {
    response.status = 200;
    response.body = page('Hello');
  }
} else {
  response.status = 404;
}
```

###### Last cell:
```javascript!noplay
const r1 = await $.fetch(new Request("/"));
print(r1); // prints headers

const data = await r1.text();
print(data); // prints response body

const r2 = await $.fetch(new Request("/404"));
print(r2); // prints headers
```

## Request object

The handler receives a `ServerRequest` object as `request`.

Useful properties include:

- `request.method` — request method such as `GET`, `POST`, or `PUT`
- `request.path` — rewritten request path, including the query string
- `request.headers` — request headers as a `Headers` object

Request body helpers return promises:

```javascript!noplay
const text = await request.text();
const data = await request.json();
const bytes = await request.bytes();
const blob = await request.blob();
const form = await request.formData();
```

`formData()` parses URL-encoded form data as `URLSearchParams`. Multipart form bodies are not supported.

## Response object

Create `ServerResponse` from the handler.

```javascript!noplay
response.status = 200;
response.headers.set('Cache-Control', 'no-store');
response.body = 'OK';
```

Setting `response.body` chooses a content type for supported values:

- `String` — body is set without changing `Content-Type`
- `Blob` — uses the blob type
- `HTML` — `Content-Type: text/html`
- `SVG` — `Content-Type: image/svg+xml`
- `XML` — `Content-Type: text/xml`
- other objects — serialized as JSON with `Content-Type: application/json`

Example XML response:

```javascript!noplay
import { xml } from 'noto:core';
response.body = xml('<root/>);
```

## Caching and updates

Application notebooks are cached as compiled bytecode. If the notebook changes, NotoJS recompiles it the next time the application is requested.

Saving, renaming, or removing application notebooks through the workspace API updates the files under `app/` and affects future requests.

## Common patterns

###### Route by path

```javascript!noplay
if('/' === request.path) {
  if('GET' === request.method) {
    response.status = 200;
    response.body = 'home';
  }
} else if('/api/' === request.path) {
  if('GET' === request.method) {
    response.status = 200;
    response.body = { ok: true, path: request.path };
  }
} else {
  response.status = 404;
  response.body = 'Not found';
}
```

###### Accept JSON input

```javascript!noplay
if('/' == request.path) {
  if('POST' === request.method) {
    const data = await request.json();
    response.body = { received: data };
  }
} else {
  response.status = 404;
  response.body = 'Not found';
}
```

###### Redirect

```javascript!noplay
if('/' == request.path) {
  if('GET' === request.method) {
    response.status = 302;
    response.headers.set('Location', '/');
  }
} else {
  response.status = 404;
  response.body = 'Not found';
}
```

###### See also
- `doc('ServerRequest')`
- `doc('ServerResponse')`
- `doc('topic:api')`
- `doc('topic:ui')`
- `doc('topic:lmdb')`
