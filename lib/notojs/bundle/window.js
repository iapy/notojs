import Headroom from './headroom.js';

class NJHead extends $.Element {
    static HEIGHT = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--nj-header-height'));
    static MAX_WIDTH = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--nj-max-header-width'));

    static fix(f) {
        return function(...args){
            return f(NJHead.fix(f), ...args);
        };
    };

    #bevent = null;
    #header = null;
    #layout = null;
    #locked = false;
    #server = document.querySelector('head > base').getAttribute('href');

    constructor() {
        super({
            'book': _ => document.querySelector('body > nj-book'),
            'side': _ => document.querySelector('body > nj-side'),
            '$Input': (e, target) => {
                if(target === this.book) {
                    if(e.removed) this.$button('draft_orders').classList.remove('disabled');
                    else this.$button('draft_orders').classList.add('disabled');
                }
            },
            '$Blurred': (e, target) => {
                if(target === this.book) {
                    this.#bevent = e;
                }
            },
            '$Clean': (e, target) => {
                if(target === this.book) {
                    if(e.is) this.$button('do_not_disturb_off').classList.add('disabled');
                    else this.$button('do_not_disturb_off').classList.remove('disabled');
                }
            },
            '$Changed': (e, target) => {
                if(target.pack || e.cell?.role === 'input') return;
                target.link.text.style.setProperty('font-style', 'italic');
                if(target == this.book) {
                    if(e.source == 'run')
                        this.$button('do_not_disturb_off').classList.remove('disabled');
                    else if((e.source == 'clean' || e.source == 'cell-del') && this.book.clean)
                        this.$button('do_not_disturb_off').classList.add('disabled');
                }
            },
            '$Packages': (e, target) => {
                if(target === this.book) {
                    if(e.is) this.querySelector('div > div:nth-child(2)').style.setProperty('visibility', 'hidden');
                    else this.querySelector('div > div:nth-child(2)').style.removeProperty('visibility');
                }
            },
            '$Connected': (e, target) => {
                if(target === this.book) {
                    this.$button('play_circle').classList.remove('disabled');
                    if('noto:doc' == this.book.cells[0].id)
                    {
                        this.book.cells[0].code = "import doc from 'noto:doc';\nprint(doc());";
                        this.book.cells[0].removeAttribute('id');
                        if (1 == document.querySelector('body').style.opacity)
                            NJCell.ready = () => setTimeout(() => this.book.cells[0].play(), 250);
                        else document.addEventListener('DOMContentLoaded', () => {
                            NJCell.ready = () => setTimeout(() => this.book.cells[0].play(), 250);
                        });
                    }
                }
                target.link.menu.classList.remove('disabled');
                target.link.info = `[${e.kernel}]`;
            },
            '$Disconnected': (e, target) => {
                if(target === this.book) {
                    this.$button('play_circle').classList.add('disabled');
                }
                target.link.menu.classList.add('disabled');
                target.link.info = null;
            }
        });
        this.#layout = $('div');

        const side = $('span.material-symbols-outlined');
        side.textContent = 'menu';
        side.addEventListener('click', this.#side.bind(this));

        const pall = $('span.material-symbols-outlined.disabled');
        pall.textContent = 'play_circle';
        pall.addEventListener('click', this.#pall.bind(this));

        const cout = $('span.material-symbols-outlined.disabled');
        cout.textContent = 'do_not_disturb_off';
        cout.addEventListener('click', this.#cout.bind(this));

        const togg = $('span.material-symbols-outlined');
        togg.textContent = 'article';
        togg.addEventListener('click', this.#togg.bind(this));

        const iput = $('span.material-symbols-outlined');
        iput.textContent = 'draft_orders';
        iput.addEventListener('click', this.#iput.bind(this));;

        const html = $('span.material-symbols-outlined');
        html.textContent = 'download_for_offline';
        html.style.display = 'none';
        html.addEventListener('click', this.#html.bind(this));

        const L = $('div');
        L.appendChild(side);

        const R = $('div');
        R.appendChild(html);
        R.appendChild(iput);
        R.appendChild(pall);
        R.appendChild(cout);
        R.appendChild(togg);

        this.#layout.appendChild(L);
        this.#layout.appendChild(R);
    }

    save(b, f=false) {
        if(b.name && b.name != b.link.title && !f) {
            fetch(`${this.#server}f/${b.name}.notojs`, {method: 'DELETE'}).then(r => {
                if(200 == r.status || 404 == r.status) this.save(b, true);
            });
        } else {
            b.name = document.title = b.link.title;
            fetch(`${this.#server}f/${b.name}.notojs`, { method: 'PUT', body: b.json }).then(r => {
                for (const link of document.querySelector('nj-side').section('Opened').querySelectorAll('nj-link')) {
                    if (link.book.name === b.name && link.book !== b) link.remove();
                }
                b.link.text.style.removeProperty('font-style');
                this.#side();
            });
        }
    }

    #html(e) {
        if(e.target.classList.contains('disabled')) return;
        this.$button('code_blocks').classList.add('disabled');
        e.target.textContent = 'hourglass';
        Promise.all([
            fetch(`${this.#server}notojs.js`).then(r => r.text()),
            fetch(`${this.#server}notojs.css`).then(r => r.text()),
            ...this.book.renderers.map(r => fetch(`${this.#server}${r}`).then(r => r.text()))
        ]).then(([js, css, ...rs]) => {
            const u = URL.createObjectURL(new Blob([[
                '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">',
                '<html><head><meta charset="utf-8"/><meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1 user-scalable=no"/>',
                '<style>', css, 'div.nj-block{margin:auto}nj-view{margin-left:auto;margin-right:auto;max-width:var(--nj-max-editor-width)}nj-view>div.nj-block{margin:unset}',
                '</style></head><body class="nj-page"><div class="nj-output"></div></body>',
                '<script type="module">', js, 'window.render=render;window.Handlers=Handlers;</script>',
                ...rs.map(code => [
                    '<script type="module">', code, 'register(window.Handlers);</script>'
                ].join('')),
                '<script type="module">Promise.all([',
                this.book.cells.map(c => c.body).filter(b => b).map(b => JSON.stringify(b)).join(','),
                '].map((d) => window.render(document.querySelector(".nj-output"), d))).then(_ => document.querySelectorAll("nj-view").forEach(e => e.resize()))',
                '</script></html>'
            ].join('')], {type: 'text/html'}));

            const a = document.createElement('a');
            a.href = u;
            a.style.display = 'none';
            a.download = `${this.book.name ?? 'Untitled'}.html`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(() => {
                this.$button('code_blocks').classList.remove('disabled');
                e.target.textContent = 'download_for_offline';
                URL.revokeObjectURL(u);
            }, 0);
        });
    }

    #iput(e) {
        if(e.target.classList.contains('disabled')) return;
        this.book.toggle();
        this.book.input();
        e.target.classList.add('disabled');
        window.scrollTo({top: 0, behavior: 'smooth'});
    }

    #open(e) {
        const link = e.target.closest('nj-link');
        const side = e.target.closest('nj-side');
        side.search({action: 'close'});
        if(e.target.classList.contains('material-symbols-outlined')) {
            const active = link.menu.classList.contains('is-open');
            if(!active) {
                side.querySelectorAll('ul.file').forEach((e) => e.classList.remove('is-open'));
                link.menu.classList.add('is-open');
            } else link.menu.classList.remove('is-open');
        } else {
            const s = e.target.closest('nj-sect').title;
            const n = link.title;
            const i = link.icon;
            link.icon = 'hourglass';
            const p = {'code_blocks':'lib/','webhook':'app/'}[i] || '';
            fetch(`${this.#server}f/${p}${n}.notojs`)
                .then(r => r.json())
                .then(d => {
                    const side = document.querySelector('nj-side');
                    for(const l of side.section('Opened').querySelectorAll('nj-link')) {
                        if(l.book.name === (p+n) && l.icon === i) {
                            l.book.remove();
                            l.remove();
                        }
                    }
                    link.icon = i;
                    side.make(side.section('Opened'), i, d, n);
                });
        }
    }

    #cout(e) {
        if(e.target.classList.contains('disabled')) return;
        for(const c of this.book.cells) c.clean();
    }

    #pall(e) {
        if(e.target.classList.contains('disabled')) return;
        e.target.textContent = 'hourglass';

        const cells = this.book.cells;
        NJHead.fix((self, result) => {
            if(!result.r) {
                e.target.textContent = 'play_circle';
                const q = result.c.querySelector('.cm-gutter-error');
                const y = q.getBoundingClientRect().top + window.pageYOffset;
                window.scrollTo({top: y, behavior: 'smooth'});
                return;
            }
            let c = cells.shift();
            while (c && !c.active) c = cells.shift();

            if(!c) {
                e.target.textContent = 'play_circle';
                if(result.c) {
                    const q = result.c.querySelector('div.nj-output');
                    const y = q.getBoundingClientRect().top + window.pageYOffset - 8;
                    window.scrollTo({ top: y, behavior: 'smooth' });
                }
                return;
            }

            const y = c.getBoundingClientRect().top + window.pageYOffset - 8;
            window.scrollTo({top: y, behavior: 'smooth'});

            if(this.#header && window.innerHeight < document.documentElement.scrollHeight)
                this.#header.unpin();

            c.play().then(self);
        })({r: true});
    }

    #side(e) {
        const side = document.querySelector('nj-side');
        if(e instanceof PointerEvent && this.#bevent && e.timeStamp - this.#bevent.timeStamp < 100) {
            side.selection = this.#bevent;
        }
        this.#bevent = null;
        side.search({action: 'close'});
        fetch(`${this.#server}f/`, {method: 'PROPFIND'}).then(r => r.json()).then(j => {
            const open = this.#open.bind(this);
            const file = this.#file.bind(this);
            for(const s of ['Notebooks',  'Libraries', 'Applications', 'Packages', 'Databases']) {
                const section = side.section(s);
                section.clear();
                if(s !== 'Packages' && s !== 'Databases') j[s.toLowerCase()].sort();
                for(const item of j[s.toLowerCase()]) {
                    if(s === 'Packages') {
                        const link = section.link(item.name, item.type ? 'code' : 'code_blocks');
                        link.dataset.url = item.url;
                        if(item.size) link.info = `[${(item.size / 1024).toFixed(1)} Kb]`;
                    } else if(s === 'Databases') {
                        if(item.name.startsWith('sys:storage:')) {
                            const link = section.link(item.name.substr(12), 'home_storage');
                            link.addEventListener('click', side.lmdb.bind(side));
                            link.info = `[${item.size}]`;
                        }
                        else if(item.name.startsWith('var:')) {
                            const link = section.link(item.name.substr(4), 'database_off');
                            link.addEventListener('click', side.lmdb.bind(side));
                            link.info = `[${item.size}]`;
                        }
                        else {
                            const link = section.link(item.name.substr(4), 'database');
                            link.addEventListener('click', side.lmdb.bind(side));
                            link.info = `[${item.size}]`;
                        }
                    } else {
                        const link = section.link(item, {
                            'Notebooks': 'book',
                            'Libraries': 'code_blocks',
                            'Applications': 'webhook'
                        }[s]);
                        link.menu.className = 'file';
                        let o = link.item();
                        o.textContent = 'Open as new';
                        o.parentElement.dataset.action = 'duplicate';
                        o.parentElement.addEventListener('click', file);
                        let r = link.item();
                        r.textContent = 'Rename';
                        r.parentElement.dataset.action = 'rename';
                        r.parentElement.addEventListener('click', file);
                        let i = link.item();
                        i.textContent = 'Remove';
                        i.parentElement.dataset.action = 'remove';
                        i.parentElement.addEventListener('click', file);
                        link.addEventListener('click', open);
                    }
                }
                if(s === 'Databases')
                {
                    if(!j[s.toLowerCase()].length) section.style.display = 'none';
                    else section.style.display = 'block';
                }
            }
        });
        if(e !== undefined) side.open();
    }

    #file(e) {
        const l = e.target.closest('ul').previousSibling;
        const i = l.icon;
        const n = l.title;
        const p = {'code_blocks':'lib/','webhook':'app/'}[i] || '';
        if(e.target.closest('li').dataset.action === 'remove') {
            fetch(`${this.#server}f/${p}${n}.notojs`, {method: 'DELETE'})
                .then(r => {
                    if(200 == r.status) {
                        const side = document.querySelector('nj-side');
                        for(const l of side
                            .section('Opened')
                            .querySelectorAll('nj-link'))
                        {
                            if(l.book.name == (p+n)) side.close(l);
                        }
                        l.remove();
                    }
                });
        }
        else if(e.target.closest('li').dataset.action === 'duplicate') {
            fetch(`${this.#server}f/${p}${n}.notojs`)
                .then(r => r.json())
                .then(d => {
                    const side = document.querySelector('nj-side');
                    side.make(side.section('Opened'), i, d);
                });
        }
        else if(e.target.closest('li').dataset.action === 'rename') {
            const server = this.#server;
            l.text.setAttribute('contenteditable', 'true');

            const r = document.createRange();
            r.selectNodeContents(l.text);

            const s = window.getSelection();
            s.removeAllRanges();
            s.addRange(r);

            const i = l.icon;
            const n = l.title;
            const p = {'code_blocks':'lib/','webhook':'app/'}[i] || '';
            l.text.addEventListener('keydown', function fn(e){
                if(e.key === 'Enter') {
                    l.text.removeEventListener('keydown', fn);
                    fetch(`${server}f/${p}${n}.notojs`, {method: 'MOVE', body: `${p}${l.title}.notojs`}).then(r => {
                        if(200 == r.status) {
                            const side = document.querySelector('nj-side');
                            for(const m of side.section('Opened').querySelectorAll('nj-link')) {
                                if(m.book.name === (p+n) && m.icon === i) {
                                    m.book.name = l.title;
                                    m.text.innerText = l.title;
                                    break;
                                }
                            }
                        }
                    });
                    l.text.blur();
                }
                else if(e.key === 'Escape') {
                    l.text.removeEventListener('keydown', fn);
                    l.title = n;
                }
            });
            l.text.addEventListener('blur', function fn(e) {
                l.text.removeEventListener('blur', fn);
                l.text.removeAttribute('contenteditable');
                l.text.dispatchEvent(new KeyboardEvent('keydown', {key: 'Enter', keyCode: 13}));
            });
            l.focus();
        }
    }

