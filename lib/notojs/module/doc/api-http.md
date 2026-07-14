### HTTP API{text-decoration=underline}

**NotoJS** exposes both long `/api/v1/...` routes and short aliases for the same operations.

Unless otherwise noted, unsupported methods return **405 Method Not Allowed** and unknown routes return **404 Not Found**.

#### Notebook window

```http
GET /
```

Returns the main **NotoJS** notebook window as HTML.

```http
POST /
```

Executes the request body as JavaScript and returns the execution result as JSON.

If the request URL contains both `wid` and `pid` query parameters, execution is delegated to the matching window/kernel process when it exists:

```http
POST /?wid=:windowId&pid=:processId
```

Otherwise, the script is executed directly by the server runtime.

#### Applications

```http
/api/v1/app/:name/:path
/a/:name/:path
```

Routes a request to an application notebook stored as `app/:name.notojs`.

The router loads the application notebook, rewrites the request target to `/:path`, and executes the application code. The application is responsible for producing the final response.

If `app/:name.notojs` does not exist, **404 Not Found** is returned.

###### See also
- `doc('topic:applications')`

#### Folder API

The folder API reads and writes notebooks in the configured **NotoJS** workspace.

##### List workspace contents

```http
PROPFIND /api/v1/folder/
PROPFIND /f/
```

Returns workspace metadata as JSON:

```json
{
  "notebooks": [],
  "applications": [],
  "libraries": [],
  "packages": [],
  "databases": []
}
```

- `notebooks` lists top-level `.notojs` files without extensions.
- `applications` lists `app/*.notojs` files without extensions.
- `libraries` lists `lib/*.js` files without extensions.
- `packages` lists configured scripts and modules.
- `databases` lists storage databases and their entry counts.

##### Load a notebook

```http
GET /api/v1/folder/:path
GET /f/:path
```

Loads a `.notojs` notebook and returns it as JSON.

If the path is under `lib/`, **NotoJS** also reads the corresponding `.js` file and returns it as the first code cell.

Returns **404 Not Found** with `[]` if the file does not exist or the path is not a `.notojs` file.

##### Save a notebook

```http
PUT /api/v1/folder/:path
PUT /f/:path
```

Saves a `.notojs` notebook from a JSON request body.

The body is expected to be an array of cells. Cells with a string `code` property are written as JavaScript parts. If a cell also contains a string `body` property, it is written as the associated JSON result part.

For paths under `lib/`, the first cell's `code` is saved to the corresponding `.js` file.

Returns **200 OK** on success, **404 Not Found** for non-`.notojs` paths, and **500 Internal Server Error** on save errors.

##### Move or rename a notebook

```http
MOVE /api/v1/folder/:path
MOVE /f/:path
```

Moves or renames a `.notojs` notebook.

The request body contains the destination path.

For paths under `lib/`, **NotoJS** also moves or renames the corresponding `.js` file.

##### Delete a notebook

```http
DELETE /api/v1/folder/:path
DELETE /f/:path
```

Deletes a `.notojs` notebook.

For paths under `lib/`, **NotoJS** also deletes the corresponding `.js` file.

Returns **200 OK** when the file is removed, **404 Not Found** for non-`.notojs` paths, and **500 Internal Server Error** if removal fails.

### Result API

The result API loads or executes notebook contents.

##### Read notebook results

```http
GET /api/v1/result/:path
GET /r/:path
```

Reads a `.notojs` notebook and returns its stored cell results.

Response format depends on the `Accept` header:

- `application/json`, `*/*`, or no `Accept` header — returns JSON.
- `text/html` — returns rendered HTML.
- Any other value — returns **406 Not Acceptable**.

If the notebook does not exist, **404 Not Found** is returned with `[]`.

##### Execute a notebook

```http
POST /api/v1/result/:path
POST /r/:path
```

Executes the code cells from a `.notojs` notebook and returns the execution output.

Execution happens in the server process and does not fork or use a separate notebook kernel process.

The request body is exposed to the executed code as global `input`:

- With `Content-Type: application/json`, the body is parsed as JSON.
- Otherwise, the body is exposed as a string.

Use `Prefer: return=minimal` to suppress the response body.

##### Execute and save notebook results

```http
PUT /api/v1/result/:path
PUT /r/:path
```

