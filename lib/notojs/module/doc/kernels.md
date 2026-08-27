## Notebook kernels and cell scope

Interactive notebooks run in isolated kernel processes. Each open notebook in the **NotoJS** UI is associated with a separate child process started by the server. The kernel owns the notebook's **QuickJS** runtime and execution context, so in-memory globals, loaded modules, pending JavaScript work, and failures are isolated from other open notebooks.

If a kernel exits or its event connection is lost, the UI marks the notebook as disconnected. Reconnecting creates or attaches a kernel for that notebook before execution controls are enabled again.

### Cell module scope

Each JavaScript cell is compiled and evaluated as a separate ES module with a generated name such as `cell-000`. Normal top-level declarations therefore belong only to that cell:

```js!noplay
const privateValue = 1;
let counter = 0;
function increment() {
  return ++counter;
}
```

`privateValue`, `counter`, and `increment` are not visible to another cell. This applies to top-level `const`, `let`, `var`, function and class declarations, as well as imported bindings.

Use `export` to make a binding available to later cells in the same notebook:

```js!noplay
// First cell
export const value = 42;
export function double(number) {
  return number * 2;
}
```

```js!noplay
// Later cell
print(value);         // 42
print(double(value)); // 84
```

After a cell finishes successfully, **NotoJS** copies its enumerable module exports onto the notebook global object. A later cell can consequently use an exported name directly without importing the earlier cell. Exporting a name that already exists in notebook-global scope replaces that global property.

If cell evaluation fails, its exports are not promoted to notebook-global scope.

### Explicit global state

Code can also share state by assigning directly to `globalThis`:

```js!noplay
globalThis.count = 1;
```

```js!noplay
count += 1;
print(count);
```

For ordinary cell declarations, explicit `export` statements make the intended public interface clearer. Direct `globalThis` assignment is useful for mutable shared state and compatibility with scripts that install browser-like globals.

Some notebook features generate exports automatically:

- An enabled input cell exports `input`, making its text or parsed JSON value available to subsequent cells.
- A named Markdown block prefixed with `!` is rewritten as an exported Markdown value.

### Resetting execution state

The notebook context menu provides two different reset operations:

- **Reset context** creates a new `JSContext` inside the existing kernel process. Notebook globals are removed, while the kernel's module cache is retained.
- **Restart kernel** terminates the child process and starts a new kernel, replacing its QuickJS runtime, context, globals, and in-memory module state.

Neither operation removes saved notebooks or persistent LMDB-backed data such as `Storage` values, package configuration, and HTTP cache entries.

### Execution without a notebook kernel

Not every notebook execution uses an interactive kernel:

- **NotoJS applications** execute HTTP requests inside the server process and do not fork a separate notebook kernel.
- `POST` and `PUT` requests to the Result API also execute notebooks in the server process without creating or using an interactive notebook kernel.

These execution paths use the same JavaScript APIs and modules, but they should not be treated as sharing the in-memory globals of an open UI notebook.

###### See also
- `doc('topic:ui')`
- `doc('topic:applications')`
- `doc('topic:packages')`
- `doc('topic:markdown')`
- `doc('topic:api:http')`
