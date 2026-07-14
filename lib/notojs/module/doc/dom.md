### Examples

#### Using SVG.js

```javascript
import { SVG, registerWindow } from
'https://cdnjs.cloudflare.com/ajax/libs/svg.js/3.2.5/svg.esm.js';
import { Document, window } from 'noto:dom';

registerWindow(window, Document.html());
const draw = SVG().viewbox(0, 0, 100, 100);

draw.rect(100, 100)
  .fill(draw.gradient('linear', function(add) {
    add.stop(0, '#f06');
    add.stop(1, '#0f9')
  }))
  .radius(10);

print(draw.node);
```

#### Parsing HTML

```javascript
import { Document } from 'noto:dom';

const d = await fetch('https://apple.com/')
  .then(r => r.text())
  .then(t => Document.html(t));

const a = new Array();
d.querySelectorAll('svg').forEach(s => {
  a.push(s);
  if(10 === a.length) {
    print(...a);
    a.length = 0;
  }
});

if(a) print(...a);
```