    #togg(e) {
        if(e.target.classList.contains('disabled')) return;
        if(e.target.textContent == 'article') {
            e.target.textContent = 'code_blocks';
            this.$button('menu').style.display = 'none';
            this.$button('play_circle').style.display = 'none';
            this.$button('draft_orders').style.display = 'none';
            this.$button('do_not_disturb_off').style.display = 'none';
            this.$button('download_for_offline').style.display = '';
            for(const cell of this.book.cells) cell.preview(true);
        } else {
            for(const cell of this.book.cells) cell.preview(false);
            this.$button('menu').style.display = '';
            this.$button('play_circle').style.display = '';
            this.$button('draft_orders').style.display = '';
            this.$button('do_not_disturb_off').style.display = '';
            this.$button('download_for_offline').style.display = 'none';
            e.target.textContent = 'article';
        }
    }

    $mount() {
        this.appendChild(this.#layout);
        fetch(`${this.#server}m`).then(r => r.text()).then(t => {
            let section = '', m;
            const apple = /Mac|iPhone|iPad/.test(navigator.platform);
            let keymap = new Object();
            t.split('\n').forEach(l => {
                if(m = l.match(/^\[(.*)\]$/), m) {
                    section = m[1];
                } else if(m = l.match(/^([\w-]+)\s?=\s?(.*)$/), m) {
                    const b = {a: false, c: false, m: false, s: false};
                    const k = m[2].split(' ');
                    if(k[0] == '-apple') {
                        if(apple) return;
                        else k.shift();
                    }
                    else if(k[0] == '+apple') {
                        if(!apple) return;
                        else k.shift();
                    }
                    while(k.length > 1) {
                        const n = k.shift();
                             if(n == 'alt') b.a = true;
                        else if(n == 'ctrl') b.c = true;
                        else if(n == 'meta') b.m = true;
                        else if(n == 'shift') b.s = true;
                    }
                    if(!(k[0] in keymap)) keymap[k[0]] = new Array();
                    keymap[k[0]].push([section, m[1], b]);
                }
            });
            console.log(keymap);
            const handler = {
                'cell.blur': c => {
                    document.activeElement.blur();
                },
                'cell.clear': c => {
                    c.$button('do_not_disturb_off').click();
                },
                'cell.next': c => {
                    c.next();
                },
                'cell.previous': c => {
                    c.prev();
                },
                'cell.run': c => {
                    c.play({widgets: true});
                },
                'preview.close': _ => {
                    this.#togg({target: this.$button('code_blocks')});
                },
                'preview.slides': _ => {
                    if(document.fullscreenElement) {
                        console.log('Exit fullscreen');
                        document.exitFullscreen();
                    }
                    else this.book.querySelector('div.nj-slide > svg')?.dispatchEvent(new MouseEvent('click', {bubble: false}));
                },
                'sidebar.close': _ => {
                    if(false !== this.side.search({action: 'close'})) return;
                    if(this.side.selection) {
                        this.side.selection.e.focus();
                        this.side.selection = null;
                    }
                    this.side.close();
                },
                'sidebar.close-file': _ => {
                    if(this.side.querySelector('div.search')) return false;
                    for(const b of this.side.querySelectorAll('nj-link span'))
                        if('true' === b.getAttribute('contenteditable')) return false;
                    for(const b of this.side.querySelectorAll('nj-link.active span.material-symbols-outlined'))
                        if(b.textContent.trim() === 'close') b.click();
                },
                'sidebar.force-close': _ => {
                    if(this.side.selection) {
                        this.side.selection.e.focus();
                        this.side.selection = null;
                    }
                    this.side.close();
                },
                'sidebar.restart-context': _ => {
                    const link = this.side.querySelector('nj-link.active + ul span[data-target="context"]');
                    if(link) link.click();
                    this.side.close();
                },
                'sidebar.restart-kernel': _ => {
                    const link = this.side.querySelector('nj-link.active + ul span[data-target="kernel"]');
                    if(link) link.click();
                    this.side.close();
                },
                'sidebar.save': _ => {
                    if(this.side.querySelector('div.search')) return false;

                    const link = this.side.querySelector('nj-link.active');
                    if(link) for(const span of link.querySelectorAll('span')) {
                        if(span.style.fontStyle == 'italic') link.querySelector('span').click();
                    }
                },
                'sidebar.search': _ => {
                    this.side.search({action:'toggle'});
                },
                'toolbar.clear': _ => {
                    this.$button('do_not_disturb_off').click();
                },
                'toolbar.edit': e => {
                    if(e.target.closest('nj-cell')) return false;
                    this.book.cells[0].focus();
                },
                'toolbar.preview': _ => {
                    this.#togg({target: this.$button('article')});
                },
                'toolbar.run': _ => {
                    this.$button('play_circle').click();
                },
                'toolbar.side': e => {
                    if(this.side.opened) this.side.close();
                    else {
                        for(const cell of document.querySelector('nj-head').book.cells) {
                            if(cell.edit.hasFocus()) {
                                this.side.selection = {e: cell.edit, s: cell.edit.listSelections()};
                                document.activeElement.blur();
                                break;
                            }
                        }
                        this.#side(e);
                    }
                }
            };
            document.addEventListener('keydown', e => {
                for(const [s, a, m] of keymap[e.code] || []) {
                    if((m.m ^ e.metaKey) || (m.c ^ e.ctrlKey) || (m.s ^ e.shiftKey) || (m.a ^ e.altKey)) continue;
                    if(s == 'toolbar' && (this.side.opened || !this.$button('article'))) continue;
                    if(s == 'preview' && !this.$button('code_blocks')) continue;
                    if(s != 'preview' && document.fullscreenElement) continue;
                    if(s == 'sidebar' && !this.side.opened) continue;
                    console.log(s, a, m);

                    const c = document.activeElement?.closest('nj-cell');
                    if(s == 'cell' && (!c || this.side.opened || !this.$button('article'))) continue;

                    if(false !== (handler[`${s}.${a}`] ?? (_ => false))(s == 'cell' ? c : e)) {
                        e.preventDefault();
                        break;
                    }
                }
            });
        });
        const mq = window.matchMedia(`(max-width: ${NJHead.MAX_WIDTH}px)`);
        const cb = this.#updateHeader.bind(this);
        if (mq.addEventListener) {
            mq.addEventListener("change", cb);
        } else {
            mq.addListener(cb);
        }
        window.addEventListener("scroll", cb, {
            passive: true
        });
        this.#updateHeader(mq);
        this.#updateHeader();
    }

    #updateHeader(e) {
        if(e === undefined || e.type == "scroll") {
            const y = window.scrollY || 0;
            if (!this.#locked && y <= 2) {
                this.#locked = true;
                this.classList.add("top-lock");
            }
            else if(this.#locked && y >= NJHead.HEIGHT - 8) {
                this.#locked = false;
                this.classList.remove("top-lock");
            }
            this.classList.toggle('headroom--has-shadow', (window.scrollY || 0) > 2);
        } else if(!e.matches) {
            if(this.#header) {
                this.#header.destroy();
                this.#header = null;
            }
        }
        else {
            if(!this.#header) {
                this.#header = new Headroom(this, {
                    offset: 0,
                    tolerance: { up: 0, down: NJHead.HEIGHT / 2 }
                });
                this.#header.init();
            }
        }
    }

    open(b) {
        document.title = b.link.title;
        b.link.info = b.kernel;
        b.status();
    }
}

