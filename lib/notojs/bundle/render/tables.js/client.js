export function register(handlers) {
    handlers[$RENDERER] = function(grid, part) {
        const d = grid.get('html stacked');
        const t = document.createElement('table');
        let p = null;
        let q = null;
        let u = null;
        let v = null;
        const w = part.view
            ? (i, n) => {
                p ??= part.view.split(' ').map(x => x[0] == '*' ? null : parseInt(x));
                u ??= 100 - p.reduce((acc, x) => x ? acc + x : acc, 0);
                v ??= p.filter(x => x).length;
                if(i < p.length && p[i]) return p[i];
                return `${u / (n - v)}`;
            }
            : (_, n) => `${100 / n}%`
        ;
        const a = part.view
            ? (i, n) => {
                q ??= part.view.split(' ').map(x => x.at(-1));
                if(i >= q.length) return null;
                if(q[i] == '<') return 'left';
                if(q[i] == '>') return 'right';
                if(q[i] == '|') return 'center';
                return null;
            }
            : (_, n) => null
        ;
        if(part.view) t.style.tableLayout = 'fixed';
        for(const row of part.data) {
            const r = document.createElement('tr');
            for(let i = 0; i < row.length; ++i) {
                const c = document.createElement(row[i].t);
                c.width = `${w(i, row.length)}%`;
                c.style.textAlign = a(i, row.length) ?? null;
                if(row[i].d instanceof Object) {
                    handlers.render(c, row[i].d);
                } else {
                    c.innerText = row[i].d;
                }
                r.appendChild(c);
            }
            t.appendChild(r);
        }
        d.appendChild(t);
    }
}
