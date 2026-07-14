## NotoJS Configuration

NotoJS is configured with an `INI` file passed to the `notojs` executable.

A typical configuration looks like this:

```ini
[engine]
jspath = /path/to/js/modules
sopath = /path/to/native/modules

[global]
agent = curl/8.7.1
prefix = http://127.0.0.1:2310

[folder]
path = /path/to/workspace
lmdb = 250mb

[server]
bind = 127.0.0.1:2310
base = /
threads = 2
log = /path/to/notojs.log

[plugin]
path = /path/to/plugins

[module:doc]
suite = /path/to/libdoc.so
```

## Configuration sections

- `[engine]`
  - `jspath` — optional path used to resolve JavaScript modules.
  - `sopath` — optional path used to resolve native `.so` modules.
- `[global]`
  - `agent` — optional user-agent value used by HTTP requests.
  - `prefix` — optional URL prefix for relative `fetch` requests. If omitted, NotoJS uses `http://{server.bind}`.
- `[folder]`
  - `path` — required workspace directory. NotoJS creates `app` and `lib` directories inside it if they do not exist.
  - `lmdb` — optional LMDB map size, for example `250mb` or `2gb`. Default is `10mb`.
- `[server]`
  - `bind` — required `host:port` bind address.
  - `base` — optional base URL for the notebook window. Defaults to `/`.
  - `threads` — optional `HTTP,sync` thread configuration. The first integer is the number of threads used to process HTTP requests; the second integer is the number of threads used to schedule synchronous operations such as disk I/O. Defaults to `1,1`.
  - `script` — optional script URL inserted into the notebook window.
  - `log` — optional server log path.
- `[plugin]`
  - `path` — directory containing plugin shared libraries. Defaults to `/usr/local/lib/notojs/plugins`.
- `[plugin.NAME]`
  - Enables a plugin named `NAME`. NotoJS loads `{plugin.path}/NAME.so` and calls its `notojs_plugin` entry point.
- `[module:doc]`
  - `suite` — optional documentation suite shared library used by `noto:doc`.
- `[timers]`
  - Entries schedule `.notojs` notebooks to run periodically. Keys must use `Nm` format where `N` divides 60, for example `5m = cleanup.notojs`.

###### See also
- `doc('topic:api:cpp')`
- `doc('topic:api:http')`
- `doc('topic:lmdb')`
- `doc('topic:timers')`
