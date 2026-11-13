import render from 'noto:render'

const traits = new class {
    is_any(o, f) {
        if(!this.is_array(o)) return false;
        for(const e of o) if(f(e)) return true;
        return false;
    }
    is_array(o) {
        return o !== null && o !== undefined && 'function' === typeof o[Symbol.iterator];
    }
    is_each(o, f) {
        if(!this.is_array(o)) return false;
        for(const e of o) if(!f(e)) return false;
        return true;
    }
    is_object(o) {
        if(o === null || 'object' !== typeof o || this.is_array(o)) return false;
        const prototype = Object.getPrototypeOf(o);
        return prototype === Object.prototype || prototype === null;
    }
    is_series(o, level=3) {
        if((level & 1) && this.is_each(o, s => this.is_series(s, 2))) return true;
        if((level & 2) && this.is_object(o)) return true;
        return false;
    }
    is_table(o) {
        if(!this.is_object(o)) return false;
        const properties = Object.keys(o);
        return properties.length > 0 && properties.every(p => Number.isFinite(o[p]));
    }
    to_series(o, type) {
        if(this.is_array(o)) {
            const values = Array.from(o);
            return {
                series: values.map(s => ({type: type, ...s})).map(s => {
                    if('scatter' === s.type && undefined !== s.size)
                        s.symbolSize = s.size, delete s.size;
                    return s;
                }),
                legend: (values.length > 1 && this.is_each(values, o => !!o.name)) ? {} : null
            }
        }
        if(['donut', 'pie'].indexOf(type) >= 0) {
            const properties = Object.keys(o);
            return {
                series: [{
                    data: properties.map(p => ({value: o[p], name: p})),
                    center: ['50%', properties.length > 1 ? '45%' : '50%'],
                    radius: type === 'donut' ? [
                        properties.length > 1 ? '30%' : '45%',
                        properties.length > 1 ? '50%' : '75%'
                    ] : properties.length > 1 ? '50%' : '75%',
                    type: 'pie'
                }],
                legend: properties.length > 1 ? {left: 'center', bottom: '2%'} : null
            }
        }
        if(this.is_object(o)) return {
            series: [{type: type, ...o}].map(s => {
                if('scatter' === s.type && undefined !== s.size)
                    s.symbolSize = s.size, delete s.size;
                return s;
            }),
            legend: null
        }
    }
};

function grid(chart) {
    chart.grid = {left: '5%', right: '5%', top: '5%'};
    if(chart.series.length == 1 || traits.is_any(chart.series, s => undefined === s.name))
        chart.grid.bottom = '5%';
}

function merge(target, ...sources) {
    if(!traits.is_object(target))
        throw new TypeError('Merge target should be an object');

    for(const source of sources) {
        if(source === undefined) continue;
        if('function' === typeof source) {
            const result = source(target);
            if(result !== undefined) {
                if(!traits.is_object(result))
                    throw new TypeError('Chart option callback should return an object');
                target = result;
            }
            continue;
        }
        if(!traits.is_object(source))
            throw new TypeError('Chart options should be an object or function');

        for(const key of Object.keys(source)) {
            const value = source[key];
            if(traits.is_object(value)) {
                const current = traits.is_object(target[key]) ? target[key] : {};
                target[key] = merge(current, value);
            } else {
                target[key] = value;
            }
        }
    }
    return target;
}

