I/O operations are executed in a background thread pool and streamed in 16KB chunks.

### Example

```javascript
import * as fs from 'noto:fs';

const f = fs.path('/file.txt');
await f.write('Hello, world!');

const d = await f.text();
print(d); 
```
