## LMDB

**[LMDB](https://www.symas.com/mdb)** is widely used in **NotoJS** and is shared by the server and kernel processes. It provides persistent storage for the HTTP cache,
package configuration, underpins the `Storage` API, and supports user-defined key-value stores.

Database names are prefixed with a namespace.

The following namespaces are used:

- `sys:` — system databases  
- `usr:` — user-defined databases  
- `var:` — plugin-defined databases  

The `sys:` namespace also defines several sub-namespaces and databases:

- `sys:errorlog` — used for logging errors occurred during automatic execution of notebooks.
- `sys:httpdata` — used for HTTP cache
- `sys:packages` — stores package configuration
- `sys:storage:` — sub-namespace containing databases backing the `Storage` API.

While `sys:` and `usr:` databases may define custom binary formats for storing values, user-defined databases follow a standard format:

- `key` — either `Number` or `String`
  - if `Number`, it is stored as a zero-prefixed big-endian `int64_t` with the sign bit flipped, ensuring natural numeric ordering in binary form (9 bytes total)
  - if `String`, it is stored as a non-null-terminated sequence of bytes

- `value` — either `String` or a JSON-serializable value
  - if `String`, it is stored as a non-null-terminated sequence of bytes
  - otherwise, it is stored as a null-terminated JSON representation

Databases in the `sys:storage:` namespace (those backing the `Storage` API) use the same binary format, with the `key` type limited to `String`.

The size of the memory-mapped file is set to `10485760` bytes (`10mb`) by default and can be configured
in the main configuration file, see `doc('topic:config')`.

All databases in `sys:storage:`, `usr:` and `var:` namespaces are listed in **NotoJS** sidebar, see `doc('topic:ui')`.

### Example

```javascript
import * as db from 'noto:db';

const d = db.open('db:a', 'db:b');
d.rw((a, b) => {
  a.set(1, 'one');
  b.set('one', 'ein');
});

const x = d.ro((a, b) => {
  return [a.get(1), b.get('one')];
});

print(x);
d.drop();
```

###### See also
- `doc('Storage')`
- `doc('noto:db')`
- `doc('topic:config')`
- `doc('topic:packages')`
