### Example

```javascript
import gzip from 'noto:gzip';
import * as fs from 'noto:fs';

const gz = gzip(fs.path("/tmp/target.gz"));
await fs.path("/tmp/source.txt")
  .text()
  .then(t => gz.write(t)));

const data = await gz.text();
```