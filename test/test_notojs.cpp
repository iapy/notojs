#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include <notojs/notojs.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(NotoJS, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(HTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, html } from 'noto:core';

const a = html('<i>test</i>');
assert(() => a instanceof HTML);

assert(() => throws(() => new HTML(), 'HTML: no constructor'));
assert(() => throws(() => new HTML(1), 'HTML: no constructor'));
assert(() => throws(() => new HTML(''), 'HTML: no constructor'));
assert(() => throws(() => new HTML({data: 1}), 'HTML: no constructor'));

const b = html('<b>bold</b>');
assert(() => b instanceof HTML);
assert(() => html(b.data) instanceof HTML);

print(a, b);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.HTML"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "<i>test</i>"));

    BOOST_TEST(!strcmp(out[0][1]["type"].GetString(), "notojs.HTML"));
    BOOST_TEST(!strcmp(out[0][1]["data"].GetString(), "<b>bold</b>"));
}

BOOST_AUTO_TEST_CASE(Image)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Image, image } from 'noto:core';

const a = image('https://imgs.xkcd.com/comics/metabolism.png');
assert(() => a instanceof Image);

assert(() => throws(() => new Image(), 'Image: no constructor'));
assert(() => throws(() => new Image(1), 'Image: no constructor'));
assert(() => throws(() => new Image(''), 'Image: no constructor'));
assert(() => throws(() => new Image({data: 1}), 'Image: no constructor'));

const b = image('https://imgs.xkcd.com/comics/metabolism.png');
assert(() => b instanceof Image);
assert(() => image(b.data) instanceof Image);

print(a, b);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Image"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "https://imgs.xkcd.com/comics/metabolism.png"));

    BOOST_TEST(!strcmp(out[0][1]["type"].GetString(), "notojs.Image"));
    BOOST_TEST(!strcmp(out[0][1]["data"].GetString(), "https://imgs.xkcd.com/comics/metabolism.png"));
}

BOOST_AUTO_TEST_CASE(Markdown)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Markdown, markdown } from 'noto:core';

const a = markdown('# Header');
assert(() => a instanceof Markdown);

assert(() => throws(() => new Markdown(), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown(1), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown(''), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown({data: 1}), 'Markdown: no constructor'));

const b = markdown('# Header');
assert(() => b instanceof Markdown);
assert(() => markdown(b.data) instanceof Markdown);

print(a, b);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
    BOOST_TEST(get_output() != std::nullopt);

    auto const &out = get_output()->get();
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "# Header"));

    BOOST_TEST(!strcmp(out[0][1]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][1]["data"].GetString(), "# Header"));
}

