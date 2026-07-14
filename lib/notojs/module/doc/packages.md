## Packages and module resolution

**NotoJS** can import built-in modules, workspace libraries, local JavaScript files, native QuickJS `.so` modules, remote ES modules, and configured package aliases.

Package configuration is stored in the workspace LMDB database and can be edited from the UI or through the packages API. It is used by both static `import` resolution and `require(...)` script loading.

### Built-in modules

Built-in **NotoJS** modules use the `noto:` scheme:

```js!noplay
import { html, markdown } from 'noto:core';
import { Document } from 'noto:dom';
import * as fs from 'noto:fs';
import doc from 'noto:doc';
```

Available built-ins depend on how **NotoJS** was built. Common modules include:

- `noto:core`
- `noto:db`
- `noto:doc`
- `noto:dom`
- `noto:fs`
- `noto:gzip` when zlib support is enabled
- `noto:zip` when libzip support is enabled

### Workspace libraries

JavaScript libraries stored in the workspace `lib/` directory can be imported by filename:

```js!noplay
import { helper } from 'tools.js';
```

**NotoJS** first checks whether the import name ends with `.js`. If it does, the resolver looks for a matching library in the workspace. Library notebooks under `lib/` are saved together with a corresponding `.js` file, so they can be edited in the UI and imported by other notebooks.

### Engine search paths

The server configuration can provide additional module search paths:

```ini
[engine]
jspath = /path/to/js/modules
sopath = /path/to/native/modules
```

`jspath` is used for JavaScript modules imported by `.js` filename:

```js!noplay
import x from 'package.js';
```

If `{jspath}/package.js` exists, **NotoJS** loads it. If `{jspath}/package.js` is a directory, **NotoJS** treats it as a rendering extension and loads `{jspath}/package.js/server.js`. This will make **NotoJS** UI to load `{jspath}/package.js/client.js`.

`sopath` is used for native QuickJS modules imported with a `.so` suffix:

```js!noplay
import native from 'native-module.so';
```

**NotoJS** loads `{sopath}/native-module.so` with `dlopen` and calls its `js_init_module` entry point.

### Remote ES modules

ES modules can be imported directly from `http://` or `https://`:

```javascript
import { SVG } from 'https://cdn.jsdelivr.net/npm/@svgdotjs/svg.js/+esm';
```

Remote modules are fetched and compiled as JavaScript modules. `https://` modules are cached as bytecode in the current runtime after they are compiled. Configured `https://` package aliases can also use the persistent HTTP cache.

If a remote module cannot be fetched, returns a non-`200` status, uses an unsupported scheme, or fails to compile, import resolution fails with a module loading error.

### Package aliases

Package aliases are configured with a restricted INI format containing two sections:

```ini
[modules]
d3 = https://cdn.jsdelivr.net/npm/d3@7/+esm
svg = https://cdn.jsdelivr.net/npm/@svgdotjs/svg.js/+esm

[scripts]
moment = https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.30.1/moment.min.js
```

- `[modules]` entries are resolved by JavaScript `import`.
- `[scripts]` entries are resolved by `require(...)`.

After saving the configuration above, modules can be imported by alias:

```js!noplay
import * as d3 from 'd3';
import { SVG } from 'svg';
```

Scripts can be loaded by name:

```js!noplay
require('moment');
const m = moment();
```

### Editing package configuration

In the UI, open the sidebar and use **Packages | Edit**. The package editor accepts a restricted subset of INI used only for package configuration:

```ini
[modules]
name = https://example.com/module.js

[scripts]
name = https://example.com/script.js
```

This format is intentionally smaller than general INI:

- Only `[modules]` and `[scripts]` sections are accepted.
- Each section can appear at most once.
- Package names must match `[A-Za-z0-9_.@-]+`.
- Values must be valid `http` or `https` URLs.
- Duplicate package names are rejected, even across sections.
- Comments must start with `#` at the beginning of a line.
- Leading whitespaces are not accepted.
- Assignments must use exactly one space before and after `=`.

Running the package editor cell saves the package config, effectively executing the following code and restarting the kernel:

```js!noplay
require.config(`
[modules]
name = https://example.com/module.js
`);
```

If the configuration is invalid, **NotoJS** reports an error with the failing line number.

You can also add more cells below the package editor cell to test package configuration changes.

### Resolution order for `import`

When a module is imported, **NotoJS** resolves it roughly in this order:

1. Native `.so` modules through `[engine].sopath`.
2. Direct `http://` or `https://` module URLs.
3. Built-in `noto:` modules and plugin modules.
4. Workspace libraries and `.js` files through `lib/` or `[engine].jspath`.
5. Package aliases from `[modules]`.

If no resolver can load the module, import fails with a reference or module resolution error.

### Caching

**NotoJS** uses two levels of caching:

- compiled bytecode in the current runtime for modules and scripts already loaded
- persistent HTTP response data for configured `https://` package URLs

When package configuration changes, **NotoJS** removes persistent cached `https://` entries for URLs that are no longer referenced by the configuration. Existing runtime bytecode remains available only for the lifetime of the current runtime context.

###### See also
- `doc('topic:config')`
- `doc('topic:extensions')`
- `doc('topic:ui')`
- `doc('require.config')`
- `doc('require.script')`
