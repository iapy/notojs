### Example

```javascript
import * as fs from 'noto:fs';
import gzip from 'gzip.so';

const gz = gzip(fs.path("/tmp/target.gz"));
await fs.path("/tmp/source.txt")
  .text()
  .then(t => gz.write(t)));

const data = await gz.text();
```
