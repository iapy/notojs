### Example

  ```js
  import YAML from 'yaml.so';

  const value = YAML.parse(`
  name: notojs
  enabled: true
  items:
    - one
    - two
  `);

  const source = YAML.stringify(value);
  ```