function heatmap(data, ...opts) {
    if(!traits.is_object(data)) throw TypeError('Unsupported arguments');

    const x = traits.is_array(data.x) ? Array.from(data.x) : data.x;
    const y = traits.is_array(data.y) ? Array.from(data.y) : data.y;
    const values = traits.is_array(data.data) ? Array.from(data.data) : data.data;
    if(traits.is_each(x, value => Number.isFinite(value) || 'string' === typeof value) &&
       traits.is_each(y, value => Number.isFinite(value) || 'string' === typeof value) &&
       traits.is_array(values) && values.length) {
        const points = values.map((point, index) => {
            if(!traits.is_array(point))
                throw new TypeError(`Heatmap data point ${index} should be iterable`);
            const values = Array.from(point);
            if(values.length !== 3)
                throw new RangeError(`Heatmap data point ${index} should contain X, Y, and value`);
            if(!Number.isInteger(values[0]) || values[0] < 0 || values[0] >= x.length ||
               !Number.isInteger(values[1]) || values[1] < 0 || values[1] >= y.length)
                throw new RangeError(`Heatmap data point ${index} is outside the axes`);
            if(!Number.isFinite(values[2]))
                throw new TypeError(`Heatmap data point ${index} value should be a finite number`);
            return values;
        });
        let min = Infinity;
        let max = -Infinity;
        for(const point of points) {
            min = Math.min(min, point[2]);
            max = Math.max(max, point[2]);
        }
        return merge({
            legend: null,
            tooltip: {position: 'top'},
            xAxis: {type: 'category', data: x, splitArea: {show: true}},
            yAxis: {type: 'category', data: y, splitArea: {show: true}},
            visualMap: {min, max, calculable: true, show: false},
            series: [{
                name: data.name,
                type: 'heatmap',
                data: points,
                emphasis: {itemStyle: {shadowBlur: 10, shadowColor: 'rgba(0, 0, 0, 0.5)'}}
            }]
        }, ...opts);
    }
    throw TypeError('Unsupported arguments');
}

function scatter(data, ...opts) {
    const series = traits.is_array(data) ? Array.from(data) : data;
    if(traits.is_series(series)) return merge({
        ...traits.to_series(series, 'scatter'),
        tooltip: {trigger: 'axis'},
        xAxis: {},
        yAxis: {}
    }, ...opts)
    throw TypeError('Unsupported arguments');
}

function pie(type, data, ...opts) {
    if(traits.is_table(data)) return merge({
        ...traits.to_series(data, type),
        tooltip: {trigger: 'item'},
    }, ...opts)
    throw TypeError('Unsupported arguments');
}

function xy(type, data, ...opts) {
    if(!traits.is_object(data)) throw TypeError('Unsupported arguments');
    if(data.continuous !== undefined && 'boolean' !== typeof data.continuous)
        throw new TypeError('Continuous should be a boolean');

    const x = traits.is_array(data.x) ? Array.from(data.x) : data.x;
    const y = traits.is_array(data.y) ? Array.from(data.y) : data.y;
    if(traits.is_each(x, x => Number.isFinite(x) || 'string' === typeof x)) {
        if(traits.is_each(y, Number.isFinite)) {
            if(x.length !== y.length)
                throw new RangeError(`X and Y data should have the same length (${x.length} !== ${y.length})`);
            const numeric = traits.is_each(x, Number.isFinite);
            if(type !== 'bar' && data.continuous === true && !numeric)
                throw new TypeError('Continuous chart data should contain only finite numbers');
            const continuous = type !== 'bar' && data.continuous !== false && numeric;
            return merge({
                series: [{
                    data: continuous ? x.map((value, index) => [value, y[index]]) : y,
                    type: type
                }],
                tooltip: {trigger: 'axis'},
                xAxis: continuous ? {
                    type: 'value',
                    scale: true,
                    minorSplitLine: { show: true }
                } : {
                    type: 'category',
                    data: x,
                    splitLine: {show: type !== 'bar'},
                    boundaryGap: type === 'bar'
                },
                yAxis: continuous ? {type: 'value', scale: true, minorSplitLine: { show: true }} : {type: 'value'}
            }, ...opts);
        }
        if(traits.is_series(y)) {
            const converted = traits.to_series(y, type);
            for(const [index, series] of converted.series.entries()) {
                if(!traits.is_array(series.data))
                    throw new TypeError(`Series ${index} data should be iterable`);
                series.data = Array.from(series.data);
                if(x.length !== series.data.length)
                    throw new RangeError(`X and series ${index} data should have the same length (${x.length} !== ${series.data.length})`);
            }
            const numeric = traits.is_each(x, Number.isFinite)
                && converted.series.every(series => traits.is_each(series.data, Number.isFinite));
            if(type !== 'bar' && data.continuous === true && !numeric)
                throw new TypeError('Continuous chart data should contain only finite numbers');
            const continuous = type !== 'bar' && data.continuous !== false && numeric;
            if(continuous)
                for(const series of converted.series)
                    series.data = x.map((value, index) => [value, series.data[index]]);
            return merge({
                ...converted,
                tooltip: {trigger: 'axis'},
                xAxis: continuous ? {
                    type: 'value',
                    scale: true,
                    minorSplitLine: { show: true }
                } : {
                    type: 'category',
                    data: x,
                    splitLine: {show: type !== 'bar'},
                    boundaryGap: type === 'bar'
                },
                yAxis: continuous ? {type: 'value', scale: true, minorSplitLine: {show: true}} : {type: 'value'}
            }, ...opts);
        }
    }
    throw TypeError('Unsupported arguments');
}

