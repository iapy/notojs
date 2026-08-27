## Packages and module resolution

**NotoJS** can load built-in modules, workspace libraries, JavaScript and native modules from configured search paths, direct remote ES modules, legacy scripts, and persistent package aliases.

Package aliases are stored in the workspace LMDB environment. They are shared by the server and notebook kernels and can be edited from the UI, through the `packages()` function from `noto:noto`, or through the Packages HTTP API.

### Built-in modules

Built-in **NotoJS** modules use the `noto:` scheme:

```js!noplay
import { assert } from 'noto:assert';
import { html, markdown } from 'noto:core';
import { Document } from 'noto:dom';
import * as fs from 'noto:fs';
import doc from 'noto:doc';
```

The built-in modules are:

- `noto:assert`
- `noto:core`
- `noto:db`
- `noto:doc`
- `noto:dom`
- `noto:fs`
- `noto:noto`

A server plugin may register another `noto:` module.

### Native modules

Native QuickJS modules use a `.so` suffix and are loaded from `[engine].sopath`:

```ini
[engine]
sopath = /usr/local/lib/notojs/stdlib
```

```js!noplay
import * as crypto from 'crypto.so';
import gzip from 'gzip.so';
import zip from 'zip.so';
```

**NotoJS** opens `{sopath}/NAME.so` with `dlopen` and calls its `js_init_module` entry point. `crypto.so` is part of the standard installation. `gzip.so` and `zip.so` are available when **NotoJS** was built with zlib and libzip respectively.

A `.so` import is treated as a native module name before package aliases are considered.

### Workspace libraries

JavaScript libraries stored in the workspace `lib/` directory can be imported by `.js` filename:

```js!noplay
import { helper } from 'tools.js';
```

Library notebooks under `lib/` are saved together with a corresponding `.js` file, allowing them to be edited in the UI and imported by other notebooks. Workspace libraries take precedence over files in `[engine].jspath` and package aliases.

### JavaScript module search path

The server configuration may provide an additional JavaScript module directory:

```ini
[engine]
jspath = /usr/local/lib/notojs/script
```

A `.js` import first looks for a regular file:

```text
{jspath}/package.js
```

If `{jspath}/package.js` is a directory, **NotoJS** treats it as a renderer extension and loads:

```text
{jspath}/package.js/server.js
```

The UI loads the corresponding `{jspath}/package.js/client.js` renderer bundle when notebook output reports that renderer.

### Direct remote ES modules

An ES module can be imported directly from an `http://` or `https://` URL:

```js!noplay
import { SVG } from 'https://cdn.jsdelivr.net/npm/@svgdotjs/svg.js/+esm';
```

The source is fetched and compiled as an ES module. Compiled `https://` bytecode is cached in memory for the lifetime of the kernel process. A direct URL import does not use the persistent package HTTP cache.

Import fails if the URL cannot be fetched, the response is not `200 OK`, the scheme is unsupported, or the source cannot be compiled as an ES module.

### Package aliases

Aliases use a restricted INI document with separate sections for ES modules and legacy scripts:

```ini
[modules]
d3 = https://cdn.jsdelivr.net/npm/d3@7/+esm
svg = https://cdn.jsdelivr.net/npm/@svgdotjs/svg.js/+esm

[scripts]
moment = https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.30.1/moment.min.js
```

`[modules]` entries are resolved by `import`:

```js!noplay
import * as d3 from 'd3';
import { SVG } from 'svg';
```

`[scripts]` entries are resolved by the global `require()` function:

```js!noplay
require('moment');
const now = moment();
```

Legacy scripts execute in global mode and may install variables or functions on the notebook global object. Use `require.script()` instead when selected values should be captured without leaving them on `globalThis`.

### Editing package configuration

In the UI, open **Packages | Edit**. The first cell contains the current package configuration. Running that cell calls `packages(config)` from `noto:noto`; after a successful update, the UI restarts the notebook kernel so subsequent imports use a fresh runtime and module cache.

The equivalent JavaScript API is:

```js!noplay
import { packages } from 'noto:noto';

const current = packages();

packages(`[modules]
d3 = https://cdn.jsdelivr.net/npm/d3@7/+esm

[scripts]
moment = https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.30.1/moment.min.js`);
```

Calling `packages()` without arguments returns the stored configuration. Calling it with a string validates and replaces the complete configuration. Programmatic calls update storage immediately but do not restart the current kernel automatically.

The same configuration can be read and replaced through the Packages HTTP API.

You can add ordinary cells below the package editor cell to test imports before leaving the editor.

### Configuration syntax

The package document accepts:

- `[modules]` and `[scripts]` section headers; each section may appear at most once.
- Empty lines.
- Full-line comments beginning with `#` in the first column.
- Assignments in the exact form `NAME = URL`.
- Names matching `[A-Za-z0-9_.@-]+`.
- Absolute `http://` or `https://` URLs.

Package names must be unique across both sections. Leading whitespace, inline comments, unsupported sections or schemes, duplicate sections or names, and other assignment spacing are rejected. Validation errors include the failing line number.

Saving a new document replaces the previous package configuration; omitted aliases are removed.

### Resolution order for `import`

For an import specifier, **NotoJS** resolves these forms in order:

1. Names ending in `.so` through `[engine].sopath`.
2. Direct URL imports, including built-in and plugin modules using the `noto:` scheme.
3. Names ending in `.js` from the workspace `lib/` directory.
4. Names ending in `.js` from `[engine].jspath`, including renderer-extension `server.js` files.
5. Aliases stored under `[modules]`.

If no resolver can load the specifier, import fails with a module resolution error.

### Resolution order for `require`

`require(name)` resolves:

1. Built-in globalThis extenders such as `console`, `crypto`, `dollar`, `dom`, and `mustache`.
2. Direct `http://` or `https://` script URLs.
3. Aliases stored under `[scripts]`.

Some built-in extenders accept a second configuration argument; for example, `require('storage', {ns: 'example'})` installs a namespaced `localStorage` object. A configured or remote legacy script executes in global mode.

`require.script(request, names[, context])` is a separate asynchronous helper. It fetches a legacy script and returns only the requested names in an object rather than using the package alias resolver.

### Caching and configuration changes

**NotoJS** uses two cache layers:

- compiled bytecode held in memory by the current kernel process;
- source responses for configured `https://` aliases stored in the workspace HTTP cache.

Direct `https://` imports and scripts use the in-memory bytecode cache but are not added to the persistent HTTP cache. Configured `https://` aliases use both layers. Configured `http://` aliases are not persistently cached.

When package configuration is replaced, persistent cached responses are removed for URLs no longer present in the new document. Already compiled bytecode remains in the current kernel process. The package editor therefore restarts its kernel after a successful save; code that calls `packages(config)` directly should restart the kernel when it needs to discard all previously compiled package code.

###### See also
- `doc('packages')`
- `doc('require')`
- `doc('require.script')`
- `doc('topic:api:http')`
- `doc('topic:config')`
- `doc('topic:extensions')`
- `doc('topic:kernels')`
- `doc('topic:ui')`
