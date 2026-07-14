# NotoJS

**NotoJS** is a lightweight JavaScript notebook environment designed for experimentation,
data visualization, automation, and embedded web applications.

Based on **[QuickJS](https://bellard.org/quickjs/)**, it provides modern JavaScript
features in a compact, embeddable runtime. With a memory footprint of only *a
few megabytes*, **NotoJS** brings full scripting capabilities even to
constrained devices such as the **Raspberry Pi Zero**.

Beyond its small size and efficiency, **NotoJS** provides:

- support for **QuickJS** native modules
- importing modules directly from `http://` and `https://`.
- familiar browser-style interfaces, including `localStorage`, `fetch` and a `DOM` for client-side libraries such as *svg.js* and *D3* can run server-side unchanged.
- a REST API for notebook execution, `localStorage` access, and package registry management
- a lightweight web framework for creating micro-applications directly within **NotoJS**
- robust notebook output with support for HTML, Markdown and layout manager
- integrated **LMDB** environment for persistent transactional key-value storage
- server-side plugins and renderer extensions defining custom rendering primitives
- client library allowing **NotoJS** notebooks or individual cells to be included in any web page
- periodic notebook execution

Internally, **NotoJS** is written entirely in C++, using asynchronous `boost::beast` for
HTTP services and [RapidJSON](https://rapidjson.org/) for data serialization.

###### See also
- `doc('topic:api')`
- `doc('topic:applications')`
- `doc('topic:extensions')`
- `doc('topic:lmdb')`
- `doc('topic:markdown')`
- `doc('topic:packages')`
- `doc('topic:timers')`
- `doc('topic:ui')`
- `doc('print')`

## Running NotoJS

Start **NotoJS** with a configuration file:

```sh
notojs /path/to/notojs.ini
```
###### See also
- `doc('topic:config')`