const $data = Symbol.for('$data');
const $prop = Symbol.for('$prop');
const $series = Symbol.for('$series');

class Axis {
    constructor(data, prop) {
        this[$data] = data;
        this[$prop] = prop;
    }
    toJSON() {
        return this[$prop](this[$data]);
    }
    set bounds(value) {
        if(!Array.isArray(value) || value.length !== 2)
            throw TypeError('Axis bounds should be [min, max]');
        const axes = this[$prop](this[$data]);
        for(const axis of (Array.isArray(axes) ? axes : [axes])) {
            if(axis.type === 'category') {
                const length = Array.isArray(axis.data) ? axis.data.length : undefined;
                for(const bound of value) {
                    if('number' !== typeof bound) continue;
                    if(!Number.isInteger(bound))
                        throw new TypeError('Category axis numeric bounds should be integer indices');
                    if(bound < 0 || (length !== undefined && bound >= length))
                        throw new RangeError(`Category axis index ${bound} is out of range`);
                }
            }
            axis.min = value[0];
            axis.max = value[1];
        }
    }
    set format(value) {
        if('string' !== typeof value)
            throw new TypeError('Axis format should be a string');
        const axes = this[$prop](this[$data]);
        for(const axis of (Array.isArray(axes) ? axes : [axes])) {
            if(!traits.is_object(axis.axisLabel)) axis.axisLabel = {};
            axis.axisLabel.formatter = value;
            if(!traits.is_object(axis.axisPointer)) axis.axisPointer = {};
            if(!traits.is_object(axis.axisPointer.label)) axis.axisPointer.label = {};
            axis.axisPointer.label.formatter = value;
        }
    }
};

class SeriesAxis extends Axis {
    constructor(data, prop, series) {
        super(data, prop);
        this[$series] = series;
    }
    set format(value) {
        super.format = value;

        const tooltip = traits.is_object(this[$data].tooltip) ? this[$data].tooltip : {};
        const current = tooltip.valueFormatter;
        const fallback = 'string' === typeof current
            ? current
            : Array.isArray(current) && 'string' === typeof current[0]
                ? current[0]
                : '{value}';
        const formatters = Array.isArray(current) ? current.slice() : [];
        for(let i = 0; i < this[$data].series.length; ++i)
            if('string' !== typeof formatters[i]) formatters[i] = fallback;
        for(const s of this[$series]) formatters[s] = value;
        this[$data].tooltip = merge({}, tooltip, {valueFormatter: formatters});
    }
    set offset(value) {
        if(!Number.isFinite(value))
            throw new TypeError('Axis offset should be a finite number');
        this[$prop](this[$data]).offset = value;
    }
}

