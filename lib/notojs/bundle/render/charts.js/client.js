import * as echarts from './echarts.min.js';

class NJChart extends HTMLElement {
    static #observer = new ResizeObserver((entries) => {
        for(const e of entries) {
            const c = echarts.getInstanceByDom(e.target);
            if(c) requestAnimationFrame(() => c.resize());
        }
    })
    #config
    connectedCallback() {
        echarts.init(this, null, {renderer:'svg'}).setOption(this.#config);
        NJChart.#observer.observe(this);
    }
    disconnectedCallback() {
        NJChart.#observer.unobserve(this);
        echarts.dispose(this);
    }
    set config(v) {
        this.#config = ((cfg) => {
            if(cfg.tooltip?.valueFormatter) {
                const formatter = cfg.tooltip.valueFormatter;
                if ('string' === typeof formatter)
                    cfg.tooltip.valueFormatter = (value) => formatter.replace(/\{value\}/, value);
                else if (formatter instanceof Array){
                    delete cfg.tooltip.valueFormatter;
                    cfg.tooltip.formatter = values => {
                        let tooltip = `<div style="font-size:14px;color:#6d6e73;font-weight:400;line-height:1;">${values[0].axisValue}</div>`;
                        tooltip = tooltip.concat('<div style="margin: 10px 0 0;line-height:1;"><div style="margin: 0px 0 0;line-height:1;">')
                        values.forEach((v, i) => {
                            tooltip = tooltip.concat(
                                `<div style="margin: ${i ? 10 : 0}px 0 0;line-height:1;">`,
                                v.marker,
                                '<span style="font-size:14px;color:#6d6e73;font-weight:400;margin-left:2px">',
                                v.seriesName,
                                '</span>',
                                '<span style="float:right;margin-left:20px;font-size:14px;color:#6d6e73;font-weight:900">',
                                (formatter[i] ?? formatter[0]).replace(/\{value\}/, v.value),
                                '<div style="clear:both"></div></div>'
                            );
                        });
                        return tooltip.concat('</div>');
                    };
                }
            }
            if(cfg.geo?.map) {
                echarts.registerMap(...cfg.geo.map);
                cfg.geo.map = cfg.geo.map[0];
            }
            for(const s of (cfg.series ?? [])) {
                if('map' !== s.type || !s.map) continue;
                echarts.registerMap(...s.map);
                s.map = s.map[0];
            }
            return cfg
        })(v);
    }
};

export function register(handlers) {
    if(!customElements.get('nj-chart'))
        customElements.define('nj-chart', NJChart);
    handlers[$RENDERER] = function(grid, part) {
        const d = grid.get('stacked');
        const c = document.createElement('nj-chart');
        c.style.display = 'block';
        c.style.width = '100%';
        c.style.aspectRatio = part.view ?? '4/3';
        c.config = {...part.data, animation: false, title: null};
        d.appendChild(c);
    }
}
