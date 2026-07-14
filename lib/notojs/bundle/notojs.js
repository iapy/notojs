import markdownit from './markdown-it.js';
import tasks from './markdown-tasks.js';
import jsonview from './jsonview.js';
import hljs from './highlight.js';
import moment from './moment.js';

const md = markdownit({
  highlight: (code, lang) => {
    if(lang) {
        const p = lang.split('!');
        if(hljs.getLanguage(p[0])) {
          try {
            return `<pre${'noplay' === p[1] ? ' class="noplay"' : ''}><code class="hljs language-${p[0]}">${hljs.highlight(code,{language:p[0]}).value}</code></pre>`;
          } catch (_) {}
        }
    }
    return `<pre><code>${md.utils.escapeHtml(code)}</code></pre>`;
  }
});

function lastOf(a, x=0) {
    return a[a.length - x - 1];
}

function math(md) {
    function inlineMath(state, silent) {
        const start = state.pos;
        if(state.src[start] !== '$') return false;

        if(state.src[start + 1] === '$') return false;

        let end = start + 1;
        while ((end = state.src.indexOf('$', end)) !== -1) {
            if (state.src[end - 1] !== '\\') break;
            end++;
        }
        if(end === -1) return false;

        const content = state.src.slice(start + 1, end).trim();
        if(!content) return false;

        if(!silent) {
            const token = state.push('math_inline', 'math', 0);
            token.content = content;
        }

        state.pos = end + 1;
        return true;
    }

    function blockMath(state, startLine, endLine, silent) {
        const startPos = state.bMarks[startLine] + state.tShift[startLine];
        const maxPos = state.eMarks[startLine];

        if (state.src.slice(startPos, maxPos).trim() !== '$$') return false;

        let nextLine = startLine + 1;
        let found = false;
        let content = '';

        while(nextLine < endLine) {
            const lineStart = state.bMarks[nextLine] + state.tShift[nextLine];
            const lineEnd = state.eMarks[nextLine];
            const line = state.src.slice(lineStart, lineEnd).trim();

            if (line === '$$') {
                found = true;
                break;
            }
            content += state.src.slice(lineStart, lineEnd) + '\n';
            nextLine++;
        }

        if(!found) return false;
        if(silent) return true;

        const token = state.push('math_block', 'math', 0);
        token.block = true;
        token.content = content.trim();
        token.map = [startLine, nextLine + 1];

        state.line = nextLine + 1;
        return true;
    }

    md.inline.ruler.after('escape', 'math_inline', inlineMath);
    md.renderer.rules.math_inline = (tokens, idx) => '$begin:math:text$' + tokens[idx].content + '$end:math:text$';

    md.block.ruler.before('fence', 'math_block', blockMath, { alt: ['paragraph', 'reference', 'blockquote', 'list'] });
    md.renderer.rules.math_block = (tokens, idx) => '$$begin:math:text$' + tokens[idx].content + '$end:math:text$$';
}

const COLORS = ['#008FFB', '#00E396', '#FEB019', '#FF4560', '#775DD0'];
const is_color = (c) => {
    return c[0] == 'c' && 2 == c.length && '0' <= c[1] && String.fromCharCode('0'.charCodeAt(0)+COLORS.length) >= c[1];
};

