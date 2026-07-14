const $ = Object.assign((d) => {
    d = d.split('.');
    const e = document.createElement(d[0]);
    d.slice(1).forEach((c) => e.classList.add(c));
    return e;
}, {
    Element: class extends HTMLElement {
        #local = [];
        #global = []
        constructor(props) {
            super();
            if(props)
            {
                Object.defineProperties(this, Object.fromEntries(
                    Object.entries(props).filter(([k]) => k[0] != '$' && k[0] != '.').map(([k,v]) => {
                        if(v instanceof Function) return [k, {
                            get() { return v(); },
                            configurable: false,
                            enumerable: true
                        }];
                        if(v instanceof Array) return [k, {
                            get() { return v[0](); },
                            set(w) { return v[1](w); },
                            configurable: false,
                            enumerable: true
                        }];
                    })
                ));
                this.#local = Object.entries(props).filter(([k]) => k[0] == '.').map(([k,v]) => {
                    return [k, (e) => v(e.detail, e.target)];
                });
                this.#global = Object.entries(props).filter(([k]) => k[0] == '$').map(([k,v]) => {
                    return [k, (e) => v(e.detail, e.target)];
                });
            }
        }
        connectedCallback() {
            if(this.#local) {
                for(const [k,v] of this.#local) {
                    this.addEventListener(`notoui.${k.substr(1)}`, v);
                }
                this.#local = null;
            }
            for(const [k,v] of this.#global) {
                document.body.addEventListener(`notoui.${k.substr(1)}`, v);
            }
            this.$mount?.();
        }
        disconnectedCallback() {
            this.$unmount?.();
            for(const [k,v] of this.#global) {
                document.body.removeEventListener(`notoui.${k.substr(1)}`, v);
            }
        }
        $dispatch(type, data) {
            console.log(`> [notoui.${type.$name}]`);
            this.dispatchEvent(new CustomEvent(`notoui.${type.$name}`, {
                detail: new type(data), bubbles: true
            }));
        }
        $button(text, x) {
            for(const e of (x || this).querySelectorAll('span.material-symbols-outlined'))
                if(e.textContent == text) return e;
        }
    },
    Event(name) {
        return class {
            static $name = name;
            constructor(o) {
                Object.assign(this, o);
            }
        };
    }
});
