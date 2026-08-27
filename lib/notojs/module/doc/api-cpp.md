### C++ SDK{text-decoration=underline}

The public headers are in the repository's `sdk/` directory. Native modules and plugins must be built against compatible **NotoJS** and **QuickJS** ABIs.

#### Public headers

- `bridge.hpp` — typed wrappers and dispatch for `JSValue` APIs.
- `global.hpp` and `module.hpp` — wrappers for selected **NotoJS** JavaScript APIs.
- `notodb.hpp` — access to the server's LMDB environment.
- `engine.hpp` — asynchronous native tasks exposed as promises.
- `plugin.hpp` — server plugin interfaces.
- `plugin.cmake` — helper for building `.so` plugins.

###### See also
- `doc('global.hpp')`
- `doc('module.hpp')`

#### JavaScript ↔ C++ bridge

Bridge wrappers validate and convert JavaScript arguments before invoking C++ code. Common types include

- `bridge::ArrayBuffer`
- `bridge::Array`
- `bridge::Lambda`
- `bridge::Number`
- `bridge::Object`
- `bridge::Promise`
- `bridge::String`

```cpp
#include <bridge.hpp>

JSValue twice(JSContext *ctx, bridge::Number value)
{
    return bridge::Number(ctx, 2 * static_cast<std::int64_t>(value));
}

JSCFunctionListEntry const funcs[] = {
    JS_CFUNC_DEF("twice", 1, &bridge::Function<&twice>::invoke)
};
```

`bridge::Function<&fn>::invoke` rejects incompatible arguments with `TypeError`. Multiple function pointers may be supplied to `bridge::Function` to define overloads.

##### Wrapping C++ classes

Derive from `bridge::Interface<Interface, Wrapped>` to expose a C++ value as a JavaScript class. Use `ref()` to access the wrapped value and define JavaScript methods in `funcs`:

```cpp
struct Counter : bridge::Interface<Counter, int>
{
    Counter() : Base{} { ref() = 0; }

    JSValue increment(JSContext *ctx)
    {
        return bridge::Number(ctx, ++ref());
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Counter::funcs[] = {
    JS_CFUNC_DEF("increment", 0,
        &bridge::Function<&Counter::increment>::invoke)
};
```

The optional third template argument defines inheritance:

```cpp
struct NodeData {};
struct ElementData : NodeData {};

struct Node : bridge::Interface<Node, NodeData> {};
struct Element : bridge::Interface<Element, ElementData, Node> {};
```

This makes the JavaScript `Element` prototype inherit from `Node` and lets bridge arguments expecting `Node` accept an `Element`. The wrapped type of a derived interface must be the same as, or publicly derive from, the wrapped type of each base interface; otherwise bridge upcasting is invalid.

A native QuickJS module exports a `js_init_module` entry point:

```cpp
int init(JSContext *ctx, JSModuleDef *mod)
{
    Counter::init(ctx, mod);
    return JS_SetModuleExportList(ctx, mod, funcs, 1);
}

extern "C" JSModuleDef *js_init_module(JSContext *ctx, char const *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return nullptr;

    JS_AddModuleExport(ctx, mod, Counter::name());
    JS_AddModuleExportList(ctx, mod, funcs, 1);
    return mod;
}
```

Place the resulting `.so` file in `[engine].sopath` and import it by filename.

#### LMDB

`notodb.hpp` exposes the LMDB environment already owned by **NotoJS**:

```cpp
notojs::DB db{ctx};
auto [txn, handle] = db.open(
    notojs::DB::RW,
    notojs::DB::VAR,
    "myplugin:data"
);
```

Use `DB::SYS` for internal databases, `DB::USR` for user data, and `DB::VAR` for plugin or application data. Plugins should use unique names in `DB::VAR`. A plugin can construct `notojs::DB` from `host.lmdb()`.

#### Asynchronous tasks

Derive from `notojs::Task` to return a JavaScript promise from native code:

- `step()` performs background work and returns `Again` or `Finish`.
- `then(JSContext *, JSValue &)` creates the result and returns `Resolve` or `Reject`.
- `run(ctx)` starts the task and returns its promise.

Do not use `JSContext *` or `JSValue` from `step()`; create JavaScript values in `then()`.

#### Server plugins

Plugins are `.so` libraries loaded by the server process. Enable a plugin with a search path and a matching configuration section:

```ini
[plugin]
path = /usr/local/lib/notojs/plugin

[plugin:demo]
book = demo.notojs
```

The server loads `demo.so` and calls:

```cpp
extern "C" notojs::IPlugin *notojs_plugin(
    boost::property_tree::ptree const &config
);
```

Minimal plugin:

```cpp
#include <plugin.hpp>

class Demo : public notojs::IPlugin
{
public:
    explicit Demo(boost::property_tree::ptree const &) {}

    bool run(notojs::IHost &host) override
    {
        host.load("demo.notojs");
        return true;
    }

    void end(notojs::IHost &) override {}
};

extern "C" notojs::IPlugin *notojs_plugin(
    boost::property_tree::ptree const &config)
{
    return new Demo(config);
}
```

**NotoJS** owns the returned plugin. `run()` starts it; returning `false` disables it. `end()` stops plugin resources during server shutdown.

`IHost` provides:

- `clog(message)` writes a line to the server log.
- `clog(message, args)` appends string and integer arguments as a JSON array.
- `lmdb()` — the shared LMDB environment.
- `load(name)` — registers a notebook for cached execution.
- `exec(name, context)` — executes a registered notebook.

Override `IContext::input()` to provide the notebook's global `input` value and `output()` to inspect the context after successful execution. Override `IPlugin::mod(IHost &)` to expose an optional module as `noto:PLUGIN_NAME`.

Build a plugin with the SDK helper:

```cmake
cmake_minimum_required(VERSION 3.12)
project(myplugin LANGUAGES CXX)

include(sdk/plugin.cmake)
plugin(myplugin)
```

```sh
cmake -S . -B build
cmake --build build --target myplugin
```

Plugins execute inside the server process. Stop worker threads in `end()` and do not retain `JSContext *` or `JSValue` objects beyond callbacks unless their lifetimes are managed explicitly.

###### See also
- `doc('topic:config')`
- `doc('topic:lmdb')`