function styles(md) {
    const RE = /\s*\{([^}]+)\}\s*$/;
    md.core.ruler.after('inline', 'custom-classes', function (state) {
        let stack = new Array();
        let tokens = state.tokens;
        for(let i = 0; i < tokens.length; ++i) {
            if(tokens[i].type.endsWith('_open')) {
                stack.push(tokens[i]);
            }
            else if(tokens[i].type.endsWith('_close')) {
                stack.pop();
            }
            else if(!tokens[i].children) continue;
            else if(tokens[i].type == 'inline') {
                const last = lastOf(tokens[i].children);
                if(last.tag == 'code' || !RE.test(last.content)) continue;

                const parts = last.content.match(/(?:\.[\w-]+)|(?:[\w-]+\=[^\s}]+)/g);
                if(!parts) continue;

                last.content = last.content.replace(RE, '');

                let list = false;
                if(0 == last.content.length) {
                    if(i > 0 && i < tokens.length) {
                        tokens[i].children.pop();
                        if(tokens[i].children.length) {
                            if(lastOf(tokens[i].children).type == 'softbreak') {
                                tokens[i].children.pop();
                                let j = i + 1;
                                for(j; j < tokens.length; ++j) {
                                    if(tokens[j].type == 'list_item_open') break;
                                    if(tokens[j].type.indexOf('_list_close') > 0) {
                                        list = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                const target = (() => {
                    if(list && stack.length > 2
                            && lastOf(stack,0).type == 'paragraph_open'
                            && lastOf(stack,1).type == 'list_item_open'
                            && lastOf(stack,2).type.indexOf('_list_') > 0
                    ) return lastOf(stack,2);
                    if(stack.length > 1
                        && lastOf(stack,0).type == 'paragraph_open'
                        && (
                            lastOf(stack,1).type == 'list_item_open'
                            || lastOf(stack,1).type == 'blockquote_open'
                        )
                    ) return lastOf(stack,1);
                    return lastOf(stack,0);
                })();

                const classes = [];
                const styles = [];

                for(const part of parts) {
                    if(part.startsWith('.')) {
                        const className = part.slice(1);
                        classes.push(className);
                    } else if(part.includes('=')) {
                        var [key, value] = part.split('=');
                        if(!key || !value) continue;
                        if(key == 'color' && is_color(value))
                            value = COLORS[parseInt(value[1])];
                        styles.push(`${key}:${value}`);
                    }
                }

                var existing = target.attrGet('class');
                if(classes.length) {
                    existing = existing ? `${existing} ${classes.join(' ')}` : classes.join(' ');
                    target.attrSet('class', existing);
                }
                existing = target.attrGet('style');
                if(styles.length) {
                    existing = existing ? `${existing}; ${styles.join('; ')}` : styles.join('; ');
                    target.attrSet('style', existing);
                }
            }
        }
    });
};

md.use(math);
md.use(styles);
md.use(tasks);

function make_block(p, t, c) {
    const block = document.createElement(t || 'div');
    block.className = c ? c.split(' ').map((c) => `nj-${Grid.classname(c)}`).join(' ') : 'nj-block';
    p.append(block);
    return block;
}

function sortKeysDeep(obj) {
    if (Array.isArray(obj)) {
        return obj.map(sortKeysDeep);
    } else if (obj && typeof obj === 'object') {
        return Object.keys(obj)
            .sort()
            .reduce((acc, key) => {
                acc[key] = sortKeysDeep(obj[key]);
                return acc;
            }, {});
    }
    return obj;
}

class NJView extends HTMLElement {
    static PADDING = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--nj-block-padding'));
    static MIN_WIDTH = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--nj-min-editor-width'));
    static MAX_WIDTH = parseInt(getComputedStyle(document.documentElement).getPropertyValue('--nj-max-editor-width'));

    static #robserver = new ResizeObserver((entries) => {
        for(const e of entries) NJView.#resize(e.target);
    });

    #button = null;
    #current = null;
    #mobserver = null;
    static #resize(target) {
        requestAnimationFrame(_ => {
            if(target.tagName.toLowerCase() != 'nj-view')
                target = target.closest('nj-view');
            if(!target) return;

            const block = target.querySelector('div.nj-block');
            if(!block) return;

            const h = block.clientHeight;
            const w = target.clientWidth;
            const eh = target.clientHeight;

            const small = w < NJView.MAX_WIDTH;
            const scale = w / ((small ? block.clientWidth : NJView.MAX_WIDTH) + 2 * NJView.PADDING);
            const margin = 0.5 * (eh - h * scale);

            const button = target.querySelector('div.nj-slide');
            const offset = button ? button.clientHeight : 0;

            block.style.marginTop = `${margin-offset}px`;
            block.style.transform = `scale(${scale})`;
        });
    }

    connectedCallback() {
        if(this.parentElement.tagName.toLowerCase() == 'body') {
            this.style.display = 'none';
            document.addEventListener('fullscreenchange', _ => {
                if(document.fullscreenElement == this) {
                    while(this.#current.lastChild && this.#current.lastChild.className != 'nj-slide')
                        this.insertBefore(this.#current.lastChild, this.firstChild);
                    this.style.display = '';
                } else {
                    this.style.display = 'none';
                    while(this.firstChild) this.#current.appendChild(this.firstChild);
                }
            });
            document.addEventListener('keyup', e => {
                if(e.metaKey || e.ctrlKey || e.altKey || document.fullscreenElement !== this) return;
                let slide = null;
                if(('ArrowRight' === e.key || ' ' === e.key) && !e.shiftKey) slide = this.#slide(1);
                else if('ArrowLeft' === e.key || (' ' === e.key && e.shiftKey)) slide = this.#slide(-1);
                if(!slide) return; while(this.firstChild) this.#current.appendChild(this.firstChild);
                this.#current = slide;
                while(this.#current.lastChild && this.#current.lastChild.className != 'nj-slide')
                    this.insertBefore(this.#current.lastChild, this.firstChild);
            });
        } else if('function' === typeof(document.body.requestFullscreen)) {
            this.#button = document.createElement('div');
            this.#button.className = 'nj-slide';
            this.#button.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path fill="currentColor" d="M20 20v.5h.5V20zm-4.646-5.354a.5.5 0 0 0-.708.708zM19.5 14v6h1v-6zm.5 5.5h-6v1h6zm.354.146l-5-5l-.708.708l5 5zM4 20h-.5v.5H4zm5.354-4.646a.5.5 0 0 0-.708-.708zM3.5 14v6h1v-6zm.5 6.5h6v-1H4zm.354-.146l5-5l-.708-.708l-5 5zM20 4h.5v-.5H20zm-5.354 4.646a.5.5 0 0 0 .708.708zM20.5 10V4h-1v6zM20 3.5h-6v1h6zm-.354.146l-5 5l.708.708l5-5zM4 4v-.5h-.5V4zm4.646 5.354a.5.5 0 1 0 .708-.708zM4.5 10V4h-1v6zM4 4.5h6v-1H4zm-.354-.146l5 5l.708-.708l-5-5z"/></svg>`;
            this.#button.firstChild.addEventListener('click', _ => {
                if(!document.querySelector('body > nj-view'))
                    document.body.appendChild(document.createElement('nj-view'));
                document.querySelector('body > nj-view').present(this);
            });
            this.insertBefore(this.#button, this.firstChild);
        }
        this.#mobserver = new MutationObserver(_ => NJView.#resize(this));
        this.#mobserver.observe(this, {childList: true, subtree: true})
        NJView.#robserver.observe(this);
    }
    disconnectedCallback() {
        NJView.#robserver.unobserve(this);
        this.#mobserver.disconnect();
        if(this.#button) this.#button.remove();
    }
    present(el) {
        this.#current = el;
        this.requestFullscreen();
    }
    resize() {
        NJView.#resize(this);
    }
    #slide(inc) {
        let book = this.#current.closest('nj-book');
        if(!book) book = document.body;

        const slides = [...book.querySelectorAll('nj-view')];
        let index = slides.indexOf(this.#current) + inc;
        return slides[index] == this ? null : slides[index];
    }
};

export const Handlers = {
    "notojs.HTML": (grid, part) => {
        const html = (e => {
            if(!(e instanceof HTMLTableCellElement)) return e;
            const d = document.createElement('div');
            e.appendChild(d); return d;
        })(grid.get('html')); if(!html) return;
        const root = html.attachShadow({ mode: 'open' });
        root.innerHTML = `<style>${Handlers[".css"]}</style>${part.data}`;
        for(const svg of html.shadowRoot.querySelectorAll('svg')) {
            if(svg.viewBox.baseVal.width >= NJView.MIN_WIDTH) {
                html.classList.add('nj-svg');
                break;
            }
        }
        for(const img of html.shadowRoot.querySelectorAll('img')) {
            img.onload = img.onerror = _ => {
                console.log('Image resolved', img);
                html.closest('nj-view')?.resize();
            };
        }
    },
    "notojs.Image": (grid, part) => {
        const image = grid.get('image'); if(!image) return;
        const inner = document.createElement('img');
        inner.onload = inner.onerror = _ => {
            console.log('Image resolved', inner);
            inner.closest('nj-view')?.resize();
        };
        inner.setAttribute('src', part.data);
        image.appendChild(inner);
    },
    "notojs.Markdown": (grid, part) => {
        const html = grid.get('html markdown'); if(!html) return;
        html.innerHTML = md.render(part.data);
        html.querySelectorAll('a').forEach(a => {
            a.target = '_blank';
        });
        html.querySelectorAll('li code').forEach(e => {
            const n = e.closest('nj-cell');
            if (n) {
                const p = e.parentElement?.parentElement?.previousElementSibling;
                if (p && 'H6' === p.nodeName && 'See also' === p.textContent && /^doc\(\'.*\'\)$/.test(e.textContent.trim())) {
                    e.parentElement.className = 'nj-see-also';
                    e.parentElement.onclick = () => {
                        n.next(`import doc from 'noto:doc';
print(${e.textContent.trim()});`);
                    }
                }
            }
        });
        html.querySelectorAll('pre:not(.noplay) code:is(.language-js, .language-javascript)').forEach(c => {
            const n = c.closest('nj-cell');
            if(n) {
                const d = document.createElement('div');
                d.className = 'nj-run-code';
                d.appendChild(document.createElement('div')).textContent = 'Run code';
                c.closest('pre').insertAdjacentElement('afterend', d);
                d.onclick = e => {
                    const r = d.querySelector('div').getBoundingClientRect();
                    if(e.offsetX < r.x + r.width) n.next(c.innerText);
                };
            }
        });
        return new Promise((resolve, reject) => {
            if(MathJax.typesetPromise)
                MathJax.typesetPromise([html]).then(resolve, reject);
            else MathJaxPending.push(()=> {
                MathJax.typesetPromise([html]).then(resolve, reject);
            });
        });
    },
    "notojs.XML": (grid, part) => {
        const xml = grid.get('xml'); if(!xml) return;
        const pre = document.createElement('pre');
        const cod = document.createElement('code');
        xml.appendChild(pre);
        pre.appendChild(cod);
        cod.classList.add('hljs', 'language-xml');
        cod.innerHTML = hljs.highlight(part.data, { language: 'xml' }).value;
    },
    render: (d, o) => Handlers[o.data !== undefined && o.type in Handlers ? o.type : '.obj']({
        get: c => (d.className = c.split(' ').map(c => `nj-${Grid.classname(c)}`).join(' '), d)
    }, o),
    ".obj": (grid, data) => {
        const el = grid.get('text json'); if(!el) return;
        jsonview.render(jsonview.create(sortKeysDeep(data)), el);
        el.querySelector('div.caret-icon')?.click();
    },
    ".css": null
};

document.addEventListener('DOMContentLoaded', () => {
    Handlers['.css'] = ((prefix) => {
        const out = [];
        for(const sheet of Array.from(document.styleSheets)) {
            let rules;
            try {
                rules = sheet.cssRules;
            } catch {
                continue;
            }

            for (const rule of Array.from(rules)) {
                if(rule.type !== CSSRule.STYLE_RULE) continue;

                let selector = rule.selectorText.trim();
                if(!selector.startsWith(prefix)) continue;
                selector = selector.replaceAll(prefix, ':host');

                out.push(`${selector} { ${rule.style.cssText} }`);
            }
        }

        return out.join("\n");
    })('div.nj-block .nj-html');
});

class Grid {
    static is_config = (part, i) => (part != null && part.type == "notojs.Grid" && i == 0);
    static classname = (c) => c.replaceAll('/', '-').replaceAll('.', '-');

    constructor(out, target) {
        this.grid = [];
        this.prev = null;
        this.cells = 0;
        this.array = [];
        this.target = target;
        this.current = null;
        out.forEach((part, i) => {
            if("object" == typeof(part)) {
                if(Grid.is_config(part, i)) {
                    console.log('grid', part.data);
                    this.grid = part.data.split(' ').map((x, j) => {
                        if(j == 0 && x[0] == ':') {
                            const slide = document.createElement('nj-view');
                            this.target.replaceWith(slide);
                            slide.appendChild(this.target);
                            x = x.substr(1);
                        }
                        const arr = x.split('/');
                        return arr.length == 1 ?  (arr[0] ? [arr[0],1] : null) : [arr[0],parseInt(arr[1])];
                    }).filter(c => null !== c);
                }
                else if(part != null && part.type && part.type in Handlers) {
                    this.array.push(1);
                    ++this.cells;
                } else {
                    const v = (0 == this.array.length || 1 == lastOf(this.array));
                    if(v) { ++this.cells; this.array.push(2); }
                    else this.array.push(0);
                }
            } else {
                const v = (0 == this.array.length || 1 == lastOf(this.array));
                if(v) { ++this.cells; this.array.push(2); }
                else this.array.push(0);
            }
        });
        var width = 0;
        var cells = 0;
        this.grid.forEach(x => {
            width += parseInt(x[0]);
            if(width == 100) width = 0;
            cells += x[1];
        });
        const left = parseInt((100 - width) / (this.cells - cells || 1));
        while(this.cells > cells)
        {
            this.grid.push([left + '%', 1]);
            ++cells;
        }
        console.log('grid', this.grid);
    }
    get(clazz) {
        const type = this.array.shift();
        if(0 == type)
        {
            return make_block(lastOf(this.current.children), 'span', 'inline');
        }
        if(this.current == null || this.grid[0][1] == 0)
        {
            if(this.current != null) this.grid.shift();
            this.current = make_block(this.target, 'div', 'column');
            this.current.style.width = `calc(${this.grid[0][0]} - 2 * var(--nj-block-padding))`;
        }
        --this.grid[0][1];
        const b = make_block(this.current, 'div', clazz);
        return 2 == type ? make_block(b, 'span', 'inline') : b;
    }
}

export function render(el, code, config={}) {
    if(code instanceof Array) return Promise.all(code.map(c => render(el, c, config)));

    let result = new Array();
    let rendrs = new Array();
    code = JSON.parse(code || '[]');

    const ri = code.findIndex((part) => "notojs.Render" === part.type);
    if(ri >= 0) {
        config.base ??= window.connection?.server;
        rendrs = config.base ? Promise.all(code.splice(ri, 1)[0].data.map((r) => {
            if(!(r in Handlers)) {
                console.log(`Loading ${config.base}${r}`);
                const p = new Promise((accept) => {
                    const failed = () => {
                        console.log(`${config.base}${r} - Failed`);
                        Handlers[r] = (grid, part) => {
                            Handlers['.obj'](grid, part);
                        };
                        Handlers[r].$resolved = false;
                        accept(null);
                    };
                    import(`${config.base}${r}`).then(m => {
                        console.log(`${config.base}${r} - OK`);
                        m.register(Handlers);
                        if (undefined === typeof Handlers[r]) failed();
                        else {
                            Handlers[r].$resolved = true;
                            accept(r);
                        }
                    }, failed);
                });
                const s = result.length;
                Handlers[r] = (grid, part) => {
                    console.log('Enqueued', part.type);
                    const p = result[s];
                    const d = grid instanceof HTMLElement ? grid : grid.get(part.type);
                    if(d) result[s] = p.then(() => {
                        Handlers[r]({
                            get: c => {
                                console.log('Rendering', r);
                                c.split(' ').forEach((v, i) => {
                                    if(i == 0) d.classList.replace(`nj-${Grid.classname(part.type)}`, `nj-${Grid.classname(v)}`);
                                    else d.classList.add(`nj-${Grid.classname(v)}`);
                                });
                                return d;
                            }
                        }, part);
                        return p;
                    });
                };
                return (result.push(p), p);
            }
            return Handlers[r].$resolved && r;
        })).then(a => a.filter(x => x)) : [];
    }

    let oi = code.findIndex((part) => "notojs.Output" === part.type);
    if(oi >= 0) {
        code[oi].data.forEach((out) => {
            const grid = config.grid ? new class {
                get(c) {
                    const d = config.grid.container(c);
                    if(d) d.className = c.split(' ').map(c => `nj-${Grid.classname(c)}`).join(' ');
                    return d;
                }
            } : new Grid(out, make_block(el));
            out.forEach(function(part, i) {
                if("object" == typeof(part)) {
                    if(part == null) grid.get('text').innerHTML = 'null';
                    else if(part.type in Handlers) {
                        const p = Handlers[part.type](grid, part);
                        if(p instanceof Promise) result.push(p);
                    } else if(!Grid.is_config(part, i)) {
                        Handlers['.obj'](grid, part);
                    }
                } else {
                    const e = grid.get('text');
                    if(e) e.textContent = part;
                }
            });
        });
        code.splice(oi, 1);
    }

    for(const part of code) {
        if(part.type.indexOf("Error") >= 0 || part.type.indexOf("Exception") >= 0) {
            const block = make_block(make_block(el), 'pre', 'error');
            if("object" == typeof(part.data)) {
                block.textContent = part.type;
                if("string" == typeof(part.data?.message)) {
                    block.textContent += (': ' + part.data.message);
                } else if (undefined  === part.data?.stack) {
                    block.textContent += (': ' + JSON.stringify(part.data));
                }
                if(undefined !== part.data?.stack) {
                    block.textContent += ('\n' + part.data.stack);
                }
                if(undefined !== part.data?.detail) {
                    block.textContent += ('\n' + part.data.detail);
                }
            } else {
                block.textContent = JSON.stringify(part.data, null, 2);
            }
        } else {
            console.error(`Unsupported part ${part.type}`);
        }
    }
    return Promise.all([rendrs, ...result]);
}

window.MathJaxPending = new Array();
window.MathJax = {
    tex: {
        inlineMath: [['$begin:math:text$', '$end:math:text$']],
        displayMath: [['$$begin:math:text$', '$end:math:text$$']]
    },
    svg: { fontCache: 'global' },
    startup: {
        ready: () => {
            MathJax.startup.defaultReady();
            for(const c of window.MathJaxPending) c();
            delete window.MathJaxPending;
        }
    }
};

customElements.define('nj-view', NJView);
customElements.whenDefined('nj-cell').then(()=> {
    customElements.get('nj-cell').render = render;
});

import('./tex-svg.js');
