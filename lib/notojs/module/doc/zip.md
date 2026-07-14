### Example

```javascript
import zip from 'noto:zip';
import * as fs from 'noto:fs';

const [b] = await Promise.all([
    fetch('https://xkcd.com/s/0b7742.png')
        .then(r => r.blob()),
    fetch('https://xkcd.com/')
        .then(r => r.text())
        .then(t => fs.path("/tmp/t.txt").write(t))
]);

const zf = zip(fs.path("/tmp/target.zip"));
await fs.path("/tmp/t.txt")
  .text()
  .then(t => zf.write({
    'a.bin': b,
    'b.txt': t
  }));

const data = await zf.read('b.txt');
print((await data['b.txt'].text()).substr(0, 30));

for(const [name, size] of zf)
  print(`${name} ${size}`)
```
