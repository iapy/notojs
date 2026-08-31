window.connection = new class {
    #uuid = null;
    #time = null;
    #timer = null;
    #books = new Map();
    #events = null;
    #server = document.querySelector('head > base').getAttribute('href');

    static Connected = $.Event('Connected');
    static Disconnected = $.Event('Disconnected');

    constructor() {
        Object.defineProperty(this, 'uuid', {
            get() {
                return this.#uuid;
            }
        });
        Object.defineProperty(this, 'server', {
            get() {
                return this.#server;
            }
        });
        this.#connect();
    }
    run(code, kernel, cid = 'cell-XXX') {
        const target = kernel == null ? this.server
            : `${this.server}?cid=${encodeURIComponent(cid)}&pid=${encodeURIComponent(kernel)}&wid=${encodeURIComponent(this.uuid)}`;
        return fetch(target, {method: 'POST', body: code}).then(response =>
            response.text().then(text => {
                if(kernel == null && !response.ok)
                    throw new Error(`JavaScript execution failed: ${response.status}`);
                const output = JSON.parse(text || '[]');
                Object.defineProperties(output, {
                    text: {value: text},
                    time: {value: parseInt(response.headers.get('X-NotoJS-Exec-Time'))},
                    ok: {value: response.ok},
                    status: {value: response.status}
                });
                return output;
            })
        );
    }
    #connect() {
        this.#events = new EventSource(`${this.server}w`);
        this.#events.addEventListener('uuid', e => {
            if(this.#uuid != e.data) {
                this.#uuid = e.data;
                this.#books.forEach((k, book) => this.add(book));
                console.log(`Window opened ${this.#uuid} ${this.#books.size}`);
            }
            this.#time = new Date();
        });
        this.#events.addEventListener('pids', e => {
            const live = new Set(e.data.split(',').map(x => parseInt(x)));
            this.#books.forEach((k, book) => {
                if(!live.has(k)) {
                    this.#books.set(book, null);
                    book.$dispatch(this.constructor.Disconnected, {
                        kernel: k,
                        reason: 'server'
                    });
                    this.add(book);
                } else live.delete(k);
            });
            live.forEach((k) => {
                fetch(`${this.server}k/${this.#uuid}`, {method: 'DELETE', body: k});
            });
            this.#time = new Date();
        });
        this.#events.addEventListener('error', e => {
            if(this.#uuid) this.#notify('network');
            this.#events.close();
            clearInterval(this.#timer);
            setTimeout(this.#connect.bind(this), 2500);
        });
        this.#timer = setInterval(_ => {
            if(Date.now() - this.#time > 10000 && this.#uuid) {
                this.#notify('timer');
                this.#events.close();
                clearInterval(this.#timer);
                setTimeout(this.#connect.bind(this), 2500);
            }
        }, 5000);
    }
    #notify(reason) {
        this.#books.forEach((k, book) => {
            book.$dispatch(this.constructor.Disconnected, {
                kernel: k,
                reason: reason
            });
            this.#books.set(book, null);
        });
        this.#uuid = null;
    }
    add(book) {
        if(!this.#uuid) this.#books.set(book, null);
        else fetch(`${this.server}k/${this.#uuid}`)
            .then(r => {
                if(200 == r.status) r.text().then(t => {
                    this.#books.set(book, parseInt(t))
                    book.$dispatch(this.constructor.Connected, {
                        kernel: t
                    });
                });
                else this.#books.set(book, null);
            });
    }
    del(book) {
        const pid = this.get(book);
        this.#books.delete(book);
        return fetch(`${this.server}k/${this.#uuid}`, {method: 'DELETE', body: pid});
    }
    get(book) {
        return this.#books.get(book);
    }
};

class NJBook extends $.Element {
    static Clean = $.Event('Clean');
    static Input = $.Event('Input');
    static Blurred = $.Event('Blurred');
    static Changed = $.Event('Changed');
    static Packages = $.Event('Packages');

    #name = null;
    #type = null;
    #timeout = null;