BOOST_AUTO_TEST_CASE(SVG)
{
    db();

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { SVG, icon } from 'noto:core';
import { svg } from 'core.so';

const a = await icon('ic/baseline-apple');
assert(() => a instanceof SVG);

assert(() => throws(() => new SVG(), 'SVG: no constructor'));
assert(() => throws(() => new SVG(1), 'SVG: no constructor'));
assert(() => throws(() => new SVG(''), 'SVG: no constructor'));
assert(() => throws(() => new SVG({data: 1}), 'SVG: no constructor'));

assert(() => svg(a.data) instanceof SVG);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(API)
{
    eval(R"JS(
import { HTML, Image, Markdown, SVG, XML } from 'noto:core';
import { assert, throws } from 'noto:assert';
import * as core from 'core.so';

const m = core.markdown("# Header");
assert(() => m instanceof Markdown);
assert(() => '# Header' == m.data);

const h = core.html('<i>italic</i>');
assert(() => h instanceof HTML);
assert(() => '<i>italic</i>' == h.data);

const s = core.svg('<svg></svg>');
assert(() => s instanceof SVG);
assert(() => '<svg></svg>' == s.data);

const t = core.svg();
assert(() => t instanceof SVG);
assert(() => '<svg></svg>' == t.data);

const x = core.xml('<root/>');
assert(() => x instanceof XML);
assert(() => '<root/>' == x.data);

const i  = core.image();
assert(() => i instanceof Image);
assert(() => 'data:application/octet-stream;base64,' == i.data);

const j = core.image('http://google.com/logo.png');
assert(() => j instanceof Image);
assert(() => 'http://google.com/logo.png' == j.data);

assert(() => !core.image('----asd213'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HTMLHasNoConstructor)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML } from 'noto:core';
import * as core from 'core.so';

const data = '<i>content</i>';
const value = core.html(data);
const message = 'HTML: no constructor';
let getterCalls = 0;
const config = {
    get data() {
        ++getterCalls;
        return data;
    }
};
class Derived extends HTML {
    constructor(...args) {
        super(...args);
    }
}

// No arguments.
assert(() => throws(() => new HTML(), message));
assert(() => throws(() => Reflect.construct(HTML, []), message));
assert(() => throws(() => Reflect.construct(HTML, [], Derived), message));
assert(() => throws(() => new Derived(), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML()));
assert(() => getterCalls === 0);

// Number.
assert(() => throws(() => new HTML(1), message));
assert(() => throws(() => Reflect.construct(HTML, [1]), message));
assert(() => throws(() => Reflect.construct(HTML, [1], Derived), message));
assert(() => throws(() => new Derived(1), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML(1)));
assert(() => getterCalls === 0);

// Empty string.
assert(() => throws(() => new HTML(''), message));
assert(() => throws(() => Reflect.construct(HTML, ['']), message));
assert(() => throws(() => Reflect.construct(HTML, [''], Derived), message));
assert(() => throws(() => new Derived(''), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML('')));
assert(() => getterCalls === 0);

// Numeric data object.
assert(() => throws(() => new HTML({data: 1}), message));
assert(() => throws(() => Reflect.construct(HTML, [{data: 1}]), message));
assert(() => throws(() => Reflect.construct(HTML, [{data: 1}], Derived), message));
assert(() => throws(() => new Derived({data: 1}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML({data: 1})));
assert(() => getterCalls === 0);

// String data object.
assert(() => throws(() => new HTML({data}), message));
assert(() => throws(() => Reflect.construct(HTML, [{data}]), message));
assert(() => throws(() => Reflect.construct(HTML, [{data}], Derived), message));
assert(() => throws(() => new Derived({data}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML({data})));
assert(() => getterCalls === 0);

// Existing content.
assert(() => throws(() => new HTML(value), message));
assert(() => throws(() => Reflect.construct(HTML, [value]), message));
assert(() => throws(() => Reflect.construct(HTML, [value], Derived), message));
assert(() => throws(() => new Derived(value), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML(value)));
assert(() => getterCalls === 0);

// Data getter is not invoked.
assert(() => throws(() => new HTML(config), message));
assert(() => throws(() => Reflect.construct(HTML, [config]), message));
assert(() => throws(() => Reflect.construct(HTML, [config], Derived), message));
assert(() => throws(() => new Derived(config), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => HTML(config)));
assert(() => getterCalls === 0);

assert(() => value instanceof HTML);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ImageHasNoConstructor)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Image } from 'noto:core';
import * as core from 'core.so';

const data = 'data:image/png;base64,AA==';
const value = core.image(data);
const message = 'Image: no constructor';
let getterCalls = 0;
const config = {
    get data() {
        ++getterCalls;
        return data;
    }
};
class Derived extends Image {
    constructor(...args) {
        super(...args);
    }
}

// No arguments.
assert(() => throws(() => new Image(), message));
assert(() => throws(() => Reflect.construct(Image, []), message));
assert(() => throws(() => Reflect.construct(Image, [], Derived), message));
assert(() => throws(() => new Derived(), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image()));
assert(() => getterCalls === 0);

// Number.
assert(() => throws(() => new Image(1), message));
assert(() => throws(() => Reflect.construct(Image, [1]), message));
assert(() => throws(() => Reflect.construct(Image, [1], Derived), message));
assert(() => throws(() => new Derived(1), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image(1)));
assert(() => getterCalls === 0);

// Empty string.
assert(() => throws(() => new Image(''), message));
assert(() => throws(() => Reflect.construct(Image, ['']), message));
assert(() => throws(() => Reflect.construct(Image, [''], Derived), message));
assert(() => throws(() => new Derived(''), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image('')));
assert(() => getterCalls === 0);

// Numeric data object.
assert(() => throws(() => new Image({data: 1}), message));
assert(() => throws(() => Reflect.construct(Image, [{data: 1}]), message));
assert(() => throws(() => Reflect.construct(Image, [{data: 1}], Derived), message));
assert(() => throws(() => new Derived({data: 1}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image({data: 1})));
assert(() => getterCalls === 0);

// String data object.
assert(() => throws(() => new Image({data}), message));
assert(() => throws(() => Reflect.construct(Image, [{data}]), message));
assert(() => throws(() => Reflect.construct(Image, [{data}], Derived), message));
assert(() => throws(() => new Derived({data}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image({data})));
assert(() => getterCalls === 0);

// Existing content.
assert(() => throws(() => new Image(value), message));
assert(() => throws(() => Reflect.construct(Image, [value]), message));
assert(() => throws(() => Reflect.construct(Image, [value], Derived), message));
assert(() => throws(() => new Derived(value), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image(value)));
assert(() => getterCalls === 0);

// Data getter is not invoked.
assert(() => throws(() => new Image(config), message));
assert(() => throws(() => Reflect.construct(Image, [config]), message));
assert(() => throws(() => Reflect.construct(Image, [config], Derived), message));
assert(() => throws(() => new Derived(config), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Image(config)));
assert(() => getterCalls === 0);

assert(() => value instanceof Image);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(MarkdownHasNoConstructor)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Markdown } from 'noto:core';
import * as core from 'core.so';

const data = '# Content';
const value = core.markdown(data);
const message = 'Markdown: no constructor';
let getterCalls = 0;
const config = {
    get data() {
        ++getterCalls;
        return data;
    }
};
class Derived extends Markdown {
    constructor(...args) {
        super(...args);
    }
}

// No arguments.
assert(() => throws(() => new Markdown(), message));
assert(() => throws(() => Reflect.construct(Markdown, []), message));
assert(() => throws(() => Reflect.construct(Markdown, [], Derived), message));
assert(() => throws(() => new Derived(), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown()));
assert(() => getterCalls === 0);

// Number.
assert(() => throws(() => new Markdown(1), message));
assert(() => throws(() => Reflect.construct(Markdown, [1]), message));
assert(() => throws(() => Reflect.construct(Markdown, [1], Derived), message));
assert(() => throws(() => new Derived(1), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown(1)));
assert(() => getterCalls === 0);

// Empty string.
assert(() => throws(() => new Markdown(''), message));
assert(() => throws(() => Reflect.construct(Markdown, ['']), message));
assert(() => throws(() => Reflect.construct(Markdown, [''], Derived), message));
assert(() => throws(() => new Derived(''), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown('')));
assert(() => getterCalls === 0);

// Numeric data object.
assert(() => throws(() => new Markdown({data: 1}), message));
assert(() => throws(() => Reflect.construct(Markdown, [{data: 1}]), message));
assert(() => throws(() => Reflect.construct(Markdown, [{data: 1}], Derived), message));
assert(() => throws(() => new Derived({data: 1}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown({data: 1})));
assert(() => getterCalls === 0);

// String data object.
assert(() => throws(() => new Markdown({data}), message));
assert(() => throws(() => Reflect.construct(Markdown, [{data}]), message));
assert(() => throws(() => Reflect.construct(Markdown, [{data}], Derived), message));
assert(() => throws(() => new Derived({data}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown({data})));
assert(() => getterCalls === 0);

// Existing content.
assert(() => throws(() => new Markdown(value), message));
assert(() => throws(() => Reflect.construct(Markdown, [value]), message));
assert(() => throws(() => Reflect.construct(Markdown, [value], Derived), message));
assert(() => throws(() => new Derived(value), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown(value)));
assert(() => getterCalls === 0);

// Data getter is not invoked.
assert(() => throws(() => new Markdown(config), message));
assert(() => throws(() => Reflect.construct(Markdown, [config]), message));
assert(() => throws(() => Reflect.construct(Markdown, [config], Derived), message));
assert(() => throws(() => new Derived(config), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => Markdown(config)));
assert(() => getterCalls === 0);

assert(() => value instanceof Markdown);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SVGHasNoConstructor)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { SVG } from 'noto:core';
import * as core from 'core.so';

const data = '<svg></svg>';
const value = core.svg(data);
const message = 'SVG: no constructor';
let getterCalls = 0;
const config = {
    get data() {
        ++getterCalls;
        return data;
    }
};
class Derived extends SVG {
    constructor(...args) {
        super(...args);
    }
}

// No arguments.
assert(() => throws(() => new SVG(), message));
assert(() => throws(() => Reflect.construct(SVG, []), message));
assert(() => throws(() => Reflect.construct(SVG, [], Derived), message));
assert(() => throws(() => new Derived(), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG()));
assert(() => getterCalls === 0);

// Number.
assert(() => throws(() => new SVG(1), message));
assert(() => throws(() => Reflect.construct(SVG, [1]), message));
assert(() => throws(() => Reflect.construct(SVG, [1], Derived), message));
assert(() => throws(() => new Derived(1), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG(1)));
assert(() => getterCalls === 0);

// Empty string.
assert(() => throws(() => new SVG(''), message));
assert(() => throws(() => Reflect.construct(SVG, ['']), message));
assert(() => throws(() => Reflect.construct(SVG, [''], Derived), message));
assert(() => throws(() => new Derived(''), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG('')));
assert(() => getterCalls === 0);

// Numeric data object.
assert(() => throws(() => new SVG({data: 1}), message));
assert(() => throws(() => Reflect.construct(SVG, [{data: 1}]), message));
assert(() => throws(() => Reflect.construct(SVG, [{data: 1}], Derived), message));
assert(() => throws(() => new Derived({data: 1}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG({data: 1})));
assert(() => getterCalls === 0);

// String data object.
assert(() => throws(() => new SVG({data}), message));
assert(() => throws(() => Reflect.construct(SVG, [{data}]), message));
assert(() => throws(() => Reflect.construct(SVG, [{data}], Derived), message));
assert(() => throws(() => new Derived({data}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG({data})));
assert(() => getterCalls === 0);

// Existing content.
assert(() => throws(() => new SVG(value), message));
assert(() => throws(() => Reflect.construct(SVG, [value]), message));
assert(() => throws(() => Reflect.construct(SVG, [value], Derived), message));
assert(() => throws(() => new Derived(value), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG(value)));
assert(() => getterCalls === 0);

// Data getter is not invoked.
assert(() => throws(() => new SVG(config), message));
assert(() => throws(() => Reflect.construct(SVG, [config]), message));
assert(() => throws(() => Reflect.construct(SVG, [config], Derived), message));
assert(() => throws(() => new Derived(config), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => SVG(config)));
assert(() => getterCalls === 0);

assert(() => value instanceof SVG);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(XMLHasNoConstructor)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { XML } from 'noto:core';
import * as core from 'core.so';

const data = '<root/>';
const value = core.xml(data);
const message = 'XML: no constructor';
let getterCalls = 0;
const config = {
    get data() {
        ++getterCalls;
        return data;
    }
};
class Derived extends XML {
    constructor(...args) {
        super(...args);
    }
}

// No arguments.
assert(() => throws(() => new XML(), message));
assert(() => throws(() => Reflect.construct(XML, []), message));
assert(() => throws(() => Reflect.construct(XML, [], Derived), message));
assert(() => throws(() => new Derived(), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML()));
assert(() => getterCalls === 0);

// Number.
assert(() => throws(() => new XML(1), message));
assert(() => throws(() => Reflect.construct(XML, [1]), message));
assert(() => throws(() => Reflect.construct(XML, [1], Derived), message));
assert(() => throws(() => new Derived(1), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML(1)));
assert(() => getterCalls === 0);

// Empty string.
assert(() => throws(() => new XML(''), message));
assert(() => throws(() => Reflect.construct(XML, ['']), message));
assert(() => throws(() => Reflect.construct(XML, [''], Derived), message));
assert(() => throws(() => new Derived(''), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML('')));
assert(() => getterCalls === 0);

// Numeric data object.
assert(() => throws(() => new XML({data: 1}), message));
assert(() => throws(() => Reflect.construct(XML, [{data: 1}]), message));
assert(() => throws(() => Reflect.construct(XML, [{data: 1}], Derived), message));
assert(() => throws(() => new Derived({data: 1}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML({data: 1})));
assert(() => getterCalls === 0);

// String data object.
assert(() => throws(() => new XML({data}), message));
assert(() => throws(() => Reflect.construct(XML, [{data}]), message));
assert(() => throws(() => Reflect.construct(XML, [{data}], Derived), message));
assert(() => throws(() => new Derived({data}), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML({data})));
assert(() => getterCalls === 0);

// Existing content.
assert(() => throws(() => new XML(value), message));
assert(() => throws(() => Reflect.construct(XML, [value]), message));
assert(() => throws(() => Reflect.construct(XML, [value], Derived), message));
assert(() => throws(() => new Derived(value), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML(value)));
assert(() => getterCalls === 0);

// Data getter is not invoked.
assert(() => throws(() => new XML(config), message));
assert(() => throws(() => Reflect.construct(XML, [config]), message));
assert(() => throws(() => Reflect.construct(XML, [config], Derived), message));
assert(() => throws(() => new Derived(config), message));
// QuickJS may reject a plain call before invoking the native callback.
assert(() => throws(() => XML(config)));
assert(() => getterCalls === 0);

assert(() => value instanceof XML);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HTMLContentSigns)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { html } from 'noto:core';

// UTF-8/WTF-8 reference implementation for a future browser-side signer.
function fnv1a(string) {
    let hash = 2166136261;
    for (const char of string) {
        const c = char.codePointAt(0);
        const bytes = c < 0x80 ? [c]
            : c < 0x800 ? [0xc0 | (c >> 6), 0x80 | (c & 63)]
            : c < 0x10000 ? [0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)]
            : [0xf0 | (c >> 18), 0x80 | ((c >> 12) & 63), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)];
        for (const byte of bytes) hash = Math.imul(hash ^ byte, 16777619);
    }
    return (hash >>> 0).toString(16).padStart(8, '0');
}
assert(() => fnv1a('hello') === '4f9f2cab');

// Empty string.
{
    const data = '';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// ASCII.
{
    const data = 'A';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Embedded null.
{
    const data = 'A\0B';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unicode.
{
    const data = 'café 😀';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate.
{
    const data = '\ud800';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired low surrogate.
{
    const data = '\udc00';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate followed by ASCII.
{
    const data = '\ud800X';
    const value = html(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => html(data).toJSON().sign === record.sign);
    assert(() => html(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ImageContentSigns)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { image } from 'noto:core';

// UTF-8/WTF-8 reference implementation for a future browser-side signer.
function fnv1a(string) {
    let hash = 2166136261;
    for (const char of string) {
        const c = char.codePointAt(0);
        const bytes = c < 0x80 ? [c]
            : c < 0x800 ? [0xc0 | (c >> 6), 0x80 | (c & 63)]
            : c < 0x10000 ? [0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)]
            : [0xf0 | (c >> 18), 0x80 | ((c >> 12) & 63), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)];
        for (const byte of bytes) hash = Math.imul(hash ^ byte, 16777619);
    }
    return (hash >>> 0).toString(16).padStart(8, '0');
}
assert(() => fnv1a('hello') === '4f9f2cab');

// Empty string.
{
    const data = 'data:text/plain,';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// ASCII.
{
    const data = 'data:text/plain,A';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Embedded null.
{
    const data = 'data:text/plain,A\0B';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unicode.
{
    const data = 'data:text/plain,café 😀';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate.
{
    const data = 'data:text/plain,\ud800';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired low surrogate.
{
    const data = 'data:text/plain,\udc00';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate followed by ASCII.
{
    const data = 'data:text/plain,\ud800X';
    const value = image(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Image' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Image' + '\0' + data));
    assert(() => image(data).toJSON().sign === record.sign);
    assert(() => image(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(MarkdownContentSigns)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { markdown } from 'noto:core';

// UTF-8/WTF-8 reference implementation for a future browser-side signer.
function fnv1a(string) {
    let hash = 2166136261;
    for (const char of string) {
        const c = char.codePointAt(0);
        const bytes = c < 0x80 ? [c]
            : c < 0x800 ? [0xc0 | (c >> 6), 0x80 | (c & 63)]
            : c < 0x10000 ? [0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)]
            : [0xf0 | (c >> 18), 0x80 | ((c >> 12) & 63), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)];
        for (const byte of bytes) hash = Math.imul(hash ^ byte, 16777619);
    }
    return (hash >>> 0).toString(16).padStart(8, '0');
}
assert(() => fnv1a('hello') === '4f9f2cab');

// Empty string.
{
    const data = '';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// ASCII.
{
    const data = 'A';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Embedded null.
{
    const data = 'A\0B';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unicode.
{
    const data = 'café 😀';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate.
{
    const data = '\ud800';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired low surrogate.
{
    const data = '\udc00';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate followed by ASCII.
{
    const data = '\ud800X';
    const value = markdown(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.Markdown' + '\0' + data));
    assert(() => markdown(data).toJSON().sign === record.sign);
    assert(() => markdown(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SVGContentSigns)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import * as core from 'core.so';

// UTF-8/WTF-8 reference implementation for a future browser-side signer.
function fnv1a(string) {
    let hash = 2166136261;
    for (const char of string) {
        const c = char.codePointAt(0);
        const bytes = c < 0x80 ? [c]
            : c < 0x800 ? [0xc0 | (c >> 6), 0x80 | (c & 63)]
            : c < 0x10000 ? [0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)]
            : [0xf0 | (c >> 18), 0x80 | ((c >> 12) & 63), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)];
        for (const byte of bytes) hash = Math.imul(hash ^ byte, 16777619);
    }
    return (hash >>> 0).toString(16).padStart(8, '0');
}
assert(() => fnv1a('hello') === '4f9f2cab');

// SVG intentionally retains its HTML serialization tag.
// Empty string.
{
    const data = '';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// ASCII.
{
    const data = 'A';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Embedded null.
{
    const data = 'A\0B';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unicode.
{
    const data = 'café 😀';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate.
{
    const data = '\ud800';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired low surrogate.
{
    const data = '\udc00';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate followed by ASCII.
{
    const data = '\ud800X';
    const value = core.svg(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.HTML' + '\0' + data));
    assert(() => core.svg(data).toJSON().sign === record.sign);
    assert(() => core.svg(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(XMLContentSigns)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { xml } from 'noto:core';

// UTF-8/WTF-8 reference implementation for a future browser-side signer.
function fnv1a(string) {
    let hash = 2166136261;
    for (const char of string) {
        const c = char.codePointAt(0);
        const bytes = c < 0x80 ? [c]
            : c < 0x800 ? [0xc0 | (c >> 6), 0x80 | (c & 63)]
            : c < 0x10000 ? [0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)]
            : [0xf0 | (c >> 18), 0x80 | ((c >> 12) & 63), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63)];
        for (const byte of bytes) hash = Math.imul(hash ^ byte, 16777619);
    }
    return (hash >>> 0).toString(16).padStart(8, '0');
}
assert(() => fnv1a('hello') === '4f9f2cab');

// Empty string.
{
    const data = '';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// ASCII.
{
    const data = 'A';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Embedded null.
{
    const data = 'A\0B';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unicode.
{
    const data = 'café 😀';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate.
{
    const data = '\ud800';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired low surrogate.
{
    const data = '\udc00';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}

// Unpaired high surrogate followed by ASCII.
{
    const data = '\ud800X';
    const value = xml(data);
    const record = value.toJSON();
    assert(() => typeof value.data === 'string');
    assert(() => record.type === 'notojs.XML' && record.data === data);
    assert(() => /^[0-9a-f]{8}$/.test(record.sign));
    assert(() => record.sign === fnv1a('notojs.XML' + '\0' + data));
    assert(() => xml(data).toJSON().sign === record.sign);
    assert(() => xml(data + '!').toJSON().sign !== record.sign);
    assert(() => JSON.parse(JSON.stringify(value)).sign === record.sign);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ContentSignTypeSeparation)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { html, markdown } from 'noto:core';
import * as core from 'core.so';

assert(() => html('same').toJSON().sign !== markdown('same').toJSON().sign);
assert(() => html('<svg/>').toJSON().sign === core.svg('<svg/>').toJSON().sign);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HTMLContentImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, html } from 'noto:core';
import * as core from 'core.so';

const data = '<i>immutable</i>';
const input = {data};
const fromInput = html(input.data);
input.data = 'changed input';
const fromFactory = html(data);
const fromNativeFactory = core.html(data);
const nativeCopyOfFactory = core.html(fromFactory.data);
const nativeCopyOfNativeFactory = core.html(fromNativeFactory.data);
const nativeCopyOfInput = core.html(fromInput.data);

// Factory result.
{
    const value = fromFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native factory result.
{
    const value = fromNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Content created before its input changes.
{
    const value = fromInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of factory result.
{
    const value = nativeCopyOfFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of native factory result.
{
    const value = nativeCopyOfNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of content created before its input changes.
{
    const value = nativeCopyOfInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => prototype === HTML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof HTML);
    assert(() => Object.isFrozen(value));
    const copy = core.html(value.data);
    assert(() => copy !== value && copy instanceof HTML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(ImageContentImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Image, image } from 'noto:core';
import * as core from 'core.so';

const data = 'data:image/png;base64,AA==';
const input = {data};
const fromInput = image(input.data);
input.data = 'changed input';
const fromFactory = image(data);
const fromNativeFactory = core.image(data);
const nativeCopyOfFactory = core.image(fromFactory.data);
const nativeCopyOfNativeFactory = core.image(fromNativeFactory.data);
const nativeCopyOfInput = core.image(fromInput.data);

// Factory result.
{
    const value = fromFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native factory result.
{
    const value = fromNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Content created before its input changes.
{
    const value = fromInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of factory result.
{
    const value = nativeCopyOfFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of native factory result.
{
    const value = nativeCopyOfNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of content created before its input changes.
{
    const value = nativeCopyOfInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Image' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => prototype === Image.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Image' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Image);
    assert(() => Object.isFrozen(value));
    const copy = core.image(value.data);
    assert(() => copy !== value && copy instanceof Image);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(MarkdownContentImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Markdown, markdown } from 'noto:core';
import * as core from 'core.so';

const data = '# Immutable';
const input = {data};
const fromInput = markdown(input.data);
input.data = 'changed input';
const fromFactory = markdown(data);
const fromNativeFactory = core.markdown(data);
const nativeCopyOfFactory = core.markdown(fromFactory.data);
const nativeCopyOfNativeFactory = core.markdown(fromNativeFactory.data);
const nativeCopyOfInput = core.markdown(fromInput.data);

// Factory result.
{
    const value = fromFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native factory result.
{
    const value = fromNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Content created before its input changes.
{
    const value = fromInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of factory result.
{
    const value = nativeCopyOfFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of native factory result.
{
    const value = nativeCopyOfNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of content created before its input changes.
{
    const value = nativeCopyOfInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.Markdown' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => prototype === Markdown.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.Markdown' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof Markdown);
    assert(() => Object.isFrozen(value));
    const copy = core.markdown(value.data);
    assert(() => copy !== value && copy instanceof Markdown);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(SVGContentImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { SVG } from 'noto:core';
import * as core from 'core.so';

const data = '<svg viewBox="0 0 10 20"></svg>';
const input = {data};
const fromInput = core.svg(input.data);
input.data = 'changed input';
const fromFactory = core.svg(data);
const fromNativeFactory = core.svg(data);
const nativeCopyOfFactory = core.svg(fromFactory.data);
const nativeCopyOfNativeFactory = core.svg(fromNativeFactory.data);
const nativeCopyOfInput = core.svg(fromInput.data);

// SVG intentionally retains its HTML serialization tag.
// Factory result.
{
    const value = fromFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native factory result.
{
    const value = fromNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Content created before its input changes.
{
    const value = fromInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of factory result.
{
    const value = nativeCopyOfFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of native factory result.
{
    const value = nativeCopyOfNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of content created before its input changes.
{
    const value = nativeCopyOfInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.HTML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => prototype === SVG.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.HTML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof SVG);
    assert(() => Object.isFrozen(value));
    const copy = core.svg(value.data);
    assert(() => copy !== value && copy instanceof SVG);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(XMLContentImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { XML, xml } from 'noto:core';
import * as core from 'core.so';

const data = '<root>immutable</root>';
const input = {data};
const fromInput = xml(input.data);
input.data = 'changed input';
const fromFactory = xml(data);
const fromNativeFactory = core.xml(data);
const nativeCopyOfFactory = core.xml(fromFactory.data);
const nativeCopyOfNativeFactory = core.xml(fromNativeFactory.data);
const nativeCopyOfInput = core.xml(fromInput.data);

// Factory result.
{
    const value = fromFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native factory result.
{
    const value = fromNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Content created before its input changes.
{
    const value = fromInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of factory result.
{
    const value = nativeCopyOfFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of native factory result.
{
    const value = nativeCopyOfNativeFactory;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}

// Native copy of content created before its input changes.
{
    const value = nativeCopyOfInput;
    const sign = value.toJSON().sign;
    const matchesContent = candidate => {
        const record = JSON.parse(JSON.stringify(candidate));
        return Object.keys(record).length === 3
            && record.type === 'notojs.XML' && record.data === data && record.sign === sign;
    };
    const prototype = Object.getPrototypeOf(value);
    const toJSON = value.toJSON;
    const symbol = Symbol('extra');
    const replacement = () => ({type: 'changed', data: 'changed'});
    const descriptor = Object.getOwnPropertyDescriptor(value, 'data');

    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => prototype === XML.prototype);
    assert(() => Object.getPrototypeOf(prototype) === Object.prototype);
    assert(() => Object.isExtensible(prototype));
    assert(() => !Object.isFrozen(prototype));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.isFrozen(value));
    assert(() => value.hasOwnProperty('data'));
    assert(() => descriptor.value === data);
    assert(() => descriptor.enumerable === true);
    assert(() => descriptor.writable === false);
    assert(() => descriptor.configurable === false);
    assert(() => !('get' in descriptor) && !('set' in descriptor));
    assert(() => Object.keys(value).join(',') === 'data');
    assert(() => matchesContent(value));

    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => throws(() => { delete value.data; }));
    assert(() => throws(() => Object.defineProperty(value, 'data', {value: 'changed'})));
    assert(() => throws(() => Object.defineProperty(value, 'data', {get: () => 'changed'})));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.deleteProperty(value, 'data'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !Reflect.defineProperty(value, 'data', {writable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {configurable: true}));
    assert(() => !Reflect.defineProperty(value, 'data', {enumerable: false}));

    assert(() => throws(() => { value.extra = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'extra', {value: replacement})));
    assert(() => !Reflect.set(value, 'extra', replacement));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: replacement}));
    assert(() => !value.hasOwnProperty('extra'));

    assert(() => throws(() => { value[symbol] = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, symbol, {value: replacement})));
    assert(() => !Reflect.set(value, symbol, replacement));
    assert(() => !Reflect.defineProperty(value, symbol, {value: replacement}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => throws(() => { value.toJSON = replacement; }));
    assert(() => throws(() => Object.defineProperty(value, 'toJSON', {value: replacement})));
    assert(() => !Reflect.set(value, 'toJSON', replacement));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: replacement}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => Object.getOwnPropertySymbols(value).length === 0);
    assert(() => throws(() => Object.setPrototypeOf(value, {})));
    assert(() => !Reflect.setPrototypeOf(value, {}));
    assert(() => !Reflect.setPrototypeOf(value, null));
    assert(() => throws(() => { value.__proto__ = {}; }));
    // Non-extensibility still permits operations that do not change the object.
    assert(() => Reflect.setPrototypeOf(value, prototype));
    assert(() => Reflect.defineProperty(value, 'data', {value: data}));

    const record = value.toJSON();
    assert(() => record.type === 'notojs.XML' && record.data === data);
    record.data = 'changed';
    record.type = 'changed';
    record.sign = 'changed';
    record.extra = true;
    assert(() => record.type === 'changed' && record.data === 'changed');
    assert(() => value.toJSON() !== record);
    assert(() => matchesContent(value.toJSON()));
    assert(() => matchesContent(value));
    assert(() => value.data === data);
    assert(() => value.toJSON === toJSON);
    assert(() => Object.getPrototypeOf(value) === prototype);
    assert(() => value instanceof XML);
    assert(() => Object.isFrozen(value));
    const copy = core.xml(value.data);
    assert(() => copy !== value && copy instanceof XML);
    assert(() => copy.data === data && Object.isFrozen(copy));
    assert(() => matchesContent(copy));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(HTMLNonserializableImmutability)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, html } from 'noto:core';
import { Document } from 'noto:dom';

const document = Document.html();
document.body.textContent = 'immutable body';
const value = html(document.body);
const data = value.data;
const descriptor = Object.getOwnPropertyDescriptor(value, '.json');
assert(() => value instanceof HTML);
assert(() => typeof data === 'string' && data.includes('immutable body'));
assert(() => Object.isFrozen(value));
assert(() => !Object.isExtensible(value));
assert(() => descriptor.value === false);
assert(() => descriptor.enumerable === false);
assert(() => descriptor.writable === false);
assert(() => descriptor.configurable === false);
assert(() => Object.keys(value).join(',') === 'data');
assert(() => throws(() => JSON.stringify(value), 'HTML cannot be serialized'));

assert(() => throws(() => { value['.json'] = true; }));
assert(() => throws(() => { delete value['.json']; }));
assert(() => throws(() => Object.defineProperty(value, '.json', {value: true})));
assert(() => !Reflect.set(value, '.json', true));
assert(() => !Reflect.deleteProperty(value, '.json'));
assert(() => !Reflect.defineProperty(value, '.json', {value: true}));
assert(() => !Reflect.defineProperty(value, '.json', {writable: true}));
assert(() => !Reflect.defineProperty(value, '.json', {configurable: true}));
assert(() => !Reflect.defineProperty(value, '.json', {enumerable: true}));
assert(() => !Reflect.set(value, 'data', '<p>changed</p>'));
assert(() => !Reflect.set(value, 'toJSON', () => ({data})));
assert(() => !Reflect.defineProperty(value, 'toJSON', {value: () => ({data})}));
assert(() => !Reflect.setPrototypeOf(value, {}));
assert(() => value['.json'] === false);
assert(() => value.data === data);
assert(() => Object.isFrozen(value));
assert(() => throws(() => value.toJSON(), 'HTML cannot be serialized'));
assert(() => throws(() => JSON.stringify(value), 'HTML cannot be serialized'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NativeContentImmutability)
{
    bridge::Context context{notojs::testing::engine->get_context()};
    auto *ctx = context.get();
    BOOST_REQUIRE(eval("import 'noto:core';", ctx, "native-content-init")
        == boost::beast::http::status::ok);

    bridge::Strong<bridge::Object> global{ctx, JS_GetGlobalObject(ctx)};
    bridge::Array empty{ctx};
    empty.append(notojs::HTML::ctor(ctx));
    empty.append(notojs::Image::ctor(ctx));
    empty.append(notojs::Markdown::ctor(ctx));
    empty.append(notojs::SVG::ctor(ctx));
    empty.append(notojs::XML::ctor(ctx));
    global.set("emptyContent", empty);
    global.set("objectContent", notojs::HTML::data(ctx, bridge::Object{ctx}));
    global.set("otherObjectContent", notojs::HTML::data(ctx, bridge::Object{ctx}));
    global.set("nonserializableHTML", notojs::HTML::data(ctx,
        bridge::String{ctx, std::string_view{"<p>native</p>"}}, false));

    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, Image, Markdown, SVG, XML } from 'noto:core';

const record = objectContent.toJSON();
let hash = 2166136261;
for (const char of 'notojs.HTML') hash = Math.imul(hash ^ char.charCodeAt(0), 16777619);
assert(() => record.sign === (hash >>> 0).toString(16).padStart(8, '0'));
assert(() => record.data === objectContent.data);
assert(() => record.sign === otherObjectContent.toJSON().sign);
objectContent.data.extra = 'changed';
objectContent.data.toString = () => { throw new Error('must not coerce object data'); };
assert(() => record.sign === objectContent.toJSON().sign);
assert(() => record.sign === emptyContent[0].toJSON().sign);

// Empty native HTML.
{
    const value = emptyContent[0];
    assert(() => value instanceof HTML);
    assert(() => value instanceof Object);
    assert(() => Object.isFrozen(value));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.keys(value).length === 0);
    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('data'));

    assert(() => !Reflect.set(value, 'extra', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('extra'));

    const symbol = Symbol('extra');
    assert(() => !Reflect.set(value, symbol, 'changed'));
    assert(() => !Reflect.defineProperty(value, symbol, {value: 'changed'}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => !Reflect.set(value, 'toJSON', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => !Reflect.setPrototypeOf(value, null));
}

// Empty native Image.
{
    const value = emptyContent[1];
    assert(() => value instanceof Image);
    assert(() => value instanceof Object);
    assert(() => Object.isFrozen(value));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.keys(value).length === 0);
    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('data'));

    assert(() => !Reflect.set(value, 'extra', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('extra'));

    const symbol = Symbol('extra');
    assert(() => !Reflect.set(value, symbol, 'changed'));
    assert(() => !Reflect.defineProperty(value, symbol, {value: 'changed'}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => !Reflect.set(value, 'toJSON', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => !Reflect.setPrototypeOf(value, null));
}

// Empty native Markdown.
{
    const value = emptyContent[2];
    assert(() => value instanceof Markdown);
    assert(() => value instanceof Object);
    assert(() => Object.isFrozen(value));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.keys(value).length === 0);
    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('data'));

    assert(() => !Reflect.set(value, 'extra', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('extra'));

    const symbol = Symbol('extra');
    assert(() => !Reflect.set(value, symbol, 'changed'));
    assert(() => !Reflect.defineProperty(value, symbol, {value: 'changed'}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => !Reflect.set(value, 'toJSON', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => !Reflect.setPrototypeOf(value, null));
}

// Empty native SVG.
{
    const value = emptyContent[3];
    assert(() => value instanceof SVG);
    assert(() => value instanceof Object);
    assert(() => Object.isFrozen(value));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.keys(value).length === 0);
    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('data'));

    assert(() => !Reflect.set(value, 'extra', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('extra'));

    const symbol = Symbol('extra');
    assert(() => !Reflect.set(value, symbol, 'changed'));
    assert(() => !Reflect.defineProperty(value, symbol, {value: 'changed'}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => !Reflect.set(value, 'toJSON', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => !Reflect.setPrototypeOf(value, null));
}

// Empty native XML.
{
    const value = emptyContent[4];
    assert(() => value instanceof XML);
    assert(() => value instanceof Object);
    assert(() => Object.isFrozen(value));
    assert(() => !Object.isExtensible(value));
    assert(() => Object.keys(value).length === 0);
    assert(() => throws(() => { value.data = 'changed'; }));
    assert(() => !Reflect.set(value, 'data', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'data', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('data'));

    assert(() => !Reflect.set(value, 'extra', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'extra', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('extra'));

    const symbol = Symbol('extra');
    assert(() => !Reflect.set(value, symbol, 'changed'));
    assert(() => !Reflect.defineProperty(value, symbol, {value: 'changed'}));
    assert(() => !value.hasOwnProperty(symbol));

    assert(() => !Reflect.set(value, 'toJSON', 'changed'));
    assert(() => !Reflect.defineProperty(value, 'toJSON', {value: 'changed'}));
    assert(() => !value.hasOwnProperty('toJSON'));
    assert(() => !Reflect.setPrototypeOf(value, null));
}

const value = nonserializableHTML;
const descriptor = Object.getOwnPropertyDescriptor(value, '.json');
assert(() => value instanceof HTML && value.data === '<p>native</p>');
assert(() => Object.isFrozen(value));
assert(() => descriptor.value === false && descriptor.enumerable === false);
assert(() => descriptor.writable === false && descriptor.configurable === false);
assert(() => Object.keys(value).join(',') === 'data');
assert(() => !Reflect.set(value, '.json', true));
assert(() => !Reflect.deleteProperty(value, '.json'));
assert(() => !Reflect.defineProperty(value, '.json', {value: true}));
assert(() => throws(() => JSON.stringify(value), 'HTML cannot be serialized'));
    )JS", ctx, "native-content-immutability");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NonContentRemainsExtensible)
{
    eval(R"JS(
import { assert } from 'noto:assert';

const encoder = new TextEncoder();
const symbol = Symbol('extra');
assert(() => Object.isExtensible(encoder));
assert(() => !Object.isFrozen(encoder));
encoder.extra = 'first';
encoder[symbol] = 'symbol value';
assert(() => encoder.extra === 'first' && encoder[symbol] === 'symbol value');
assert(() => Reflect.set(encoder, 'extra', 'second'));
assert(() => encoder.extra === 'second');
assert(() => Reflect.defineProperty(encoder, 'extra', {value: 'third'}));
assert(() => encoder.extra === 'third');
assert(() => Reflect.deleteProperty(encoder, 'extra'));
assert(() => !encoder.hasOwnProperty('extra'));
assert(() => encoder instanceof TextEncoder);
assert(() => encoder.encoding === 'utf-8');
assert(() => Array.from(encoder.encode('ABC')).join(',') === '65,66,67');
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
