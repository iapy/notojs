## NotoJS UI

The **NotoJS** UI is mobile-friendly and is implemented using only native browser APIs and these libraries:
- **CodeMirror**
- **headroom.js**
- subset of **Material Symbols**

The header toolbar provides common notebook actions:

Left side:
- ![menu](https://api.iconify.design/material-symbols-light/menu.svg) open the sidebar{.list-item-with-icon}
    - `⌘ + \` on Apple 
    - `Ctrl + \` on Windows/Linux

Right side (top-to-bottom or right-to-left depending on the browser window size):
- ![toggle](https://api.iconify.design/material-symbols-light/article-outline.svg) switch to preview mode{.list-item-with-icon}
    - `⌘ + P` on Apple 
    - `Ctrl + P` on Windows/Linux
- ![clear](https://api.iconify.design/material-symbols-light/do-not-disturb-off-outline.svg) clear all rendered output{.list-item-with-icon}
    - `Ctrl + Shift + L` on all platforms
- ![play](https://api.iconify.design/material-symbols-light/play-circle-outline.svg) run all cells{.list-item-with-icon}
    - `⌘ + Shift + Enter` on Apple
    - `Ctrl + Shift + Enter` on Windows/Linux
- ![input](https://api.iconify.design/material-symbols-light/draft-orders.svg) add an input cell{.list-item-with-icon}

### Generic cells

Each generic cell has a toolbar that provides cell actions:

Left side:
- ![play](https://api.iconify.design/material-symbols-light/play-circle-outline.svg) run the cell{.list-item-with-icon}
    - `⌘ + Enter` on Apple
    - `Ctrl + Enter` on Windows/Linux
- ![clear](https://api.iconify.design/material-symbols-light/do-not-disturb-off-outline.svg) clear the cell's output{.list-item-with-icon}
    - `Ctrl + L` on all platforms 

Right side:
- ![above](https://api.iconify.design/material-symbols-light/add-row-above-outline.svg) insert a cell above{.list-item-with-icon}
- ![below](https://api.iconify.design/material-symbols-light/add-row-below-outline.svg) insert a cell below{.list-item-with-icon}
- ![toggle](https://api.iconify.design/material-symbols-light/dock-to-right-outline.svg) open the secondary toolbar{.list-item-with-icon}

Secondary toolbar:
- ![delete](https://api.iconify.design/material-symbols-light/delete-outline.svg) remove the cell{.list-item-with-icon}
- ![move up](https://api.iconify.design/material-symbols-light/text-select-move-up.svg) move the cell up{.list-item-with-icon}
- ![move down](https://api.iconify.design/material-symbols-light/text-select-move-down.svg) move the cell down{.list-item-with-icon}
- ![enable](https://api.iconify.design/material-symbols-light/label-outline.svg) enable/disable the cell{.list-item-with-icon}
- ![toggle](https://api.iconify.design/material-symbols-light/dock-to-left-outline.svg) open the primary toolbar{.list-item-with-icon}

After execution, the cell toolbar shows:
- left side: the generated cell name, such as `cell-000`. Absence of generated name indicates that the cell has never been executed in the current kernel.
- right side: cell execution time

### Input cell

Input cell is used to define the global `input` value for notebook execution.

Input cell can switch between two modes:

- ![text](https://api.iconify.design/material-symbols-light/article-outline.svg) text input — the content is exported as a string{.list-item-with-icon}
- ![json](https://api.iconify.design/material-symbols-light/data-object.svg) json input — the content is parsed as JSON and exported as JavaScript object{.list-item-with-icon}

Internally, the input cell generates code equivalent to one of:

```js!noplay
globalThis.input = "...text...";
globalThis.input = { /* object data */ };
```

Input cell is intended for testing automated notebook execution and is not saved with the notebook. Disabling or removing the input cell executes

```js!noplay
delete globalThis.input
```

### Package configuration cell

The package configuration cell is available only in Package configuration editor and is used to edit the and apply configuration.

Left side:
- ![play](https://api.iconify.design/material-symbols-light/play-circle-outline.svg) apply the package configuration and restart the kernel{.list-item-with-icon}
    - `⌘ + Enter` on Apple
    - `Ctrl + Enter` on Windows/Linux

Right side:
- ![below](https://api.iconify.design/material-symbols-light/add-row-below-outline.svg) insert a normal JavaScript cell below; this may be used to test the applied configuration{.list-item-with-icon}

Other cell actions are not applicable and are disabled.

#### Additional shortcuts

- move to previous cell (create a new cell above if none exists):
  - `⌥ + ↑` on Apple
  - `Alt + ↑` on Windows/Linux
- move to next cell (create a new cell below if none exists):
  - `⌥ + ↓` on Apple
  - `Alt + ↓` on Windows/Linux

### Sidebar

The sidebar shows workspace resources.

#### Opened

Currently open notebooks. The notebook name follows the kernel PID and the close button, which closes the notebook and discards any unsaved changes. The notebook name is *italicized* if it has unsaved changes.
- ![notebook](https://api.iconify.design/material-symbols-light/book-outline.svg) generic notebook{.list-item-with-icon}
- ![library](https://api.iconify.design/material-symbols-light/code-blocks-outline.svg) library notebook or package configuration editor{.list-item-with-icon}
- ![application](https://api.iconify.design/material-symbols-light/webhook.svg) application notebook{.list-item-with-icon}

The active notebook (the one currently being edited) has a context menu:
- ![reset](https://api.iconify.design/material-symbols-light/restart-alt.svg) Reset context — re-creates `JSContext` in the kernel process. This removes all global variables while keeping the module cache.{.list-item-with-icon}
- ![reset](https://api.iconify.design/material-symbols-light/restart-alt.svg) Restart kernel — restarts the kernel process.{.list-item-with-icon}
    - `⌘ + K` on Apple
    - `Ctrl + K` on Windows/Linux

Clicking the notebook icon of the active notebook saves the notebook on the server.
- `Enter` on all platforms

#### Notebooks

Displays generic notebooks in the workspace. Clicking a notebook name opens it. Clicking a notebook icon opens its context menu:
- ![open](https://api.iconify.design/material-symbols-light/content-copy-outline.svg) Open as new — creates a new notebook populated with cells from this notebook.{.list-item-with-icon}
- ![rename](https://api.iconify.design/material-symbols-light/edit-document-outline.svg) Rename — renames the notebook.{.list-item-with-icon}
- ![remove](https://api.iconify.design/material-symbols-light/delete-outline.svg) Remove — removes the notebook.{.list-item-with-icon}

#### Libraries

Displays library notebooks in the workspace. Clicking a library name opens it. Clicking a library icon opens its context menu.

#### Applications

Displays application notebooks in the workspace. Clicking an application name opens it. Clicking an application icon opens its context menu.

#### Packages

Displays installed packages and their sizes.

- ![script](https://api.iconify.design/material-symbols-light/code.svg) legacy script package{.list-item-with-icon}
- ![module](https://api.iconify.design/material-symbols-light/code-blocks-outline.svg) ES module package{.list-item-with-icon}

#### Databases

Lists **LMDB** databases and their sizes. Hidden if no databases have been created.

- ![storage](https://api.iconify.design/material-symbols-light/home-storage-outline.svg) database backing `Storage`{.list-item-with-icon}
- ![user](https://api.iconify.design/material-symbols-light/database-outline.svg) user-defined database{.list-item-with-icon}
- ![plugin](https://api.iconify.design/material-symbols-light/database-off-outline.svg) plugin-defined database{.list-item-with-icon}

If the sidebar was opened while a cell editor was focused, clicking a database inserts code that accesses it.

#### Search mode

Press `\` while the sidebar is open to toggle search mode. Press `Enter` in search mode to open the result if it is unique.
If the sidebar was opened while a cell editor was focused, search also queries `iconify.design` for icons.
Clicking an icon inserts code that imports it into the cell.

### Preview mode

Preview mode hides editors and cell toolbars, leaving only rendered output.
In preview mode, the header changes from edit controls to export controls:

- ![toggle](https://api.iconify.design/material-symbols-light/code-blocks-outline.svg) switch to edit mode{.list-item-with-icon}
    - `⌘ + P` on Apple
    - `Ctrl + P` on Windows/Linux
- ![export](https://api.iconify.design/material-symbols-light/download-for-offline-outline.svg) download rendered output as standalone HTML{.list-item-with-icon}
 
### Kernels and connection state

The UI keeps a server-sent-events connection to the **NotoJS** server. **NotoJS** *does not* use websockets, which means running it behind **Nginx**
is configured using just
- `proxy_http_version 1.1;`
- `proxy_set_header Upgrade $http_upgrade;`

Each open notebook is associated with a separate kernel process forked by the server. If the event stream fails or the kernel process exits, the UI marks the notebook as disconnected and disables execution controls. When the connection returns, the UI requests a fresh kernel and re-enables execution.

###### See also
- `doc('topic:applications')`
- `doc('topic:packages')`
- `doc('topic:lmdb')`