Executes the notebook like `POST`, then saves the generated cell results back to the notebook path.

Like `POST`, execution happens in the server process and does not fork or use a separate notebook kernel process.

The same `Accept`, `Content-Type`, and `Prefer` behavior applies as for `POST`.

#### Storage API

The storage API persists key/value data in LMDB-backed namespaces. Long and short routes are equivalent.

Values have one of the following types in list responses:

- `type = 0` — stored as a string; `value` is returned as a string.
- `type = 1` — stored as JSON and parsed successfully; `value` is returned as JSON.
- `type = 2` — stored as JSON but not parseable; `value` is returned as a string.

##### List all storage namespaces

```http
GET /api/v1/storage/
GET /s/
```

Returns all storage entries as a JSON array. Each entry contains:

- `ns` — namespace name.
- `key` — entry key.
- `type` — stored value type.
- `value` — stored value.

Add `?var=:name` to receive JavaScript instead of JSON:

```http
GET /api/v1/storage/?var=storage
GET /s/?var=storage
```

The response body is wrapped as:

```javascript!noplay
var storage = [...];
```

##### List a namespace

```http
GET /api/v1/storage/:namespace
GET /s/:namespace
```

Returns all entries in `:namespace` as a JSON array. Each entry contains `key`, `type`, and `value`.

Add `?var=:name` to receive JavaScript instead of JSON.

If the namespace does not exist, **404 Not Found** is returned.

##### Read a key

```http
GET /api/v1/storage/:namespace/:key
GET /s/:namespace/:key
```

Returns the value for `:key` in `:namespace`.

- String values are returned as `text/plain`.
- JSON values are returned as `application/json`.
- JSON values with `?var=:name` are returned as `application/javascript` and wrapped in a variable assignment.

If the namespace or key does not exist, **404 Not Found** is returned.

##### Write a key

```http
PUT /api/v1/storage/:namespace/:key
PUT /s/:namespace/:key
```

Adds or updates `:key` in `:namespace` with the request body.

If `Content-Type` is exactly `application/json`, the body is stored as JSON. Otherwise, it is stored as a string.

Invalid JSON returns **400 Bad Request** with the RapidJSON parse error text. Storage errors return **500 Internal Server Error**.

##### Delete a namespace

```http
DELETE /api/v1/storage/:namespace
DELETE /s/:namespace
```

Deletes the entire namespace.

##### Delete a key

```http
DELETE /api/v1/storage/:namespace/:key
DELETE /s/:namespace/:key
```

Deletes `:key` from `:namespace`.

#### Kernel API

```http
GET /api/v1/kernel/:windowId
GET /k/:windowId
```

Creates or attaches a kernel process for a window and returns the process id as plain text.

Returns **404 Not Found** if the window does not exist, and **500 Internal Server Error** if the kernel cannot be created.

```http
PUT /api/v1/kernel/:windowId
PUT /k/:windowId
```

Sends the request body to the window's kernel execution handler.

```http
DELETE /api/v1/kernel/:windowId
DELETE /k/:windowId
```

Terminates the kernel process identified by the request body.

#### Window events

```http
GET /api/v1/window
GET /w
```

Opens a server-sent events stream for a **NotoJS** window.

Response headers include:

```http
Content-Type: text/event-stream
Cache-Control: no-cache
```

#### Keymap

```http
GET /api/v1/keymap
GET /m
```

Returns the bundled keymap as `text/plain`.

The response uses `ETag` and `Cache-Control: public, max-age=0, must-revalidate`. Requests with a matching `If-None-Match` return **304 Not Modified**.

#### Packages

##### Read package resolver configuration

```http
GET /api/v1/packages
GET /p
```

Returns the package resolver configuration as `text/plain`.

##### Update package resolver configuration

```http
PUT /api/v1/packages
PUT /p
```

Replaces the package resolver configuration with the request body and returns the accepted configuration as `text/plain`.

The configuration may contain `[scripts]` and `[modules]` sections. Package URLs must use `http` or `https`.

Validation errors return **500 Internal Server Error** with a plain-text error message.

#### Log rotation

```http
PUT /api/v1/log/rotate
```

Rotates the configured server log and returns the rotated log path as `text/plain; charset="utf-8"`.

If log rotation is not configured, **405 Method Not Allowed** is returned.
