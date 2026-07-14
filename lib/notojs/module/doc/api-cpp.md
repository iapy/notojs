### C++ API{text-decoration=underline}

All required public C++ API headers are in the repository's `/api` directory. To use the C++ API from an external project, copy or symlink that `/api` directory anywhere in your project tree and add it to your compiler include path.

#### JavaScript ↔ C++ bridge

The public `bridge.hpp` header contains small C++ wrappers around **QuickJS** values. It lets native code expose JavaScript functions, modules, classes, and objects without manually writing all argument conversion and `JSValue` lifetime handling.

Common wrappers include
- `bridge::String`
- `bridge::Number`
- `bridge::Array`
- `bridge::ArrayBuffer`
- `bridge::Object`
- `bridge::Promise`
- `bridge::Lambda`

Native functions can declare these types as parameters and use them to construct correctly typed return values. `bridge::Function<&fn>::invoke` can be used in `JSCFunctionListEntry` tables to validate arguments, convert values, call the C++ function, and return the resulting `JSValue`.

```cpp
JSValue random(JSContext *ctx, bridge::Number count) {
    std::vector<std::uint8_t> bytes(static_cast<std::int64_t>(count));
    return bridge::ArrayBuffer(ctx, bytes.data(), bytes.size());
}

JSCFunctionListEntry const funcs[] = {
    JS_CFUNC_DEF("random", 1, &bridge::Function<&random>::invoke)
};
```

The bridge provides type-safe dispatch at the C++ boundary: if JavaScript passes values that do not match the declared wrapper types, the call is rejected with `TypeError` before the native function body runs. A single JavaScript export can also map to multiple C++ overloads, with the bridge selecting the first overload whose argument types match.

```cpp
JSValue hash_0(JSContext *ctx, bridge::String algorithm, bridge::ArrayBuffer data);
JSValue hash_1(JSContext *ctx, bridge::Lambda algorithm, bridge::ArrayBuffer data);

using hash = bridge::Function<&hash_0, &hash_1>;

JSCFunctionListEntry const funcs[] = {
    JS_CFUNC_DEF("hash", 2, &hash::invoke)
};
```

The bridge also supports wrapping C++ interfaces and objects for JavaScript through `bridge::Interface`, plus stronger ownership helpers such as `bridge::Strong<T>` for values that must outlive a temporary expression.

A minimal native JavaScript module that wraps a C++ class and export its constructor:

```cpp
#include <bridge.hpp>

namespace {

struct Counter : bridge::Interface<Counter, int>
{
    Counter() : Base(0) {}

    JSValue inc(JSContext *ctx) {
        return bridge::Number(ctx, ++ref());
    }
};

JSCFunctionListEntry const Counter::funcs[] = {
    JS_CFUNC_DEF("inc", 0, &bridge::Function<&Counter::inc>::invoke)
};

int init(JSContext *ctx, JSModuleDef *mod)
{
    Counter::init(ctx, mod);
    return 0;
}

} // namespace

extern "C" JSModuleDef *js_init_module(JSContext *ctx, char const *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, &init);
    if(!mod) return nullptr;

    JS_AddModuleExport(ctx, mod, Counter::name());
    return mod;
}
```

JavaScript can then import and instantiate the native class:

```js!noplay
import { Counter } from 'counter.so';

const c = new Counter();
print(c.inc());
```

The bridge also supports exposing class static methods, special methods such as property getters and setters, inheritance between wrapped classes, and JavaScript iterators.

#### **NotoJS** specific types

The public `global.hpp` and `module.hpp` headers define C++ wrappers for **NotoJS**-specific functions and classes. These wrappers expose native access to selected global APIs and module APIs, such as host-backed object types, constructors, and helper functions that return `JSValue` instances suitable for use from native **QuickJS** callbacks.

###### See also
- `doc('global.hpp')`
- `doc('module.hpp')`

#### **LMDB** databases

The public `notodb.hpp` header provides access to the **LMDB** environment managed by **NotoJS**. Native code should use this environment instead of opening a separate database environment when it needs to read or write data that belongs to the running server.

Create a `notojs::DB` from an `MDB_env *` returned by the host API, or from a `JSContext *` when running inside a native **QuickJS** callback:

```cpp
#include <notodb.hpp>

notojs::DB db{host.lmdb()};
// or, inside a native JS callback:
notojs::DB db{ctx};
```

`notojs::DB` exposes helpers for built-in databases such as `http()` and `pkgs()`, and generic `open()` overloads for named databases in one of the **NotoJS** namespaces:

- `DB::SYS` — internal system databases
- `DB::USR` — user-facing data
- `DB::VAR` — variable/plugin/application data

Use read-only access for lookups and read-write access for updates:

```cpp
auto [txn, dbi] = db.open(notojs::DB::RO, notojs::DB::VAR, "myplugin:data");
auto [wtxn, wdbi] = db.open(notojs::DB::RW, notojs::DB::VAR, "myplugin:data");
```

Plugins should prefer the `VAR` namespace and use unique database names, for example by prefixing them with the plugin name. This avoids collisions with **NotoJS** internal databases and with other plugins.

