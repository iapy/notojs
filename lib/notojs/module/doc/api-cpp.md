### C++ SDK{text-decoration=underline}

The public SDK is maintained in the repository's `sdk/` directory and installed under `lib/notojs/devkit`. Native modules and plugins must be built against compatible **NotoJS** and **QuickJS** ABIs.

#### Public headers

- `bridge.hpp` — typed wrappers and dispatch for `JSValue` APIs.
- `engine.hpp` — the `notojs::Task` base class for asynchronous native work exposed as promises.
- `global.hpp` — wrappers for selected global **NotoJS** JavaScript types.
- `module.hpp` — wrappers for selected built-in module types.
- `notodb.hpp` — access to the server's LMDB environment.
- `plugin.hpp` — server plugin, host, and execution-context interfaces.

#### CMake build helper

Include the installed `notojs.cmake` helper from the same devkit:

```cmake
include(${CMAKE_INSTALL_PREFIX}/lib/notojs/devkit/notojs.cmake)
```

It provides these helpers for native extensions:

- `notojs_plugin(name [LINK ...] [NOINSTALL])` builds `name.cpp` as `name.so` and installs it under `lib/notojs/plugin` unless `NOINSTALL` is specified.
- `notojs_module(name [LINK ...] [NOINSTALL])` builds `name.cpp` as `name.so` and installs it under `lib/notojs/stdlib` unless `NOINSTALL` is specified.
- `notojs_package(MAINTAINER name)` configures CPack using the current project name and version. Extension DEB and RPM packages receive an exact dependency on the **NotoJS** version that configured the installed devkit.

The `LINK` list accepts additional libraries or CMake targets. The helper assigns a stable private namespace to the extension target and locally built targets linked into it, preventing symbols from different extensions from colliding inside the server process.

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

Place the resulting `.so` file in `[engine].sopath` and import it by filename. The `notojs_module()` helper installs native modules to `lib/notojs/stdlib`, which is the default `[engine].sopath` in an installed **NotoJS** configuration.

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
prefix = demo-
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
#include <string>
#include <unordered_set>

class Demo : public notojs::IPlugin
{
public:
    explicit Demo(boost::property_tree::ptree const &config)
    : prefix{config.get<std::string>("prefix", "demo-")}
    {}

    bool run() override
    {
        return true;
    }

    void end() override {}

    bool updated(std::string const &name) override
    {
        if(name.size() < prefix.size() || name.compare(0, prefix.size(), prefix))
            return false;

        notebooks.insert(name);
        return true;
    }

    void removed(std::string const &name) override
    {
        notebooks.erase(name);
    }

private:
    std::string prefix;
    std::unordered_set<std::string> notebooks;
};

extern "C" notojs::IPlugin *notojs_plugin(
    boost::property_tree::ptree const &config)
{
    return new Demo(config);
}
```

**NotoJS** owns the returned plugin and assigns its protected `host` pointer before invoking plugin callbacks.

For every top-level `.notojs` notebook, `updated(name)` is called during startup and after each save. `name` is the extensionless logical notebook name. Return `true` to subscribe to that notebook. The callback should only update plugin state or scheduling metadata; it must not call `host->exec()` synchronously. When a subscribed notebook is deleted, `removed(name)` is called with the same extensionless name. `run()` starts the plugin after initial notebook discovery; returning `false` disables it. `end()` stops plugin resources during server shutdown.

`IHost` provides:

- `clog(message)` writes a line to the server log.
- `clog(message, args)` appends supported scalar arguments (`bool`, floating-point values, strings, and signed or unsigned integers) as a JSON array.
- `lmdb()` returns the shared LMDB environment.
- `exec(name, context)` executes a subscribed notebook and returns `true` only when compilation and execution succeed. Pass the extensionless logical notebook name; the executed notebook receives the same value as read-only `self.name` from the default `noto:noto` export.

`IContext` exchanges the notebook's `self.context` value around plugin-triggered execution:

- `set_context(JSContext *ctx, JSValue &value)` runs before execution. Set `value` to an owned `JSValue` created in `ctx`. The host installs and consumes that value; do not free it or use it after the callback returns. Leaving it as `JS_UNDEFINED` starts execution with no context value.
- `get_context(JSContext *ctx, JSValue value)` runs only after successful execution. `value` is borrowed and valid only for the duration of the callback. Do not free or retain it; copy any needed data into plugin-owned native storage before returning. The host releases the value afterward.

The `JSContext *` supplied to either callback belongs to that execution and must not be retained. Override `IPlugin::mod()` to expose an optional module as `noto:PLUGIN_NAME`.

Build a plugin from `demo.cpp` with the installed SDK helper:

```cmake
cmake_minimum_required(VERSION 3.18)
project(notojs-demo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_INSTALL_PREFIX}/lib/notojs/devkit/notojs.cmake)
find_package(Boost CONFIG REQUIRED)

notojs_plugin(demo)
notojs_package(MAINTAINER "Your Name")
```

Configure with the same installation prefix as **NotoJS**, then build and install:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target demo
cmake --install build
```

`notojs_plugin(demo)` produces `demo.so` and installs it under `lib/notojs/plugin`, matching the `[plugin].path` search directory in the generated server configuration. Enable it with a `[plugin:demo]` section. Packages generated through CPack depend on the exact **NotoJS** devkit version used to configure the project.

Plugins execute inside the server process. Stop worker threads in `end()` and do not retain `JSContext *` or `JSValue` objects beyond callbacks unless their lifetimes are managed explicitly.

###### See also
- `doc('topic:config')`
- `doc('topic:lmdb')`
