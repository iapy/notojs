# NotoJS

**NotoJS** is a lightweight JavaScript notebook environment designed for experimentation,
data visualization, automation, and embedded web applications.

Based on **[QuickJS](https://bellard.org/quickjs/)**, it provides modern JavaScript
features in a compact, embeddable runtime. With a memory footprint of only *a
few megabytes*, **NotoJS** brings full scripting capabilities even to
constrained devices such as the **Raspberry Pi Zero**.

Beyond its small size and efficiency, **NotoJS** provides:

- built-in modules, native **QuickJS** `.so` modules, and remote ES module imports over `http://` and `https://`
- browser-like APIs including `fetch`, `localStorage`, `Blob`, `URL`, and a server-side DOM capable of running libraries such as *svg.js* and *D3*
- persistent transactional key-value storage backed by an integrated **LMDB** environment
- a REST API for executing notebooks and managing workspace storage and package configuration
- a lightweight framework for building server-rendered applications directly in notebooks
- rich cell output including HTML, Markdown, images, SVG, charts, tables, and configurable layouts
- native server plugins and renderer extensions for integrating external services and custom output types
- a client library for embedding complete notebooks or individual cells in other web pages
- scheduled notebook execution with timers

Internally, **NotoJS** is written entirely in C++, using asynchronous `boost::beast` for
HTTP services and [RapidJSON](https://rapidjson.org/) for data serialization.

###### See also
- `doc('topic:api')`
- `doc('topic:applications')`
- `doc('topic:extensions')`
- `doc('topic:kernels')`
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
