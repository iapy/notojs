import render from 'noto:render'

export const table = render($THIS, /^(\d+%|\*)([<>|])?(?: (\d+%|\*)([<>|])?)*$/, (x, c) => {
    if(x instanceof Array) {
        if(!x.length) return [];
        const r = undefined === c
            ? [Object.keys(x[0]).map(k => ({d: k, t: 'th'}))]
            : [c.map(c => ({d: c instanceof Array ? c[1] : c, t: 'th'}))]
        ;
        const k = undefined === c
            ? r[0].map(r => r.d)
            : c.map(c => c instanceof Array ? c[0] : c)
        ;
        for(const e of x) r.push(k.map(k => ({
            d: e[k] === undefined ? '' : e[k],
            t: 'td'
        })));
        return r;
    }
    if(x instanceof Object) {
        const k = undefined === c
            ? Object.keys(x).map(x => [x,x])
            : c.map(x => x instanceof Array ? x : [x,x]);
        return k.map(([k, n]) => {
            return [{d: n, t: 'th'}, {d: x[k], t: 'td'}];
        });
    }
    if('undefined' === typeof(x)) throw new TypeError('Unsupported value type: undefined');
    if(null === x) throw new TypeError('Unsupported value type: null');
    throw TypeError(`Unsupported value type: ${typeof(x)}`);
});

export default table;
