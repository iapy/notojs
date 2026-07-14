function proxy(fn) {
    return new Proxy(fn, {
        get(t, p) {
            if(p.match(/^\d+\/\d+$/)) return (...args) => ({view: p, ...t(...args)});
            throw new TypeError(`Invalid aspect ratio ${p}`);
        }
    });
}

function is_asofas(a) {
    return a instanceof Array && a.reduce((acc, x) => acc & (x instanceof Array), true);
}

export const chart = proxy((t, o) => new Object({type: $.__renderer($RENDERER), data: ((type, opts) => {
    if(['area', 'bar', 'line'].indexOf(type) >= 0) return (array => ({
        grid: (() => array && opts.y.length > 1 && opts.labels
            ? {
                left: '5%',
                right: '5%',
                top: '5%'
            }
            : {
                left: '5%',
                right: '5%',
                top: '5%',
                bottom: '10%'
            }
        )(),
        series: (() => {
            if(array) return opts.y.map((y, i) => (Object.assign({
                data: y,
                type: type == 'area' ? 'line' : type,
                name: opts.labels?.[i]
            }, type == 'area'
                ? Object.assign({
                        areaStyle:{}
                    },
                    opts.stack
                        ? {stack: opts.stack}
                        : {}
                )
                : (opts.stack ? {stack: opts.stack}:{}))))
            else return [Object.assign({
                data: opts.y,
                type: type == 'area' ? 'line' : type,
                name: opts.labels?.[0]
            }, type == 'area'
                ? Object.assign({
                        areaStyle:{}
                    },
                    opts.stack
                        ? {stack: opts.stack}
                        : {}
                )
                : (opts.stack ? {stack: opts.stack}:{}))
            ]
        })(),
        legend: array && opts.y.length > 1 && opts.labels ? {} : null,
        tooltip: {
            trigger: 'axis',
            valueFormatter: opts.format?.y
        },
        xAxis: {
            boundaryGap: type == 'bar',
            data: opts.x,
            splitLine: {
                show: type != 'bar'
            },
            type: 'category'
        },
        yAxis: (() =>
            (opts.format?.y instanceof Array)
            ? opts.format?.y.map(f => ({
                axisLabel: {
                    formatter: f
                },
                type: 'value'
            })) : {
                axisLabel: {
                    formatter: opts.format?.y
                },
                type: 'value'
            }
        )()
    }))(is_asofas(opts.y));
    throw new TypeError(`Unsupported chart type ${type}`);
})(t, o)}));

export const echart = proxy(x => new Object({type: $.__renderer($RENDERER), data: (x => {
    if(x.geo?.map && !(x.geo.map instanceof Array && x.geo.map.length >= 2 && 'string' == typeof(x.geo.map[0])))
        throw TypeError('geo.map should be an array [name, ...data]');
    return x;
})(x)}));