    constructor() {
        super({
            'json': _ => {
                const data = new Array();
                for(const cell of this.cells) {
                    data.push((() => {
                        const c = new Object();
                        if('generic' === cell.role) {
                            c.code = cell.code;
                            if(cell.body) c.body = cell.body;
                            else if(!cell.active) c.role = 'disabled';
                        } else if('handler' === cell.role) {
                            c.code = CodeMirror.notojs.app.code(cell.edit);
                            c.role = 'handler';
                        }
                        return c;
                    })());
                }
                return JSON.stringify(data);
            },
            'name': [
                _ => this.#name ? (this.#type ? `${this.#type}/${this.#name}` : this.#name) : null,
                v => this.#name = v
            ],
            'pack': _ => this.cells[0].role == 'packages',
            'type': [
                _ => this.#type,
                v => this.#type = v || null
            ],
            'clean': _ => {
                for(const cell of this.cells) {
                    if('input' === cell.role) continue;
                    if(!cell.$button('do_not_disturb_off').classList.contains('disabled'))
                        return false;
                }
                return true;
            },
            'cells': _ => [...this.querySelectorAll('nj-cell')],
            'kernel': _ => window.connection.get(this),
            'renderers': _ => {
                let s = new Set();
                for(const cell of this.cells) {
                    s = s.union(new Set(cell.render));
                }
                return [...s];
            },
            'slides': _ => {
                for(const cell of this.cells) {
                    if(cell.slides) return true;
                }
                return false;
            },
            '.Connected': _ => {
                this.cells.forEach(c => c.$connected());
            },
            '.Disconnected': _ => {
                this.cells.forEach(c => c.$disconnected());
            }
        });
        window.connection.add(this);
    }
    eval(cid, code) {
        return window.connection.run(code, this.kernel, cid)
            .then(output => [output.text.trim(), output.time, cid]);
    }
    find(name) {
        for(const cell of this.cells)
            if(cell.name == name) return cell;
    }
    input() {
        const c = $('nj-cell');
        c.role = 'input';
        this.prepend(c);
    }
    remove(el) {
        if(typeof(el) == 'undefined') super.remove();
        else {
            el.remove();
            for (const c of this.querySelectorAll('nj-cell'))
                if ('input' !== c.role) return;
            this.appendChild(document.createElement('nj-cell'));
        }
    }
    toggle() {
        for(const cell of this.cells) cell.$button('dock_to_left')?.click();
    }
    $mount() {
        if(this.#timeout) {
            clearTimeout(this.#timeout);
            this.#timeout = null;
        }
    }
    $unmount() {
        this.#timeout = setTimeout(() => window.connection.del(this), 500);
    }
    status() {
        const k = this.kernel;
        k ? this.$dispatch(window.connection.constructor.Connected, {kernel: k})
          : this.$dispatch(window.connection.constructor.Disconnected, {kernel: null, reason: 'unknown'});
        const i = this.cells[0].role == 'input';
        const p = this.cells[0].role == 'packages';
        this.$dispatch(NJBook.Clean, {is: this.clean});
        this.$dispatch(NJBook.Input, {removed: !this.type && !i});
        this.$dispatch(NJBook.Packages, {is: p});
    }
    restart(e) {
        if(e === 'kernel' || e.target?.dataset.target === 'kernel') {
            this.$dispatch(window.connection.constructor.Disconnected, {kernel: this.kernel, reason: 'restart'});
            window.connection.del(this).then((r) => window.connection.add(this));
        }
        else if(e === 'context' || e.target?.dataset.target == 'context') {
            this.$dispatch(window.connection.constructor.Disconnected, {kernel: this.kernel, reason: 'restart'});
            fetch(`${window.connection.server}k/${window.connection.uuid}`, {
                method: 'PUT', body: new String(this.kernel)
            }).then((r) => {
                if(200 == r.status) this.$dispatch(window.connection.constructor.Connected, {kernel: this.kernel});
            });
        }
    }
}

class NJCell extends $.Element {
    #editor = null;
    #errors = null;
    #layout = null;
    #output = null;
    #render = [];
    #widget = [];

    #body = null;
    #name = null;
    #time = null;

