## Pipelines

A **NotoJS** pipeline is a sequence of transformations applied to the current notebook `input` value. Pipelines are built with the global `$` function, usually with each transformation placed in its own notebook cell.

This cell-per-stage structure is the main idea of pipelines: every step is independent in the notebook UI, so transformations can be disabled, reordered, edited, or deleted without rewriting one large block of code.

When `$` is called with a function, **NotoJS** invokes that function with the current `globalThis.input` value:

```js!noplay
$(value => {
  return value.trim();
});
```

If the function returns a value other than `undefined`, that value replaces `globalThis.input`. The next pipeline step receives the updated value.

```js
globalThis.input = '  hello  ';

$(text => text.trim());
$(text => text.toUpperCase());

print(input); // HELLO
```

Returning `undefined` leaves `input` unchanged. This is useful for validation, logging, or side effects:

```js!noplay
$(value => {
  console.clog('pipeline input', value);
});
```

### Chaining transformations across cells

Each call to `$` is one pipeline stage. In practice, each stage should live in a separate cell: one cell reads the incoming `input`, transforms it, and leaves the result in `input` for the next enabled cell.

This makes the pipeline interactive. You can disable a stage to skip it, drag cells to change the transformation order, or delete a stage entirely. The notebook then runs the remaining enabled cells in order, passing the updated `input` from one stage to the next.

First cell:

```js!noplay
$(rows => rows.filter(row => row.enabled));
```

Second cell:

```js!noplay
$(rows => rows.map(row => ({
  name: row.name,
  total: row.items.reduce((sum, item) => sum + item.price, 0)
})));
```

Third cell:

```js!noplay
$(rows => rows.sort((a, b) => b.total - a.total));
```

Later cell:

```js!noplay
print(input);
```

Because stages are plain JavaScript functions, they can use any available APIs, including `Storage`, `fetch`, `require`, or imported modules.

###### See also
- `doc('$')`
