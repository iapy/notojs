import * as echarts from './echarts.min.js';
import jsep from './jsep.js';

export const compile = new class {
    #expressions = new Map;
    #number = Object.freeze({
        toExponential: Number.prototype.toExponential,
        toFixed: Number.prototype.toFixed,
        toPrecision: Number.prototype.toPrecision,
        toString: Number.prototype.toString
    })

    #format = new class {
        #weekdays = Object.freeze(['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'])
        #months = Object.freeze(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
            'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'])
        #number = /^%([-+ 0]*)(\d+)?(?:\.(\d+))?([fFeEgGdi])$/
        #date = /\[([^\]]*)\]|YYYY|MMM|MM|M|ddd|DD|D|HH|H|hh|h|mm|m|ss|s|SSS|A/g

        #formatNumber(value, format) {
            const match = format.match(this.#number);
            if(!match) throw new SyntaxError(`Unsupported number format ${format}`);

            const flags = new Set(match[1]);
            const width = match[2] === undefined ? 0 : Number(match[2]);
            const precision = match[3] === undefined ? undefined : Number(match[3]);
            const type = match[4];
            if(width > 1000)
                throw new RangeError('Number format width should not exceed 1000');
            if(precision !== undefined && precision > 100)
                throw new RangeError('Number format precision should not exceed 100');

            const number = Number(value);
            const negative = number < 0 || Object.is(number, -0);
            const magnitude = Math.abs(number);
            let sign = negative ? '-' : flags.has('+') ? '+' : flags.has(' ') ? ' ' : '';
            let result;

            if(Number.isNaN(number)) {
                sign = '';
                result = 'NaN';
            } else if(!Number.isFinite(number)) {
                result = 'Infinity';
            } else if(type === 'd' || type === 'i') {
                result = String(Math.trunc(magnitude));
                if(precision !== undefined) result = result.padStart(precision, '0');
            } else if(type === 'f' || type === 'F') {
                result = magnitude.toFixed(precision ?? 6);
            } else if(type === 'e' || type === 'E') {
                result = magnitude.toExponential(precision ?? 6);
            } else {
                const significant = precision ?? 6;
                if(significant < 1)
                    throw new RangeError('Number format precision should be at least 1 for %g');
                const [mantissa, exponent] = magnitude.toPrecision(significant).split('e');
                result = mantissa.replace(/(\.\d*?[1-9])0+$|\.0+$/, '$1');
                if(exponent !== undefined) result += `e${exponent}`;
            }

            if(type === type.toUpperCase()) result = result.toUpperCase();

            const padding = Math.max(0, width - sign.length - result.length);
            if(flags.has('-')) return sign + result + ' '.repeat(padding);
            if(flags.has('0')) return sign + '0'.repeat(padding) + result;
            return ' '.repeat(padding) + sign + result;
        }

        #formatDate(value, format) {
            const date = value instanceof Date ? value : new Date(value);
            if(Number.isNaN(date.getTime())) return String(value);

            const pad = (value, width = 2) => String(value).padStart(width, '0');
            const hour = date.getHours();
            const directives = {
                YYYY: () => pad(date.getFullYear(), 4),
                MMM: () => this.#months[date.getMonth()],
                MM: () => pad(date.getMonth() + 1),
                M: () => String(date.getMonth() + 1),
                ddd: () => this.#weekdays[date.getDay()],
                DD: () => pad(date.getDate()),
                D: () => String(date.getDate()),
                HH: () => pad(hour),
                H: () => String(hour),
                hh: () => pad(hour % 12 || 12),
                h: () => String(hour % 12 || 12),
                mm: () => pad(date.getMinutes()),
                m: () => String(date.getMinutes()),
                ss: () => pad(date.getSeconds()),
                s: () => String(date.getSeconds()),
                SSS: () => pad(date.getMilliseconds(), 3),
                A: () => hour < 12 ? 'AM' : 'PM'
            };
            return format.replace(this.#date, (token, literal) =>
                literal === undefined ? directives[token]() : literal
            );
        }

        #impl(value, format) {
            if('string' !== typeof format) throw new TypeError('Format should be a string');
            value = value !== null && value !== undefined && 'function' === typeof value.valueOf
                ? value.valueOf()
                : value;
            return format.startsWith('%')
                ? this.#formatNumber(value, format)
                : this.#formatDate(value, format);
        }
        constructor() {
            const fn = (value, format) => this.#impl(value, format);
            Object.setPrototypeOf(fn, new.target.prototype);
            return fn;
        }
    }

    #expr(expr) {
        if('BinaryExpression' === expr?.type) {
            const l = this.#expr(expr?.left);
            const r = this.#expr(expr?.right);
            if('function' === typeof l && 'function' === typeof r) {
                if('+' === expr?.operator) return (value, axis) => l(value, axis) + r(value, axis);
                if('-' === expr?.operator) return (value, axis) => l(value, axis) - r(value, axis);
                if('*' === expr?.operator) return (value, axis) => l(value, axis) * r(value, axis);
                if('/' === expr?.operator) return (value, axis) => l(value, axis) / r(value, axis);
                if('%' === expr?.operator) return (value, axis) => l(value, axis) % r(value, axis);
                if('<' === expr?.operator) return (value, axis) => l(value, axis) < r(value, axis);
                if('>' === expr?.operator) return (value, axis) => l(value, axis) > r(value, axis);
                if('&&' === expr?.operator) return (value, axis) => l(value, axis) && r(value, axis);
                if('||' === expr?.operator) return (value, axis) => l(value, axis) || r(value, axis);
                if('**' === expr?.operator) return (value, axis) => l(value, axis) ** r(value, axis);
                if('<=' === expr?.operator) return (value, axis) => l(value, axis) <= r(value, axis);
                if('>=' === expr?.operator) return (value, axis) => l(value, axis) >= r(value, axis);
                if('==' === expr?.operator) return (value, axis) => l(value, axis) === r(value, axis);
                if('!=' === expr?.operator) return (value, axis) => l(value, axis) !== r(value, axis);
            }
        }
        if('CallExpression' === expr?.type) {
            const func = this.#name(expr.callee?.property);
            const self = this.#expr(expr.callee?.object);
            const args = expr.arguments.map(a => this.#expr(a));
            if(!args.every(a => 'function' === typeof a)) return;
            if(Math === self) {
                const mf = Object.hasOwn(Math, func) ? Math[func] : undefined;
                if('function' === typeof mf)
                    return (value, axis) => mf(...args.map(a => a(value, axis)))
            } else if('function' === typeof self) {
                if(func === 'format') {
                    if(args.length !== 1) return;
                    return (value, axis) => this.#format(self(value, axis), args[0](value, axis))
                } else {
                    const nf = Object.hasOwn(this.#number, func)
                        ? this.#number[func]
                        : undefined;
                    if('function' === typeof nf)
                        return (value, axis) => nf.call(self(value, axis), ...args.map(a => a(value, axis)))
                }
            }
        }
        if('ConditionalExpression' === expr?.type) {
            const test = this.#expr(expr.test);
            const consequent = this.#expr(expr.consequent);
            const alternate = this.#expr(expr.alternate);
            if('function' === typeof test && 'function' === typeof consequent && 'function' === typeof alternate)
                return (value, axis) => test(value, axis) ? consequent(value, axis) : alternate(value, axis);
        }
        if('MemberExpression' === expr?.type && 'Identifier' === expr.object?.type) {
            if('Math' === expr.object?.name && !expr.computed) {
                const name = this.#name(expr.property);
                const constant = Object.hasOwn(Math, name) ? Math[name] : undefined;
                if('number' === typeof constant) return () => constant;
            }
            else if('value' === expr.object?.name && expr.computed) {
                const prop = expr.property?.value;
                if('number' === typeof prop) return (value) => value[prop];
            }
        }
        if('Identifier' === expr?.type) {
            if('Math' === expr?.name) return Math;
            if('axis' === expr?.name) return (_, axis) => axis;
            if('value' === expr?.name) return (value) => value;
        }
        if('Literal' === expr?.type) return () => expr.value;
        if('UnaryExpression' === expr?.type && expr.prefix) {
            const argument = this.#expr(expr.argument);
            if('function' !== typeof argument) return;
            if('-' === expr?.operator) return (value, axis) => -argument(value, axis);
            if('+' === expr?.operator) return (value, axis) => +argument(value, axis);
            if('!' === expr?.operator) return (value, axis) => !argument(value, axis);
            if('~' === expr?.operator) return (value, axis) => ~argument(value, axis);
        }
    }
    #name(o) {
        if('Identifier' === o?.type) return o?.name;
    }
    #code(tmpl) {
        const code = tmpl.trim();
        if(this.#expressions.has(code)) return this.#expressions.get(code);

        const fn = this.#expr(jsep(code));
        if(!fn) throw new SyntaxError(`Unsupported expression ${tmpl}`)
        this.#expressions.set(code, fn);
        return fn;
    }
    #impl(tmpl) {
        let offset = 0;
        const parts = [];

        const textarea = document.createElement('textarea');
        for(const match of tmpl.matchAll(/\{([^{}]+)\}/g)) {
            const text = tmpl.slice(offset, match.index);
            if(text)
            {
                textarea.innerHTML = text;
                const decoded = textarea.value;
                parts.push(() => decoded);
            }

            parts.push(this.#code(match[1]));
            offset = match.index + match[0].length;
        }

        const text = tmpl.slice(offset);
        if(text)
        {
            textarea.innerHTML = text;
            const decoded = textarea.value;
            parts.push(() => decoded);
        }

        textarea.remove();
        return (value, axis) => parts.map(part => part(value, axis)).join('');
    }
    constructor() {
        const fn = (a, b) => (true === b ? this.#code(a) : this.#impl(a));
        Object.setPrototypeOf(fn, new.target.prototype);
        return fn;
    }
};