The same header also exposes environment utilities such as `stat()`, `info()`, and `backup(path)`, plus byte-order helpers for storing integer keys or values consistently.

#### Asynchronous tasks

The public `engine.hpp` header provides `notojs::Task`, a helper for exposing long-running native work to JavaScript as a `Promise`. A task runs its C++ work outside the immediate native callback and later resolves or rejects the JavaScript promise on the **QuickJS** context.

To define a task, derive from `notojs::Task` and implement two virtual functions:

- `step()` performs one unit of background work and returns `Again` to continue or `Finish` when the task is complete.
- `then(JSContext *, JSValue &)` runs after the background work finishes. It creates the JavaScript result value and returns `Resolve` or `Reject`.

```cpp
#include <engine.hpp>
#include <bridge.hpp>

class CountTask : public notojs::Task
{
public:
    explicit CountTask(std::int64_t n)
    : n{n} {}

protected:
    Step step() override
    {
        if(n <= 0) return Finish;
        result.push_back(std::to_string(n--));
        return Again;
    }

    Then then(JSContext *ctx, JSValue &value) override
    {
        if(result.empty()) return Reject;

        bridge::Array array{ctx};
        for(auto const &item : result)
            array.append(bridge::String(ctx, item));

        value = array;
        return Resolve;
    }

private:
    std::int64_t n;
    std::vector<std::string> result;
};
```

A native function starts the task by creating it as a `std::shared_ptr` and returning `task->run(ctx)`:

```cpp
JSValue run(JSContext *ctx, bridge::Number n)
{
    return std::make_shared<CountTask>(n)->run(ctx);
}
```

From JavaScript, the exported function can be used as an asynchronous operation:

```js!noplay
const result = await run(3);
// ["3", "2", "1"]
```

Use this pattern when native code needs to do incremental or blocking work without completing inside the initial JavaScript callback. Keep **QuickJS** values and `JSContext *` usage in `then()`; `step()` should work with ordinary C++ data owned by the task.

#### Server-side plugins

**NotoJS** server-side plugins are shared libraries loaded by the main server process at startup. A plugin can run background C++ code, react to external events, execute notebooks, provide input to those notebooks, receive execution output, and optionally expose a native JavaScript module.

Plugins are useful for integrating **NotoJS** with local services, devices, message queues, file watchers, pipes, sensors, or any other source of events that should trigger notebook code.

##### Configuration

Plugins are enabled in the **NotoJS** configuration file.

```ini
[plugin]
path = /path/to/plugins

[plugin:demo]
pipe = /tmp/notojs-demo.pipe
book = plugin-demo.notojs
```

- `[plugin].path` is the directory containing plugin shared libraries. It defaults to `/usr/local/lib`.
- `[plugin:NAME]` enables plugin `NAME`.
- **NotoJS** loads `{plugin.path}/NAME.so`.
- The contents of `[plugin.NAME]` are passed to the plugin loader as a `boost::property_tree::ptree`.

For the example above, **NotoJS** loads:

```text
/path/to/plugins/demo.so
```

and calls the exported `notojs_plugin` function.

##### Plugin entry point

A plugin shared library must export:

```cpp
extern "C" notojs::IPlugin *notojs_plugin(
    boost::property_tree::ptree const &config
);
```

The function receives the plugin's configuration subtree and returns a heap-allocated `notojs::IPlugin` instance. **NotoJS** takes ownership of the returned pointer.

Minimal skeleton:

```cpp
#include <plugin.hpp>

class MyPlugin : public notojs::IPlugin {
public:
    MyPlugin(boost::property_tree::ptree const &cfg) {}

    bool run(notojs::IHost &host) override {
        return true;
    }

    void end(notojs::IHost &host) override {}
};

extern "C" notojs::IPlugin *notojs_plugin(
    boost::property_tree::ptree const &pt
) {
    return new MyPlugin(pt);
}
```

If the library cannot be loaded, or if the `notojs_plugin` symbol is missing, **NotoJS** logs the error and does not start that plugin.

##### Lifecycle

Plugins participate in the server lifecycle:

1. During configuration, **NotoJS** finds every `[plugin.NAME]` section and loads `{plugin.path}/NAME.so`.
2. When the server starts, **NotoJS** calls `plugin->run(host)`.
3. If `run()` returns `false`, the plugin is removed and not used.
4. If `run()` returns `true`, the plugin remains active.
5. After all active plugins start, **NotoJS** registers any plugin modules returned by `plugin->mod(host)`.
6. On server shutdown, **NotoJS** calls `plugin->end(host)`.

`run()` is the right place to start worker threads, open sockets, create file watchers, or subscribe to external systems. `end()` should stop those resources and join any plugin-owned threads.

##### The `IHost` host API

**NotoJS** passes an `IHost` reference to `run()`, `end()` and `mod()`.

```cpp
class IHost {
public:
    virtual MDB_env *lmdb() = 0;
    virtual void clog(std::string &&line) = 0;
    virtual void load(std::string const &notebook) = 0;
    virtual bool exec(std::string const &notebook, IContext &context) = 0;
};
```