class YAxis extends Axis {
    add(series, position) {
        const selectors = Array.isArray(series) ? series : [series];
        if(!selectors.length)
            throw new TypeError('At least one series should be selected');

        const indices = new Set();
        for(const selector of selectors) {
            let index = -1;
            if(Number.isInteger(selector)) {
                if(selector >= 0 && selector < this[$data].series.length) index = selector;
            } else if('string' === typeof selector && selector.length) {
                const matches = [];
                for(let i = 0; i < this[$data].series.length; ++i)
                    if(this[$data].series[i].name === selector) matches.push(i);
                if(matches.length > 1)
                    throw new TypeError(`Series name ${selector} is ambiguous; use an index`);
                if(matches.length) index = matches[0];
            } else {
                throw new TypeError('Series should be selected by index or name');
            }
            if(index < 0) throw new RangeError(`Series ${selector} not found`);
            indices.add(index);
        }

        let axes = this[$prop](this[$data]);
        const vertical = axes === this[$data].yAxis;
        const property = vertical ? 'yAxisIndex' : 'xAxisIndex';
        const id = vertical ? 'yAxisId' : 'xAxisId';
        const attached = axisValues(axes);
        for(const axis of attached) {
            if(!traits.is_object(axis.axisLine)) axis.axisLine = {};
            axis.axisLine.onZero ??= false;
        }
        for(const index of indices) {
            const selected = this[$data].series[index];
            const assigned = selected[id] === undefined
                ? undefined
                : attached.findIndex(axis => axis?.id === selected[id]);
            if((selected[property] !== undefined && selected[property] !== 0) ||
                (selected[id] !== undefined && assigned !== 0))
                throw new TypeError(`Series ${index} already has custom axis`);
        }

        const positions = vertical ? ['left', 'right'] : ['top', 'bottom'];
        position ??= vertical ? 'right' : 'top';
        if(!positions.includes(position))
            throw new TypeError(`${vertical ? 'Y' : 'X'}-axis position should be ${positions.join(' or ')}`);

        if(!Array.isArray(axes)) axes = this[$prop](this[$data], [axes]);

        const defaultPosition = vertical ? 'left' : 'bottom';
        const offsets = axes
            .filter(axis => (axis.position ?? defaultPosition) === position)
            .map(axis => Number.isFinite(axis.offset) ? axis.offset : 0);
        const offset = offsets.length
            ? Math.max(...offsets) + (vertical ? 30 : 20)
            : 0;
        const aindex = axes.length;
        const axis = merge({}, axes[0], {position, offset});
        delete axis.id;
        axes.push(axis);

        for(const index of indices) {
            delete this[$data].series[index][id];
            this[$data].series[index][property] = aindex;
        }
        return new SeriesAxis(this[$data], d => axes[aindex], indices);
    }
    set format(value) {
        if(Array.isArray(value)) {
            if(!value.length)
                throw new TypeError('Y-axis format array should contain strings');
            for(let i = 0; i < value.length; ++i)
                if('string' !== typeof value[i])
                    throw new TypeError('Y-axis format array should contain strings');
        } else {
            super.format = value;
        }
        const tooltip = traits.is_object(this[$data].tooltip) ? this[$data].tooltip : {};
        this[$data].tooltip = merge({}, tooltip, {valueFormatter: value});
    }
}

const Series = Object.freeze({
    get(target, prop, receiver) {
        if(prop === 'type' && target.type === 'line' && traits.is_object(target.areaStyle))
            return 'area';
        return Reflect.get(target, prop, receiver)
    },
    set(target, prop, value) {
        if(prop === 'type') {
            if(value == 'area') {
                if(target.areaStyle === undefined) Reflect.set(target, 'areaStyle', {})
                return Reflect.set(target, prop, 'line')
            }
            delete target.areaStyle;
        }
        return Reflect.set(target, prop, value);
    }
})

function axisValues(value) {
    return Array.isArray(value) ? value : [value];
}

function axisType(value) {
    const axes = axisValues(value);
    if(!axes.length) return;
    if(axes.every(axis => axis?.type === 'category')) return 'category';
    if(axes.every(axis => axis?.type !== 'category')) return 'value';
    throw new TypeError('Mixed category and value axes are not supported');
}

function chartAxis(data, property) {
    if(!data[property]) return;
    let axes = data[property];
    const Type = axisType(axes) === 'category' ? Axis : YAxis;
    return new Type(data, (target, value) => {
        const current = target.xAxis === axes
            ? 'xAxis'
            : target.yAxis === axes
                ? 'yAxis'
                : undefined;
        if(current === undefined)
            throw new TypeError('Axis is no longer attached to the chart');
        if(value === undefined) return axes;
        target[current] = value;
        axes = value;
        return value;
    });
}

function axesAre(value, type) {
    return !!value && axisType(value) === type;
}