class NJLink extends $.Element {
    static Close = $.Event('Close');

    #icon;
    #info;
    #menu;
    #text;

    constructor() {
        super({
            'icon': [
                _ => this.#icon.textContent,
                v => this.#icon.textContent = v
            ],
            'info' : [
                _ => this.#info.textContent,
                v => this.#info.textContent = v || ''
            ],
            'menu' : _ => {
                if(!this.#menu) {
                    this.#menu = $('ul');
                    if(this.isConnected) this.after(this, this.#menu);
                }
                return this.#menu;
            },
            'text' : _ => this.#text,
            'close': [
                _ => this.$button('close') !== undefined,
                v => {
                    if(v) {
                        const c = $('span.material-symbols-outlined');
                        c.textContent = 'close';
                        this.appendChild(c);
                    }
                }
            ],
            'title': [
                _ => this.#text.textContent,
                v => this.#text.textContent = v
            ]
        });
        this.#icon = $('span.material-symbols-outlined');
        this.#info = $('span.detail');
        this.#text = $('span');
        this.#menu = null;
    }

    hide() {
        this.style.display = 'none';
        if(this.#menu) this.#menu.style.display = 'none';
    }

    show() {
        this.style.removeProperty('display');
        if(this.#menu) this.#menu.style.removeProperty('display');
    }

    item() {
        const li = $('li');
        const sp = $('span');
        li.appendChild(sp);
        this.menu.appendChild(li);
        return sp;
    }

    $mount() {
        this.prepend(this.#info);
        this.prepend(this.#text);
        this.prepend(this.#icon);
        if(this.#menu) this.after(this.#menu);
    }

    $unmount() {
        if(this.#menu) this.#menu.remove();
    }
}

class NJSect extends $.Element {
    #head;
    #body;

    constructor() {
        super({
            'title': [
                _ => this.#head.querySelector('h1').textContent,
                v => this.#head.querySelector('h1').textContent = v
            ]
        });
        this.#head = $('div');
        this.#head.appendChild($('h1'));
        this.#body = $('div');
    }

    clear() {
        for(const l of this.#body.querySelectorAll('nj-link'))
            l.remove();
        for(const i of this.#body.querySelectorAll('img'))
            i.remove();
    }

    empty() {
        return 0 == this.#body.querySelectorAll('nj-link').length;
    }

    click(t,f) {
        const text = $('span');
        if(f === undefined) {
            text.textContent = 'New';
            text.addEventListener('click', t);
        } else {
            text.textContent = t;
            text.addEventListener('click', f);
        }
        this.#head.appendChild(text);
    }

    link(t, i='link', c=false) {
        const link = $('nj-link');
        link.title = t;
        link.icon = i;
        link.close = c;

        this.#body.appendChild(link);
        return link;
    }

    icon(i) {
        const img = new Image();
        img.src = i;
        img.title = i;
        return this.#body.appendChild(img);
    }

    $mount() {
        this.appendChild(this.#head);
        this.appendChild(this.#body);
    }
}

class NJSide extends $.Element {
    #abort = null;
    #drawer;
    #overlay;
    #selection = null;

    #server = document.querySelector('head > base').getAttribute('href');

    constructor() {
        super({
            'content': _ => this.#drawer.querySelector('div:not(.search):not(.shadow)'),
            'opened': _ => this.#overlay.classList.contains('is-open'),
            'selection': [
                _ => this.#selection,
                v => this.#selection = v
            ]
        });
        this.#drawer = $('aria');
        this.#drawer.setAttribute('aria-hidden', 'true');
        this.#drawer.appendChild($('div.shadow.top'));
        this.#drawer.appendChild($('div'));
        this.#drawer.appendChild($('div.shadow.bottom'));

        this.#overlay = $('div');
        this.#overlay.setAttribute('hidden', '');
        this.#overlay.addEventListener('click', e => {
            this.close();
        });

        let startY = null;
        let pulling = false;

        this.#drawer.addEventListener('touchstart', e => {
            if(this.#drawer.scrollTop === 0) {
                startY = e.touches[0].clientY;
                pulling = true;
            }
        });

        this.#drawer.addEventListener('touchend', e => {
            if(pulling && 0 < (e.changedTouches[0].clientY - startY)) {
                window.scrollTo({top:0});
                this.search({action: 'open', source: 'touch'});
            }
            pulling = false;
            startY = null;
        });
    }

    open() {
        document.querySelectorAll('html, body').forEach(e => {
            e.style.overflow = 'hidden';
        });
        this.#drawer.setAttribute('aria-hidden', 'false');
        this.#drawer.classList.add('is-open');
        this.#overlay.removeAttribute('hidden');
        this.#overlay.classList.add('is-open');
    }

    close(link) {
        this.search({action:'close'});
        if(link === undefined) {
            document.querySelectorAll('html, body').forEach(e => {
                e.style.overflow = '';
            });
            this.#drawer.setAttribute('aria-hidden', 'true');
            this.#drawer.classList.remove('is-open');
            this.#overlay.setAttribute('hidden', '');
            this.#overlay.classList.remove('is-open');
            this.querySelectorAll('ul.file').forEach((e) => e.classList.remove('is-open'));
        } else {
            link.book.remove();
            link.remove();
            const open = this.section('Opened');
            if(!open.querySelector('nj-link')) {
                this.make(open, 'book');
            } else if(link.classList.contains('active')) {
                this.#open(open.querySelector('nj-link').book);
            }
        }
    }

    search(c) {
        let inp = this.#drawer.querySelector('div.search');
        if(('toggle' == c.action || 'open' == c.action) && !inp) {
            this.querySelector('div.shadow.top').style.display = 'none';
            this.querySelector('nj-sect').style.marginTop = '0.5em';
            this.section('Icons').clear();
            inp = $('div.search');
            const s = inp.appendChild($('input'));
            s.setAttribute('type', 'text');
            s.setAttribute('placeholder', 'Search');
            this.#drawer.insertBefore(inp, this.#drawer.firstChild);
            s.addEventListener('focus', (e) => {
                setTimeout(() => e.target.style.caretColor = 'unset', 250);
            });
            s.addEventListener('keyup', (e) => {
                e.preventDefault();
                if(e.key === 'Enter') {
                    const link = [...document.querySelectorAll("nj-link")].filter(e => e.style.display != 'none');
                    if(link) link[0].click();
                } else {
                    this.filter(s.value.toLowerCase());
                }
            });
            s.focus();
        }
        else if(('toggle' == c.action || 'close' == c.action) && inp) {
            inp.remove();
            this.filter();
            if(this.#selection) {
                this.#selection.e.focus();
                this.#selection.e.setSelections(this.#selection.s);
                this.#selection = null;
            }
            this.#selection = null;
            this.querySelector('nj-sect').style.marginTop = '0';
            this.querySelector('div.shadow.top').style.display = '';
        } else return false;
    }

    filter(v) {
        if(!v) {
            for(const l of this.querySelectorAll('nj-link')) {
                l.show();
            }
            for(const s of this.content.querySelectorAll('nj-sect')) {
                if(s.title == 'Opened' || s.title == 'Packages' || (s.title == 'Databases' && !s.empty())) { s.style.display = 'block'; }
                if(s.title == 'Icons') { s.remove(); }
            }
        } else {
            for(const l of this.querySelectorAll('nj-link')) {
                if(l.text.textContent.toLowerCase().indexOf(v) < 0) l.hide();
                else l.show();
            }
            this.section('Opened').style.display = 'none';
            this.section('Packages').style.display = 'none';
            this.section('Databases').style.display = 'none';

            if(this.#selection) {
                if(this.#abort) this.#abort.abort();
                this.#abort = new AbortController();
                fetch('https://api.iconify.design/search?query=' + encodeURIComponent(v), {signal: this.#abort.signal})
                    .then(r => {
                        this.#abort = null;
                        this.section('Icons').clear();
                        r.json().then(j => j.icons.forEach(i => {
                            this.section('Icons').icon(`https://api.iconify.design/${i.replace(':', '/')}.svg`).addEventListener('click', _ => {
                                this.#selection.e.setSelections(this.#selection.s);
                                this.#selection.e.replaceSelection(`await icon('${i.replace(':', '/')}')`);
                                this.#selection.e.focus();
                                this.#selection = null;
                                this.close();
                            });
                        }, _ => {}));
                    }, _ => {});
            }
        }
    }

    lmdb(e) {
        if(this.#selection) {
            const link = e.target.closest('nj-link');
            this.#selection.e.setSelections(this.#selection.s);
            if('home_storage' == link.icon) {
                this.#selection.e.replaceSelection(`new Storage('${link.text.innerText}')`);
            } else if('database_off' == link.icon) {
                this.#selection.e.replaceSelection(`db.open('var:${link.text.innerText}')`);
            } else {
                this.#selection.e.replaceSelection(`db.open('${link.text.innerText}')`);
            }
            this.#selection.e.focus();
            this.#selection = null;
            this.close();
        }
    }

    section(title) {
        for(const s of this.content.querySelectorAll('nj-sect'))
            if(s.title == title) return s;
        const sect = $('nj-sect');
        sect.title = title;
        this.content.appendChild(sect);
        return sect;
    }

    $mount() {
        this.appendChild(this.#drawer);
        this.appendChild(this.#overlay);

        const open = this.section('Opened');
        this.section('Notebooks').click(e => {
            this.make(open, 'book');
        });
        this.section('Libraries').click(e => {
            this.make(open, 'code_blocks');
        });
        this.section('Applications').click(e => {
            this.make(open, 'webhook');
        });
        this.section('Packages').click('Edit', e => {
            let book = document.querySelector('nj-head').book;
            if (book?.pack) {
                this.close();
                return;
            } else if(book) {
                this.#close(book);
                book = null;
            }
            book = [...document.querySelectorAll('nj-tabs > nj-book')].filter(b => b.pack)[0];
            if(book) {
                this.#open(book);
                this.close();
                return;
            }
            book = $('nj-book');
            book.link = open.link('Packages', 'hourglass', true);
            book.link.book = book;
            book.link.classList.add('active');
            book.link.addEventListener('click', e => {
                if(e.target.textContent == 'close') {
                    this.close(book.link);
                } else {
                    if(!book.link.classList.contains('active')) {
                        this.#close(document.querySelector('nj-head').book);
                        this.#open(book);
                    }
                    this.close();
                }
            });
            const cell = $('nj-cell');
            cell.role = 'packages';
            book.appendChild(cell);

            fetch(`${this.#server}p`).then(r => r.text()).then(t => {
                book.link.icon = 'code_blocks';
                cell.code = t;
                this.close();
            });

            document.body.appendChild(book);
            document.querySelector('nj-head').open(book);

            this.#drawer.scrollTop = 0;
            this.#menu(book);
        });
        this.section('Databases');
        this.make(open, 'book');

        if('#doc' === document.location.hash)
        {
            const cell = document.querySelector('nj-cell');
            cell.id = 'noto:doc';
        }
    }

    make(open, type, info=null, name=null) {
        this.#drawer.scrollTop = 0;
        let book = document.querySelector('nj-head').book;
        if(book) { this.#close(book); book = null; }

        if(name || info) for(const b of document.querySelectorAll('nj-book')) {
            if(!b.name && b.link.icon !== 'code_blocks' && b.link.text.style.getPropertyValue('font-style') !== 'italic') {
                for(const cell of b.querySelectorAll('nj-cell')) cell.remove();
                b.link = b.link.remove();
                book = b;
                break;
            }
        }

        book = book || $('nj-book');
        book.type = {'code_blocks': 'lib', 'webhook': 'app'}[type];
        book.link = open.link(name || 'Untitled', (info ? 'hourglass' : type), true);
        document.body.appendChild(book);
        let dirty = false;
        if(info) {
            for(const c of info) {
                const cell = $('nj-cell');
                if('webhook' === type && 'handler' === c.role)
                    cell.role = 'handler';
                book.appendChild(cell);
                cell.code = c.code;
                dirty |= (name && !!(cell.body = c.body));
                if ('disabled' === c.role) cell.active = false;
            }
            if(name) book.name = name;
            else book.link.text.style.setProperty('font-style', 'italic');
        } else {
            const cell = $('nj-cell');
            if('webhook' === type)
                cell.role = 'handler';
            book.appendChild(cell);
        }
        book.link.addEventListener('click', e => {
            if(e.target.textContent == 'close') {
                this.close(book.link);
            } else if(book.link.classList.contains('active')) {
                if(e.target.classList.contains('material-symbols-outlined') && book.name) {
                    document.querySelector('nj-head').save(book);
                } else {
                    this.#edit(book);
                }
            } else {
                this.#close(document.querySelector('nj-head').book);
                this.#open(book);
                this.close();
            }
        });
        book.link.classList.add('active');
        book.link.book = book;
        document.querySelector('nj-head').open(book);
        if(!dirty) document.querySelector('nj-head').$button('do_not_disturb_off').classList.add('disabled');
        else document.querySelector('nj-head').$button('do_not_disturb_off').classList.remove('disabled');

        this.#menu(book);
        if(info) {
            const cells = book.cells;
            NJHead.fix((self, ok) => {
                if(!cells.length) {
                    book.link.icon = type;
                    return this.close();
                }
                const c = cells.shift();
                c.open().then(self);
            })(true);
        } else this.close();
    }

    #close(b) {
        document.querySelector('nj-tabs').appendChild(b);
        b.link.classList.replace('active', 'inactive');
    }

    #menu(b) {
        const e = b.link.item();
        e.textContent = 'Reset context';
        e.dataset.target = 'context';
        e.addEventListener('click', (e) => {
            if(!b.link.menu.classList.contains('disabled')) {
                b.link.menu.classList.add('disabled');
                b.restart(e);
                this.close();
            }
        });
        const k = b.link.item();
        k.dataset.target = 'kernel';
        k.textContent = 'Restart kernel';
        k.addEventListener('click', (e) => {
            if(!b.link.menu.classList.contains('disabled')) {
                b.link.menu.classList.add('disabled');
                b.restart(e);
                this.close();
            }
        });
    }

    #edit(b) {
        b.link.text.setAttribute('contenteditable', 'true');

        const r = document.createRange();
        r.selectNodeContents(b.link.text);

        const s = window.getSelection();
        s.removeAllRanges();
        s.addRange(r);

        const p = b.link.title;
        b.link.text.addEventListener('keydown', function fn(e){
            if(e.key === 'Enter') {
                b.link.text.removeEventListener('keydown', fn);
                if(b.link.title != p) document.querySelector('nj-head').save(b);
                b.link.text.blur();
            }
            else if(e.key === 'Escape') {
                b.link.text.removeEventListener('keydown', fn);
                b.link.title = p;
            }
        });
        b.link.text.addEventListener('blur', function fn(e) {
            b.link.text.removeEventListener('blur', fn);
            b.link.text.removeAttribute('contenteditable');
            b.link.text.dispatchEvent(new KeyboardEvent('keydown', {key: 'Enter', keyCode: 13}));
        });
        b.link.text.focus();
    }

    #open(b) {
        document.body.appendChild(b);
        b.link.classList.replace('inactive', 'active');
        document.querySelector('nj-head').open(b);
    }
}

class NJTabs extends $.Element {}

customElements.define('nj-head', NJHead);
customElements.define('nj-link', NJLink);
customElements.define('nj-sect', NJSect);
customElements.define('nj-side', NJSide);
customElements.define('nj-tabs', NJTabs);

document.addEventListener('DOMContentLoaded', () => {
    document.querySelector('body').style.opacity = 1;
});
