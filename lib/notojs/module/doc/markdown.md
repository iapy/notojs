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
print(new $.__Markdown({data: `
# Hello from Markdown
`}));
```

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
const hello = new $.__Markdown({data: `
# Markdown block
`});
```

Prefix the name with `!` to export the value into global notebook scope:

```javascript
<[!hello[
# Exported markdown block
]]>
```

This becomes:

```javascript
export const hello = new $.__Markdown({data: `
# Exported markdown block
`});
```

### Code echo blocks

Use `<[![` to echo JavaScript source as a non-runnable highlighted Markdown code block:

```javascript
<[![
const x = 1 + 2;
print(x);
]]>
```

The preprocessor wraps the block in a fenced code block with `js!noplay`, so it is displayed without the run action.

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
