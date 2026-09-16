## Markdown

NotoJS supports Markdown in ordinary JavaScript notebook cells. There is no separate Markdown cell type: Markdown blocks are preprocessed into JavaScript before execution.

### Markdown blocks

A Markdown block starts at the beginning of a line with `<[[` and ends with `]]>` on its own line:

```javascript
const value = 42;
<[[
# Hello from Markdown
]]>
print(`Back in JavaScript: ${value}`);
```

During preprocessing, the block is rewritten as a `Markdown` object and printed:

```javascript
print($(`# Hello from Markdown`));
```

`$(string)` is the Markdown factory used by the preprocessor. It accepts a primitive string and returns an immutable `Markdown` value without calling the disabled constructor. The examples show readable template literals; generated code uses escaped string literals, so quotes, backslashes, newlines, and `${...}` in Markdown remain literal text rather than JavaScript interpolation.

### Named Markdown values

A block can assign Markdown to a JavaScript constant instead of printing it immediately. Put an identifier between `<[` and `[`:

```javascript
<[hello[
# Markdown block
]]>
print(hello);
```

This preprocesses to a constant similar to:

```javascript
const hello = $(`# Markdown block`);
```

Prefix the name with `!` to export the value into global notebook scope:

```javascript
<[!hello[
# Exported markdown block
]]>
```

This becomes:

```javascript
export const hello = $(`# Exported markdown block`);
```

### Code echo blocks

Use `<[![` to echo JavaScript source as a non-runnable highlighted Markdown code block:

```javascript
<[![
const x = 1 + 2;
print(x);
]]>
```

The preprocessor prints the source through `$()` as a fenced code block with `js!noplay`, so it is displayed without the run action. It then executes the original JavaScript unchanged, once.

### Slides

Use `<[:[` to print a Markdown block with the slide layout:

```javascript
<[:[
# Slide 1
]]>

<[:[
# Slide 2
]]>
```

This is equivalent to printing Markdown through `print[':'](...)`.

### Rendering features

Markdown output is rendered with `markdown-it` in the notebook frontend.

Supported extensions include:

- syntax highlighting for fenced code blocks through **highlight.js**
- task lists, such as `- [ ] todo` and `- [x] done`
- inline math with `$...$`
- block math fenced by lines containing only `$$`
- custom classes and inline styles appended with `{...}`

Example:

```javascript
<[[
## Tasks

- [ ] write docs
- [x] implement renderer

Inline math: $a^2 + b^2 = c^2$

$$
E = mc^2
$$

A highlighted paragraph {color=red}
]]>
```

JavaScript fenced code blocks are runnable by default in rendered Markdown. The notebook adds a **Run code** action for `js` and `javascript` fences:

````js
<[[
```js
print('Run this in a new cell');
```
]]>
````

Use `!noplay` to render highlighted source without the run action:

````js
<[[
```js!noplay
print('Display only');
```
]]>
````

### Programmatic Markdown

Markdown values can be created explicitly from JavaScript with `noto:core`:

```javascript
import { markdown } from 'noto:core';

const a = 'Alice';
const b = 'Bob';
print(markdown(`Hello, ${a}! I'm ${b}.`));
```

Use `$(string)` or the public `markdown()` factory to create Markdown values. The `Markdown` class remains exported from `noto:core` for `instanceof` checks, but `new Markdown(...)` rejects construction. Factory-created values are frozen; create a new value to change their content.

### Templating with Mustache

Use the global `$` function to apply a Mustache template to a Markdown value:

```javascript
<[template[
### {{title}}

Hello, {{name}}.

Please complete the following documentation sections:

{{#items}}
- {{.}}
{{/items}}

{{sign}}
]]>

print($(template, {
  title: 'Change request',
  name: 'Alice',
  sign: 'Bob',
  items: [
    'Markdown blocks',
    'Mustache templates'
  ]
}));

print($(template, {
  title: 'Change request',
  name: 'Carol',
  sign: 'Bob',
  items: [
    'print function'
  ]
}));
````

###### See also
- `doc('noto:core')`
- `doc('print')`
- `doc('$')`
