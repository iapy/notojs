### Example

The `/tmp` virtual filesystem mount must be configured as writable.

```javascript
import * as fs from 'noto:fs';
import zip from 'zip.so';

const source = fs.path('/tmp/source.txt');
await source.write('Hello from NotoJS');

const archive = zip(fs.path('/tmp/example.zip'));

await archive.write({
  'README.txt': 'Archive created by NotoJS',
  'files/source.txt': source
});

const files = await archive.read('README.txt', 'files/source.txt');

print(await files['README.txt'].text());
print(await files['files/source.txt'].text());

for (const [name, size] of archive) {
  print(`${name}: ${size} bytes`);
}
```