function translatePositions(value, positions) {
    for(const axis of axisValues(value))
        if(Object.hasOwn(positions, axis?.position)) axis.position = positions[axis.position];
}

function swapSeriesAxes(series) {
    for(const [x, y] of [['xAxisIndex', 'yAxisIndex'], ['xAxisId', 'yAxisId']]) {
        const xv = series[x];
        const yv = series[y];
        if(yv === undefined) delete series[x];
        else series[x] = yv;
        if(xv === undefined) delete series[y];
        else series[y] = xv;
    }
}

const Chart = Object.freeze({
    get series() {
        return this.data.series?.map(s => new Proxy(s, Series));
    },
    get horizontal() {
        return axesAre(this.data.yAxis, 'category') &&
            axesAre(this.data.xAxis, 'value');
    },
    get stacked() {
        const series = (this.data.series ?? []).filter(value =>
            value.type === 'bar' || value.type === 'line' && value.areaStyle
        );
        return series.length > 0 && series.every(value => !!value.stack);
    },
    get xaxis() {
        return chartAxis(this.data, 'xAxis');
    },
    get yaxis() {
        return chartAxis(this.data, 'yAxis');
    },
    set format(value) {
        const heatmap = this.data.series?.length && this.data.series.every(series => 'heatmap' === series.type);
        if((this.xaxis || this.yaxis) && !heatmap)
            throw new TypeError('Use xaxis.format or yaxis.format');
        if('string' !== typeof value)
            throw new TypeError('Chart format should be a string');
        const tooltip = traits.is_object(this.data.tooltip) ? this.data.tooltip : {};
        this.data.tooltip = merge({}, tooltip, {valueFormatter: value});
    },
    set horizontal(flag) {
        if('boolean' !== typeof flag)
            throw new TypeError('Chart horizontal should be a boolean');

        const vertical = axesAre(this.data.xAxis, 'category') &&
            axesAre(this.data.yAxis, 'value');
        const horizontal = axesAre(this.data.yAxis, 'category') &&
            axesAre(this.data.xAxis, 'value');
        if(!vertical && !horizontal)
            throw new TypeError('Chart cannot be horizontal');
        if(flag === horizontal) return;

        const xAxis = this.data.xAxis;
        this.data.xAxis = this.data.yAxis;
        this.data.yAxis = xAxis;
        translatePositions(this.data.xAxis, {left: 'bottom', right: 'top'});
        translatePositions(this.data.yAxis, {top: 'right', bottom: 'left'});
        for(const series of this.data.series) swapSeriesAxes(series);
    },
    set stacked(flag) {
        if('boolean' !== typeof flag)
            throw new TypeError('Chart stacked should be a boolean');
        let modified = 0;
        for(const s of this.data.series) {
            if('bar' == s.type) {
                if(flag) s.stack = 'x';
                else delete s.stack;
                ++modified;
            } else if('line' == s.type && s.areaStyle) {
                if(flag) s.stack = 'Total';
                else delete s.stack;
                ++modified;
            }
        }
        if(!modified) throw new TypeError('Chart cannot be stacked');
    }
})

const view = /^\d+\/\d+$/;
export const chart = Object.freeze(Object.assign(render(Chart, $THIS, view, opts => opts), {
    area: render(Chart, $THIS, view, (data, opts) => xy('line', data, grid, chart => {
        for(const s of chart.series) s.areaStyle = {};
    }, opts)),
    bar: render(Chart, $THIS, view, (data, opts) => xy('bar', data, grid, opts)),
    donut: render(Chart, $THIS, view, (data, opts) => pie('donut', data, opts)),
    heatmap: render(Chart, $THIS, view, (data, opts) => heatmap(data, grid, opts)),
    line: render(Chart, $THIS, view, (data, opts) => xy('line', data, grid, chart => {
        if(axesAre(chart.xAxis, 'value'))
            for(const s of chart.series)
                if(s.type === 'line') s.showSymbol ??= false;
    }, opts)),
    pie: render(Chart, $THIS, view, (data, opts) => pie('pie', data, opts)),
    scatter: render(Chart, $THIS, view, (data, opts) => scatter(data, grid, opts))
}))

export default chart;
