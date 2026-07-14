#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Application, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Responses)
{
    eval(R"JS(
import { html, image, xml } from 'noto:core';
import { Document } from 'noto:dom';
import { assert } from 'noto:assert';

const response = new ServerResponse();

response.body = 'Test';
assert(() => null == response.headers.get('Content-Type'));

const doc = Document.html();
doc.title = 'Hello';

response.body = html(doc);
assert(() => 'text/html' == response.headers.get('Content-Type'));

const text = await response.text();
assert(() => 'Hello' == Document.html(text).title);

const svg = doc.createElementNS("http://www.w3.org/2000/svg", "svg");
response.body = image(svg);

assert(() => 'image/svg+xml' == response.headers.get('Content-Type'));

response.body = {'name': 'foo', 'value': 42};
assert(() => 'application/json' == response.headers.get('Content-Type'));

response.body = xml(Document.xml('<root/>'));
assert(() => 'text/xml' == response.headers.get('Content-Type'));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