function escapeHTML(value) {
    const escapes = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#39;'
    };
    return String(value).replace(/[&<>"']/g, character => escapes[character]);
}

function formatterValue(value, dimension) {
    if(Array.isArray(value)) {
        const index = Number.isInteger(dimension) ? dimension : dimension === 'x' ? 0 : 1;
        value = value[index] ?? value[0];
    }
    if('string' === typeof value && value.trim() && Number.isFinite(Number(value)))
        return Number(value);
    return value;
}

class NJChart extends HTMLElement {
    static #observer = new ResizeObserver((entries) => {
        for(const e of entries) {
            const c = echarts.getInstanceByDom(e.target);
            if(c) requestAnimationFrame(() => c.resize());
        }
    })
    #config
    #hideTooltip = event => {
        if(this.contains(event.target)) return;
        const chart = echarts.getInstanceByDom(this);
        if(chart) {
            chart.dispatchAction({type: 'hideTip'});
            chart.dispatchAction({type: 'downplay'});
        }
    }
    connectedCallback() {
        if(this.#config.color === undefined && window.palette.length)
            this.#config.color = window.palette;
        echarts.init(this, null, {renderer:'svg'}).setOption(this.#config);
        NJChart.#observer.observe(this);
        document.addEventListener('pointerdown', this.#hideTooltip, true);
    }
    disconnectedCallback() {
        document.removeEventListener('pointerdown', this.#hideTooltip, true);
        NJChart.#observer.unobserve(this);
        echarts.dispose(this);
    }
    set config(v) {
        this.#config = ((cfg) => {
            const series = Array.isArray(cfg.series) ? cfg.series : cfg.series ? [cfg.series] : [];
            const valueDimension = series.length && series.every(value => value?.type === 'heatmap') ? 2 : 'y';
            const axisFormatters = {x: [], y: []};
            let customAxisTooltip = false;
            for(const [dimension, axes] of [['x', cfg.xAxis], ['y', cfg.yAxis]]) {
                const values = Array.isArray(axes) ? axes : [axes];
                values.forEach((axis, index) => {
                    if('string' === typeof axis?.axisLabel?.formatter) {
                        const formatter = compile(axis.axisLabel.formatter);
                        axisFormatters[dimension][index] = value =>
                            formatter(formatterValue(value, dimension), false);
                        axis.axisLabel.formatter = value =>
                            formatter(formatterValue(value, dimension), true);
                        customAxisTooltip = true;
                    }
                    if('string' === typeof axis?.axisPointer?.label?.formatter) {
                        const formatter = compile(axis.axisPointer.label.formatter);
                        axisFormatters[dimension][index] ??= value =>
                            formatter(formatterValue(value, dimension), false);
                        axis.axisPointer.label.formatter = params =>
                            formatter(formatterValue(params.value, dimension), true);
                        customAxisTooltip = true;
                    }
                });
            }

            const configuredFormatter = cfg.tooltip?.valueFormatter;
            let valueFormatter;
            let valueFormatters;
            if('string' === typeof configuredFormatter) {
                const formatter = compile(configuredFormatter);
                valueFormatter = (value, dimension = valueDimension) =>
                    formatter(formatterValue(value, dimension), false);
            } else if(Array.isArray(configuredFormatter)) {
                if(!configuredFormatter.length)
                    throw new TypeError('Tooltip formatter array cannot be empty');
                valueFormatters = configuredFormatter.map(value => {
                    if('string' === typeof value) {
                        const formatter = compile(value);
                        return (data, dimension = valueDimension) =>
                            formatter(formatterValue(data, dimension), false);
                    }
                    throw new TypeError('Tooltip formatters must be strings');
                });
            }

            const customTooltip = cfg.tooltip?.trigger === 'axis' &&
                cfg.tooltip.formatter === undefined &&
                (customAxisTooltip || valueFormatters);
            if(customTooltip) {
                delete cfg.tooltip.valueFormatter;
                cfg.tooltip.formatter = input => {
                    const values = Array.isArray(input) ? input : [input];
                    if(!values.length || !values[0]) return '';

                    const first = values[0];
                    const dimension = first.axisDim;
                    const valueDimension = dimension === 'y' ? 'x' : 'y';
                    const index = Number.isInteger(first.axisIndex) ? first.axisIndex : 0;
                    const axisFormatter = axisFormatters[dimension]?.[index];
                    const rawAxisValue = first.axisValue ?? first.axisValueLabel;
                    const axisValue = escapeHTML(axisFormatter
                        ? axisFormatter(rawAxisValue)
                        : first.axisValueLabel ?? rawAxisValue ?? '');
                    let tooltip = `<div style="font-size:14px;color:#6d6e73;font-weight:400;line-height:1;">${axisValue}</div>`;
                    tooltip = tooltip.concat('<div style="margin: 10px 0 0;line-height:1;"><div style="margin: 0px 0 0;line-height:1;">')
                    values.forEach((v, i) => {
                        const series = Number.isInteger(v.seriesIndex) ? v.seriesIndex : i;
                        const formatter = valueFormatters
                            ? valueFormatters[series] ?? valueFormatters[0]
                            : valueFormatter;
                        const formatted = formatter
                            ? formatter(v.value, valueDimension)
                            : formatterValue(v.value, valueDimension);
                        tooltip = tooltip.concat(
                            `<div style="margin: ${i ? 10 : 0}px 0 0;line-height:1;">`,
                            v.marker,
                            '<span style="font-size:14px;color:#6d6e73;font-weight:400;margin-left:2px">',
                            escapeHTML(v.seriesName),
                            '</span>',
                            '<span style="float:right;margin-left:20px;font-size:14px;color:#6d6e73;font-weight:900">',
                            escapeHTML(formatted),
                            '<div style="clear:both"></div></div>'
                        );
                    });
                    return tooltip.concat('</div></div>');
                };
            } else if(valueFormatter) {
                cfg.tooltip.valueFormatter = value => valueFormatter(value);
            } else if(valueFormatters) {
                delete cfg.tooltip.valueFormatter;
                if(cfg.tooltip?.formatter === undefined)
                    throw new TypeError('Tooltip formatter arrays require an axis-trigger tooltip');
            }

            for(const s of series) {
                if('string' === typeof s.symbolSize) s.symbolSize = compile(s.symbolSize, true);
            }

            return cfg
        })(v);
    }
};

export function register(handlers) {
    if(!customElements.get('nj-chart'))
        customElements.define('nj-chart', NJChart);
    handlers[$THIS] = function(grid, part) {
        const c = document.createElement('nj-chart');
        c.style.display = 'block';
        c.style.width = '100%';
        c.style.aspectRatio = part.view ?? '16/9';
        c.config = {...part.data, animation: false, title: null};
        grid.get('stacked').appendChild(c);
    }
}