###### `lmbb()`

Returns a pointer to **NotoJS** **LMDB** environment.

###### `clog(line)`

Writes a line to the **NotoJS** server log. After the server has started, logging is posted through the server synchronous I/O thread so plugins can call it from background threads.

```cpp
host.clog("event received");
```

###### `load(notebook)`

Registers a notebook with the plugin executor and records its current version.

```cpp
host.load("plugin-demo.notojs");
```

When the notebook is saved, renamed, or removed through the workspace API, **NotoJS** updates or clears the cached version used by plugins.

###### `exec(notebook, context)`

Executes a registered notebook with a plugin-provided context.

```cpp
host.exec("plugin-demo.notojs", *this);
```

**NotoJS** compiles and caches the notebook as bytecode, executes all cells in order, and records execution errors in `sys:errorlog` database.
If the notebook no longer exists, its cached bytecode is removed and `exec()` returns `false`.

##### Passing input and receiving output

`IPlugin` inherits from `IContext`:

```cpp
class IContext {
public:
    virtual void input(JSContext *ctx, JSValue &value);
    virtual void output(JSContext *ctx);
};
```

Override `input()` to provide the notebook's global `input` value.

```cpp
void input(JSContext *ctx, JSValue &value) override {
    value = bridge::String{ctx, current_line};
}
```

If `input()` leaves `value` as `JS_UNDEFINED`, no global `input` value is installed.

Override `output()` to inspect results after notebook execution. The default implementation does nothing. The callback receives the QuickJS context after all cells finish successfully.

##### Exposing a JavaScript module

Plugins can optionally expose a native JavaScript module by overriding `mod()`.

```cpp
notojs::IPlugin::Module mod(notojs::INotoJS &) override {
    return [](JSContext *ctx, char const *name) -> JSModuleDef * {
        JSModuleDef *mod = JS_NewCModule(ctx, name, &init);
        if(!mod) return nullptr;

        JS_AddModuleExportList(ctx, mod, funcs, count);
        return mod;
    };
}
```

**NotoJS** registers the module under the plugin name. A plugin named `demo` can be imported as:

```js!noplay
import { get } from 'noto:demo';
print(get());
```

Plugin modules are registered after `run()` succeeds. If `run()` returns `false`, the module is not registered.

##### Inotify

The bundled `inotify` plugin watches a configured filesystem path and executes a notebook whenever a regular file is created, modified, or moved into that watched tree. On macOS it uses **FSEvents**; on Linux it uses native **inotify** and recursively adds watches for subdirectories.

Configure it with `path` and `book`:

```ini
[plugin:inotify]
path = /tmp/watch
book = inotify.notojs
```

- `path` is resolved as a **NotoJS** filesystem path and converted to a native path by the plugin.
- `book` is the notebook executed for each matching file event.

The notebook receives the changed path as its global `input` value. The path is converted back to the configured **NotoJS** virtual path space, so notebook code can pass it to `noto:fs` APIs.

The plugin also exposes a small native module named `noto:inotify` with `path()`, which returns the configured watched path as a `noto:fs` `Path` object.

##### Building plugins

Plugins are built as shared libraries with no `lib` prefix and a `.so` suffix. The helper macro in `/api/plugin.cmake` shows the expected shape. After copying or symlinking `/api` into your project tree, include `api/plugin.cmake` and use the `plugin(NAME)` helper for each plugin source file:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my-notojs-plugin)

include(api/plugin.cmake)
plugin(myplugin)
```

The plugin source should include the public plugin API header:

```cpp
#include <plugin.hpp>
```

Build it as a normal CMake target:

```sh
cmake -S . -B build
cmake --build build --target myplugin
```

This produces `myplugin.so`; place that file in the configured `[plugin].path` directory and enable it with a `[plugin:myplugin]` section. Build plugins with the same QuickJS and **NotoJS** ABI expectations as the running server. On macOS, the helper uses unresolved dynamic lookup so plugin symbols are resolved by the host process at load time.

##### Notebook caching and updates

Plugin-triggered notebooks use the same cacher as timer and application execution. The first execution compiles the notebook to bytecode. Later executions reuse cached bytecode until the notebook changes.

When a notebook is saved through the editor or folder API, **NotoJS** calls the plugin manager's update hook. If a plugin previously loaded that notebook with `noto.load(name)`, the next `exec()` uses the updated version.

When a notebook is removed or renamed, the cached plugin version is cleared and `exec()` returns `false` until the plugin loads a valid notebook again.

##### Safety and threading notes

- Plugins run inside the **NotoJS** server process; crashes or undefined behavior can affect the server.
- Plugin worker threads should stop promptly in `end()`.
- Use `INotoJS::clog()` instead of writing directly from many threads when possible.
- Do not keep `JSContext *` or `JSValue` values beyond the `input()` or `output()` callback unless you fully manage QuickJS lifetimes.
- Keep plugin notebook execution short if events can arrive frequently; `exec()` runs notebook code synchronously for the calling plugin thread after compilation/cache lookup.

###### See also
- `doc('topic:config')`
- `doc('topic:lmdb')`
