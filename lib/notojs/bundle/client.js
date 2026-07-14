window.NotoJS = new class {
    #mod = null;
    #url = null;

    constructor() {
        const url = new URL(document.currentScript.src);
        url.pathname = url.pathname.substring(0, 1 + url.pathname.lastIndexOf('/'));

        this.#url = url.toString();
        this.#mod = Promise.all([
            new Promise(resolve => {
                const link = document.createElement('link');
                link.rel = 'stylesheet';
                link.href = `${this.#url}notojs.css`;
                link.onload = _ => {
                    console.log(`${this.#url}notojs.css - OK`);
                    import(`${this.#url}notojs.js`).then(m => resolve(m));
                };
                link.onerror = _ => {
                    console.error(`${this.#url}notojs.css - Failed`);
                    import(`${this.#url}notojs.js`).then(m => resolve(m));
                };
                document.head.appendChild(link);
            }),
            new Promise(resolve => {
                if (document.readyState === 'loading') {
                    document.addEventListener('DOMContentLoaded', _ => {
                        this.#style();
                        resolve();
                    }, { once: true });
                } else {
                    this.#style();
                    resolve();
                }
            })
        ]).then(([m]) => {
            document.querySelectorAll('div[data-notojs]').forEach(e => {
                const post = !e.children.length && e.firstChild && e.firstChild?.nodeType === Node.TEXT_NODE && e.textContent.trim();
                this.render(e, e.dataset.notojs, {
                    text: post ? e.textContent : undefined
                }).then(e => {
                    if(post) e.firstChild.remove();
                    if('notojsOnready' in e.dataset)
                        new Function(e.dataset.notojsOnready).call(e);
                });
            });
            console.log(`${this.#url}notojs.js - OK`);
            return m;
        });
    }

    #style() {
        if(document.querySelector('div[data-notojs-style]')) {
            const style = document.createElement('style');
            style.textContent = `div[data-notojs-style]{margin:auto;max-width:var(--nj-max-editor-width);}`;
            document.head.appendChild(style);
        }
    }

    render(el, book, config = {}) {
        return Promise.all([this.#mod, fetch(`${this.#url}r/${book}.notojs`, {
            body: (config.text ?? (config.json ? JSON.stringify(config.json) : undefined)),
            method: (config.text || config.json) ? 'POST' : 'GET',
            headers: (undefined !== config.json) ? [['Content-type', 'application/json']] : undefined
        }).then(r => r.json())]).then(([m, d]) => new Promise((accept, reject) => {
            m.render(el, d, {base: this.#url, grid: config.grid})
                .then(_ => accept(el), reject);
        }));
    }
};