    constructor() {
        super({
            'body': [
                _ => this.#body,
                v => this.#body = v
            ],
            'edit': _ => this.#editor,
            'name': [
                _ => {
                    return this.#name.textContent;
                },
                v => {
                    if(v !== null) throw new Error();
                    this.#name.textContent = '';
                }
            ],
            'book': _ => this.parentElement,
            'code': [
                _ => {
                    if('input' === this.role) {
                        if(this.$button('data_object'))
                            return `export const input = ${this.edit.getValue()};`;
                        return `export const input = ${JSON.stringify(this.edit.getValue())};`
                    }
                    if('packages' === this.role) {
                        return `import { packages } from 'noto:noto'; packages(${JSON.stringify(this.edit.getValue())});`
                    }
                    return this.#editor.getValue();
                },
                v => {
                    if('handler' === this.role) {
                        CodeMirror.notojs.app.code(this.#editor, v);
                    } else {
                        this.#editor.setValue(v);
                    }
                }
            ],
            'role': [
                _ => this.getAttribute('nj-role') || 'generic',
                v => this.setAttribute('nj-role', v)
            ],
            'active': [
                _ => !this.hasAttribute('nj-inactive'),
                v => v
                    ? (_ => {
                        this.removeAttribute('nj-inactive');
                        if(this.book.kernel) this.$button('play_circle').classList.remove('disabled');
                    })()
                    : (_ => {
                        this.setAttribute('nj-inactive', '');
                        this.$button('play_circle').classList.add('disabled');
                        this.#time.innerText = null;
                        this.name = null;
                        this.clean();
                        this.clear();
                        if ('input' === this.role && this.book.kernel)
                            this.book.eval(this.#get_cellid(), 'delete globalThis.input').catch(_ => {});
                    })()
            ],
            'render': _ => new Set(this.#render),
            'slides': _ => {
                return !!this.querySelector('nj-view');
            }
        });

        this.#layout = $('div');

        const tbar = $('div');
        this.#layout.appendChild(tbar);

        const play = $('span.material-symbols-outlined');
        play.textContent = 'play_circle';
        play.addEventListener('click', this.play.bind(this));

        const cout = $('span.material-symbols-outlined.disabled');
        cout.textContent = 'do_not_disturb_off';

        this.#name = $('span');
        this.#time = $('span');

        const L = $('div');
        L.append(play);
        L.append(cout);
        L.append(this.#name);

        const a = $('span.material-symbols-outlined');
        a.textContent = 'add_row_above';
        a.addEventListener('click', e => {
            if(a.classList.contains('disabled')) return;
            this.book.insertBefore(document.createElement('nj-cell'), this).focus();
            this.book.$dispatch(NJBook.Changed, {source: 'cell-add'});
        });

        const b = $('span.material-symbols-outlined');
        b.textContent = 'add_row_below';
        b.addEventListener('click', e => {
            if(b.classList.contains('disabled')) return;
            const n = this.nextSibling;
            if(n) this.book.insertBefore(document.createElement('nj-cell'), n).focus();
            else this.book.appendChild(document.createElement('nj-cell')).focus();
            this.book.$dispatch(NJBook.Changed, {source: 'cell-add'});
        });

        const d = $('span.material-symbols-outlined');
        d.textContent = 'delete';
        d.addEventListener('click', e => {
            if(d.classList.contains('disabled')) return;
            const book = this.book;
            book.remove(this);
            if('input' === this.role) {
                book.$dispatch(NJBook.Input, {removed: true});
                if(book.kernel)
                    book.eval(this.#get_cellid(), 'delete globalThis.input').catch(_ => {});
            }
            book.$dispatch(NJBook.Changed, {source: 'cell-del'});
        });

        const z = $('span.material-symbols-outlined');
        z.textContent = 'text_select_move_up';
        z.addEventListener('click', e => {
            if (z.classList.contains('disabled')) return;
            this.previousElementSibling.before(this);
            this.book.toggle();
            this.book.$dispatch(NJBook.Changed, {source: 'cell-mov'});
        });

        const y = $('span.material-symbols-outlined');
        y.textContent = 'text_select_move_down';
        y.addEventListener('click', e => {
            if (y.classList.contains('disabled')) return;
            this.nextElementSibling.after(this);
            this.book.toggle();
            this.book.$dispatch(NJBook.Changed, {source: 'cell-mov'});
        });

        const x = $('span.material-symbols-outlined');
        x.textContent = 'label';
        x.addEventListener('click', e => {
            if(x.classList.contains('disabled')) return;
            if('label' === x.textContent) {
                x.textContent = 'label_off';
                this.#del_errors();
                this.active = false;
            } else {
                x.textContent = 'label';
                this.active = true;
            }
            this.book.toggle();
            this.book.$dispatch(NJBook.Changed, {source: 'cell-act'});
        });

        const r = $('span.material-symbols-outlined');
        const p = [this.#time, a, b, r];
        const s = [d, z, y, x, r];

        r.textContent = 'dock_to_right';
        r.addEventListener('click', e => {
            if(r.classList.contains('disabled')) return;
            if('dock_to_right' === r.textContent) {
                this.book.toggle();
                r.textContent = 'dock_to_left';
                R.replaceChildren(...s);
                this.#set_state(s);
            } else {
                r.textContent = 'dock_to_right';
                R.replaceChildren(...p);
            }
        });

        const R = $('div');
        R.append(...p);

        tbar.appendChild(L);
        tbar.appendChild(R);
        this.#output = $('div.nj-output');
        this.#layout.appendChild(this.#output);
    }
    play(o) {
        this.clear();
        this.book.toggle();
        if(!NJBook._disconnected && (!this.book.kernel || !this.$button('play_circle')))
            return new Promise(a => a({r: !this.#output.querySelector('.nj-error'), c: this}));
        this.$button('play_circle').textContent = 'hourglass';
        return this.book.eval(this.#get_cellid(), this.code).then(([body, time, name]) => {
            if(time > 1000) this.#time.textContent = parseInt(time / 1000.0).toFixed(1) + 's';
            else this.#time.textContent = time + 'ms';
            const changed = (() => {
                if('handler' === this.role) return false;
                if('lib' === this.book.type && this === this.book.firstChild) return false;
                return true;
            })() && (0 != (this.#body || '').localeCompare(body));
            this.#name.textContent = name;
            this.#body = body;

            this.#del_errors();
            this.#output.innerHTML = '';
            if('packages' === this.role) this.#output.style.setProperty('visibility', 'hidden');
            this.$button('hourglass').textContent = 'play_circle';
            return NJCell.render(this.#output, body).then(objects => {
                this.#render = objects.shift();
                if(changed) this.book.$dispatch(NJBook.Changed, {source: 'run', cell: this});
                const ok = this.#add_errors();
                if(this.role !== 'packages') {
                    if(this.role !== 'handler' && this.role !== 'input') {
                        this.$button('do_not_disturb_off').classList.remove('disabled');
                        if(true === o?.widgets) {
                            const r = this.#output.getBoundingClientRect();
                            if(r.top < 0 || r.top + 50 > window.innerHeight) {
                                const node = document.createElement('div');
                                node.classList.add('jump-to-output');
                                const { line } = this.#editor.getCursor();
                                node.innerHTML = 'output &#8595;';
                                node.addEventListener('click', () => {
                                    while(this.#widget.length) this.#widget.pop().clear();
                                    const pad = window.innerWidth > ($('nj-head')?.constructor.MAX_WIDTH ?? 0) ? 15 : 5;
                                    window.scrollTo({top: this.#output.getBoundingClientRect().top + window.pageYOffset - pad});
                                });
                                this.#widget.push(this.#editor.addLineWidget(line, node, {
                                    above: false,
                                    coverGutter: false
                                }));
                            }
                        }
                    }
                } else if(ok) {
                    this.book.restart('kernel');
                }
                return new Promise(a => a({r: ok, c: this}));
            });
        });
    }
    open() {
        return this.#body ? NJCell.render(this.#output, this.#body).then(objects => {
            this.#render = objects.shift();
            const ok = this.#add_errors();
            this.$button('do_not_disturb_off').classList.remove('disabled');
            return new Promise(a => a({r: ok, c: this}));
        }) : new Promise(a => a({r: true, c: this}));
    }
    clear() {
        for(const w of this.#widget) w.clear();
        this.#widget = new Array();
    }
    focus() {
        this.#editor.focus();
        let node = window.document.getSelection().focusNode;
        if(node.nodeType != document.ELEMENT_NODE) node = node.parentElement;
        const y = node.getBoundingClientRect().y + window.pageYOffset;
        window.scrollTo({top: y - window.innerHeight / 2});
    }
    preview(on) {
        this.#layout.querySelectorAll(':scope > div').forEach(e => {
            if(on ^ e.classList.contains('preview')) e.classList.toggle('preview')
        });
    }
    #add_errors() {
        const err = this.#output.querySelector('.nj-error');
        if(!err || !this.name) return !err;

        this.#errors = new Array();
        if('packages' === this.role) {
            const error = err.textContent.trim().split('\n')[0];
            const e = error.substring(0, error.lastIndexOf('@'));
            const l = error.substring(error.lastIndexOf('@') + 1);

            const marker = document.createElement("div");
            marker.className = "cm-gutter-error";
            marker.innerHTML = "&#x25CF;";

            this.#errors.push({cell: this.name, line: this.edit.getLineHandle(parseInt(l) - 1)});
            this.edit.setGutterMarker(parseInt(l) - 1, 'CodeMirror-linenumbers', marker);
            err.textContent = e;
            this.#output.style.removeProperty('visibility');
        } else {
            const stack = err.textContent.trim().split('\n')
                .map(x => x.trim())
                .filter(x => x.startsWith('at'))
                .reverse();

            if(stack.length > 0) for(const s of stack) {
                const match = s.match(/^\s*at\s+(?:(.+?)\s+\()?(.+):(\d+):(\d+)\)?$/);
                if(!match) continue;

                const [,,file,line,] = match;
                const cell = this.book.find(file);
                if(!cell) continue;

                const marker = document.createElement("div");
                marker.className = "cm-gutter-error";
                marker.innerHTML = (cell == this ? "&#x25CF;" : "&#9675;");

                this.#errors.push({cell: cell.name, line: cell.edit.getLineHandle(parseInt(line) - 1)});
                cell.edit.setGutterMarker(parseInt(line) - 1, 'CodeMirror-linenumbers', marker);
            }
        }
        return false;
    }
    #del_errors() {
        for(let e of (this.#errors || [])) {
            const c = this.book.find(e.cell);
            if(!c) continue;

            const l = c.edit.getLineNumber(e.line);
            if(l == null) continue;

            c.edit.setGutterMarker(l, 'CodeMirror-linenumbers', null);
        }
        this.#errors = null;
    }
    #get_cellid() {
        if(this.name) return this.name;
        if('input' == this.role) return 'input';
        let max = -1;
        for(const c of this.book.cells) {
            if(!c.name.length || c.name == 'input') continue;
            max = Math.max(max, parseInt(c.name.split('-')[1]));
        }
        return `cell-${new String(max+1).padStart(3,'0')}`;
    }
    #set_state(buttons) {
        const toggle = (b, s) => s ? b.classList.remove('disabled') : b.classList.add('disabled');
        if ('lib' === this.book.type) {
            if (!this.previousElementSibling) {
                toggle(buttons[3], false);
                toggle(buttons[1], false);
                toggle(buttons[2], this.nextElementSibling?.active);
            }
            else if (!this.previousElementSibling.previousElementSibling) {
                toggle(buttons[1], this.active);
                toggle(buttons[2], this.nextElementSibling);
                toggle(buttons[3], true);
            } else {
                toggle(buttons[1], true);
                toggle(buttons[3], true);
                toggle(buttons[2], this.nextElementSibling);
            }
        } else if (this.book.pack) {
            toggle(buttons[1], false);
            toggle(buttons[2], false);
            toggle(buttons[3], false);
        } else if ('input' !== this.role) {
            toggle(buttons[1], this.previousElementSibling && 0 > ['handler', 'input', 'packages'].indexOf(this.previousElementSibling.role));
            toggle(buttons[2], this.nextElementSibling && 0 > ['handler', 'packages'].indexOf(this.nextElementSibling.role));
        } else {
            toggle(buttons[1], false);
            toggle(buttons[2], false);
        }
        if(this.active) buttons[3].textContent = 'label';
        else buttons[3].textContent = 'label_off';
    }
    next(code) {
        if(undefined !== code) {
            this.$button('add_row_below').click();
            this.nextSibling.code = code.trim();

            this.nextSibling.play().then(() => {
                const q = this.nextSibling.querySelector('div.nj-output');
                const y = q.getBoundingClientRect().top + window.pageYOffset - 8;
                window.scrollTo({top: y, behavior: 'smooth'});
            });
        } else {
            if(this.nextSibling) this.nextSibling.focus();
            else this.$button('add_row_below').click();
        }
    }
    prev() {
        if(this.previousSibling) this.previousSibling.focus();
        else this.$button('add_row_above').click();
    }
    $mount() {
        this.appendChild(this.#layout);
        if(!this.#editor) {
            const code = document.createElement('div');
            const mode = (this.role == 'input' ? null : (this.role == 'packages' ? 'pkg' : 'js-md'));
            this.#layout.prepend(code);

            this.#editor = CodeMirror(code, {
                mode,
                tabSize: 2,
                indentUnit: 2,
                keyMap: 'default',
                lineNumbers: true,
                indentWithTabs: false,
                matchBrackets: mode == 'js-md',
                value: (this.role == 'handler' ? '  ' : ''),
                inputStyle: (this.role == 'handler' ? 'textarea' : 'contenteditable'),
                extraKeys: mode == 'js-md' ? CodeMirror.notojs.edit.extraKeys : {}
            });
            if(mode == 'js-md') CodeMirror.notojs.edit.init(this.#editor);
            this.#editor.on('blur', _ => {
                this.book?.$dispatch(NJBook.Blurred, {e: this.#editor, s: this.#editor.listSelections(), timeStamp: Date.now()});
                this.clear();
            });
            this.#editor.on('focus', _ => {
                this.book.toggle();
            });
            this.#editor.on('change', (_, e) => {
                if(e.origin != 'setValue') this.book.$dispatch(NJBook.Changed, {source: 'editor', cell: this});
                this.clear();
            });
            if(this.role === 'input') {
                this.$button('add_row_above').classList.add('disabled');
                this.$button('add_row_below').classList.add('disabled');
                this.$button('do_not_disturb_off').textContent = 'article';
                this.$button('article').classList.remove('disabled');
                this.$button('article').addEventListener('click', (e) => {
                    if(e.target.textContent == 'article') {
                        e.target.textContent = 'data_object';
                    } else {
                        e.target.textContent = 'article';
                    }
                });
            }
            else if (this.book.pack) {
                if('packages' === this.role) {
                    this.$button('do_not_disturb_off').classList.add('disabled');
                    this.$button('add_row_above').classList.add('disabled');
                    this.$button('dock_to_right').classList.add('disabled');
                }
            } else {
                this.$button('do_not_disturb_off').addEventListener('click', e => {
                    this.clean();
                });
                if(this.role === 'handler') {
                    this.$button('do_not_disturb_off').classList.add('disabled');
                    this.$button('dock_to_right').classList.add('disabled');
                    CodeMirror.notojs.app.init(this.#editor);
                }
            }
        }
        if(!this.book.kernel) this.$button('play_circle').classList.add('disabled');
    }
    clean() {
        const b = this.$button('do_not_disturb_off');
        if(!b || b.classList.contains('disabled')) return;
        this.book.toggle();
        this.$button('do_not_disturb_off').classList.add('disabled');
        this.#output.innerHTML = '';
        this.#del_errors();
        this.#body = null;
        this.book.$dispatch(NJBook.Changed, {source: 'clean', cell: this});
    }
    $unmount() {
        this.#layout.remove();
    }
    $connected() {
        if(this.active) this.$button('play_circle', this.#layout)?.classList.remove('disabled');
    }
    $disconnected() {
        this.$button('play_circle', this.#layout)?.classList.add('disabled');
        this.name = null;
    }
}

Object.defineProperty(NJCell, 'ready', {
    set(v) {
        if (this._render) v();
        else this._ready = v;
   }
});

Object.defineProperty(NJCell, 'render', {
    get() {
        return this._render;
    },
    set(v) {
        this._render = v;
        if (this._ready) {
            this._ready();
            delete this._ready;
        }
    }
});

Object.defineProperty(NJBook, '_disconnected', {
    get() {
        return typeof(this.__disconnected) == 'undefined' ? false : this.__disconnected;
    },
    set(v) {
        this.__disconnected = v;
    }
});

customElements.define('nj-book', NJBook);
customElements.define('nj-cell', NJCell);
