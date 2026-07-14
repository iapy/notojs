#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(NotoJS, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(HTML)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML, html } from 'noto:core';

const a = html('<i>test</i>');
assert(() => a instanceof HTML);

assert(() => throws(() => new HTML(), 'HTML: no matching constructor found'));
assert(() => throws(() => new HTML(1), 'HTML: no matching constructor found'));
assert(() => throws(() => new HTML(''), 'HTML: no matching constructor found'));
assert(() => throws(() => new HTML({data: 1}), 'HTML: no matching constructor found'));

const b = new HTML({data: '<b>bold</b>'});
assert(() => b instanceof HTML);
assert(() => new HTML(b) instanceof HTML);

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

assert(() => throws(() => new Image(), 'Image: no matching constructor found'));
assert(() => throws(() => new Image(1), 'Image: no matching constructor found'));
assert(() => throws(() => new Image(''), 'Image: no matching constructor found'));
assert(() => throws(() => new Image({data: 1}), 'Image: no matching constructor found'));

const b = new Image({data: 'https://imgs.xkcd.com/comics/metabolism.png'});
assert(() => b instanceof Image);
assert(() => new Image(b) instanceof Image);

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

assert(() => throws(() => new Markdown(), '__Markdown: no matching constructor found'));
assert(() => throws(() => new Markdown(1), '__Markdown: no matching constructor found'));
assert(() => throws(() => new Markdown(''), '__Markdown: no matching constructor found'));
assert(() => throws(() => new Markdown({data: 1}), '__Markdown: no matching constructor found'));

const b = new Markdown({data: '# Header'});
assert(() => b instanceof Markdown);
assert(() => new Markdown(b) instanceof Markdown);

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

const a = await icon('ic/baseline-apple');
assert(() => a instanceof SVG);

assert(() => throws(() => new SVG(), 'SVG: no matching constructor found'));
assert(() => throws(() => new SVG(1), 'SVG: no matching constructor found'));
assert(() => throws(() => new SVG(''), 'SVG: no matching constructor found'));
assert(() => throws(() => new SVG({data: 1}), 'SVG: no matching constructor found'));

assert(() => new SVG(a) instanceof SVG);
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

BOOST_AUTO_TEST_SUITE_END()
