#include <boost/test/unit_test.hpp>
#include <memory.hpp>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(CSS, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(style)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const s = d.body.style;

assert(() => s instanceof window.CSSStyleDeclaration);
assert(() => s instanceof window.CSSStyleProperties);

assert(() => s === d.body.style);
assert(() => 0 === s.length);
assert(() => '' === s.item(0));
assert(() => '' === s.getPropertyValue('width'));
assert(() => '' === s.width);
assert(() => 'width' in s);

d.body.setAttribute('style', 'width: 100px');
assert(() => 1 === s.length);
assert(() => 'width' === s.item(0));
assert(() => '100px' === s.width);
assert(() => '100px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));
assert(() => '' === s.getPropertyPriority('height'));

d.body.setAttribute('style', 'width: 200px; height: 1000px !important');
assert(() => 2 === s.length);
assert(() => 'width' === s.item(0));
assert(() => '200px' === s.width);
assert(() => '200px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));
assert(() => 'height' === s.item(1));
assert(() => '1000px' === s.height);
assert(() => '1000px' === s.getPropertyValue('height'));
assert(() => 'important' === s.getPropertyPriority('height'));
assert(() => '1000px' === s.removeProperty('height'));
assert(() => 1 === s.length);

s.setProperty('width', '300px');
assert(() => 1 === s.length);
assert(() => 'width' in s);
assert(() => 'width' === s.item(0));
assert(() => '300px' === s.width);
assert(() => '300px' === s.getPropertyValue('width'));
assert(() => '' === s.getPropertyPriority('width'));

s.setProperty('height', '2000px', 'asdf');
assert(() => 1 === s.length);

s.setProperty('height', '1000px', '');
assert(() => 2 === s.length);
assert(() => 'width' in s);
assert(() => 'height' in s);
assert(() => 'height' === s.item(1));
assert(() => '1000px' === s.height);
assert(() => '1000px' === s.getPropertyValue('height'));
assert(() => '' === s.getPropertyPriority('height'));

s.setProperty('width', '200px', 'important');
assert(() => 2 === s.length);
assert(() => 'width' === s.item(1));
assert(() => '200px' === s.width);
assert(() => '200px' === s.getPropertyValue('width'));
assert(() => 'important' === s.getPropertyPriority('width'));

d.body.removeAttribute('style');
assert(() => 0 === s.length);
assert(() => 'width' in s);
assert(() => 'height' in s);
assert(() => '' === s.width);
assert(() => '' === s.height);

s.cssText = 'width: 20px; float: right';
assert(() => 2 === s.length);
assert(() => 'width' in s);
assert(() => 'cssFloat' in s);
assert(() => 'height' in s);
assert(() => '' === s.height);
assert(() => 'width: 20px; float: right' === s.cssText);
assert(() => '20px' === s.width);
assert(() => '20px' === s.getPropertyValue('width'));
assert(() => 'right' === s.cssFloat);
assert(() => 'right' === s.getPropertyValue('float'));

s.cssFloat = 'left';
assert(() => 'left' === s.cssFloat);
assert(() => 'left' === s.getPropertyValue('float'));

s.backgroundColor = 'red';
assert(() => 'backgroundColor' in s);
assert(() => 'red' === s.backgroundColor);
assert(() => 'red' === s.getPropertyValue('background-color'));
assert(() => 'width: 20px; float: left; background-color: red;' === s.cssText);

const arr = Array.from(s);
assert(() => 3 === arr.length);
assert(() => arr.indexOf('width') >= 0);
assert(() => arr.indexOf('float') >= 0);
assert(() => arr.indexOf('background-color') >= 0);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(computedStyle)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const c = window.getComputedStyle(d.body);
const s = d.body.style;

assert(() => 'accent-color' === c.item(0));
assert(() => 'auto' === c.getPropertyValue(c.item(0)));
assert(() => c.item(0) === Array.from(c)[0]);
assert(() => 'transparent' === c.getPropertyValue('background-color'));
assert(() => '16px' === c.getPropertyValue('font-size'));
assert(() => '' === s.getPropertyValue('background-color'));
assert(() => '' === s.getPropertyValue('font-size'));
assert(() => 'transparent' === c.backgroundColor);
assert(() => '16px' === c.fontSize);
assert(() => '' === s.backgroundColor);

s.backgroundColor = 'red';
assert(() => '16px' === c.getPropertyValue('font-size'));
assert(() => 'red' === c.getPropertyValue('background-color'));
assert(() => 'red' === s.getPropertyValue('background-color'));
assert(() => 'red' === c.backgroundColor);
assert(() => '16px' === c.fontSize);
assert(() => 'red' === s.backgroundColor);

const style = d.body.appendChild(d.createElement('style'));
style.textContent = 'body {font-size: 10px}';

assert(() => '10px' === c.getPropertyValue('font-size'));
assert(() => '10px' === c.fontSize);

style.textContent += 'body.body {font-size: 12px}';

assert(() => '10px' === c.getPropertyValue('font-size'));
assert(() => '10px' === c.fontSize);

d.body.className = 'body';

assert(() => '12px' === c.getPropertyValue('font-size'));
assert(() => '12px' === c.fontSize);

style.textContent += '#body { font-size: 8px }';

assert(() => '12px' === c.getPropertyValue('font-size'));
assert(() => '12px' === c.fontSize);

d.body.id = 'body';

assert(() => '8px' === c.getPropertyValue('font-size'));
assert(() => '8px' === c.fontSize);

const d1 = d.body.appendChild(d.createElement('div'));
const dc = window.getComputedStyle(d1);

assert(() => '8px' === c.getPropertyValue('font-size'));
assert(() => '8px' === c.fontSize);

d.body.id = null;

assert(() => '12px' === dc.getPropertyValue('font-size'));
assert(() => '12px' === dc.fontSize);

d.body.classList.remove('body');
assert(() => '10px' === dc.getPropertyValue('font-size'));
assert(() => '10px' === dc.fontSize);

const svg = d.body.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'svg'));
const rect = svg.appendChild(d.createElementNS('http://www.w3.org/2000/svg', 'rect'));
const rcst = window.getComputedStyle(rect);

rect.setAttribute('fill', 'red');
assert(() => 'red' === rcst.fill);

style.sheet.replaceSync('rect { fill: blue; } #rect { fill: yellow }');
assert(() => 'blue' === rcst.fill);

rect.id = 'rect';
assert(() => 'yellow' === rcst.fill);

const N = c.length;
assert(() => N === rcst.length);

s.setProperty('all-style', 'custom');
assert(() => N + 1 === c.length);
assert(() => 'all-style' === c.item(N));

style.textContent = 'body { one-style: one }';
assert(() => N + 2 === c.length);
assert(() => 'all-style' === c.item(N));
assert(() => 'one-style' === c.item(N + 1));

const arr = Array.from(c);
assert(() => N + 2 === arr.length);
assert(() => arr.indexOf('all-style') == N);
assert(() => arr.indexOf('one-style') == N + 1);

const sheet = style.sheet;
await sheet.replace('body {font-size: 12px} div{font-size: 10px}');

const div = d.body.appendChild(d.createElement('div'));
assert(() => '10px' == window.getComputedStyle(div).fontSize);

style.textContent = 'body {font-size: 12px !important} div {font-size: 10px}';
assert(() => '10px' == window.getComputedStyle(div).fontSize);

style.textContent = 'body {font-size: 12px !important} .foo {font-size: 10px}';
div.className = 'foo';
assert(() => '10px' == window.getComputedStyle(div).fontSize);

style.textContent = 'body {font-size: 12px !important} .foo {font-size: 10px} #foo {font-size: 14px}';
div.id = 'foo';
assert(() => '14px' == window.getComputedStyle(div).fontSize);

style.textContent = 'body {font-size: 12px !important} .foo {font-size: 10px !important} #foo {font-size: 14px}';
assert(() => '10px' == window.getComputedStyle(div).fontSize);

sheet.deleteRule(0);
sheet.insertRule('body {font-size: 14px}');

assert(() => '10px' == window.getComputedStyle(div).fontSize);
assert(() => '14px' == c.fontSize);

assert(() => 3 == sheet.cssRules.length);
assert(() => 'body {font-size: 14px}' == sheet.cssRules[0].cssText);
assert(() => '.foo {font-size: 10px !important}' == sheet.cssRules[1].cssText);
assert(() => '#foo {font-size: 14px}' == sheet.cssRules[2].cssText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(cssCascade)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const style = d.head.appendChild(d.createElement('style'));
style.textContent = `
    div { color: red; }
    .target { color: blue; }
    .target { color: green; }
    #identified { color: purple; }

    .parent { font-size: 30px !important; }
    .child { font-size: 20px; }

    .important { background-color: blue !important; border-top-color: blue !important; }
    div, #comma { opacity: 0.5; }
    .comma { opacity: 0.7; }
`;

const classTarget = d.body.appendChild(d.createElement('div'));
classTarget.className = 'target';
assert(() => 'green' === window.getComputedStyle(classTarget).color);

const identified = d.body.appendChild(d.createElement('div'));
identified.id = 'identified';
identified.className = 'target';
assert(() => 'purple' === window.getComputedStyle(identified).color);

const parent = d.body.appendChild(d.createElement('section'));
parent.className = 'parent';
parent.style.letterSpacing = '3px';
parent.style.setProperty('--theme', 'dark');
const child = parent.appendChild(d.createElement('div'));
child.className = 'child';
assert(() => '20px' === window.getComputedStyle(child).fontSize);
assert(() => '3px' === window.getComputedStyle(child).letterSpacing);
assert(() => 'dark' === window.getComputedStyle(child).getPropertyValue('--theme'));

const important = d.body.appendChild(d.createElement('div'));
important.className = 'important';
important.style.backgroundColor = 'red';
important.style.setProperty('border-top-color', 'red', 'important');
const importantStyle = window.getComputedStyle(important);
assert(() => 'blue' === importantStyle.backgroundColor);
assert(() => 'red' === importantStyle.borderTopColor);

const comma = d.body.appendChild(d.createElement('div'));
comma.id = 'comma';
comma.className = 'comma';
assert(() => '0.5' === window.getComputedStyle(comma).opacity);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(computedStyleInvalidation)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const d = Document.html();
const target = d.body.appendChild(d.createElement('div'));
target.className = 'target';
const targetStyle = window.getComputedStyle(target);
const initialColor = targetStyle.color;

const detached = d.createElement('div');
const style = d.createElement('style');
style.textContent = '.target { color: red; }';
detached.appendChild(style);
assert(() => initialColor === targetStyle.color);

d.head.appendChild(style);
assert(() => 'red' === targetStyle.color);

style.remove();
assert(() => initialColor === targetStyle.color);

d.head.appendChild(style);
assert(() => 'red' === targetStyle.color);

detached.appendChild(style);
assert(() => initialColor === targetStyle.color);

const redParent = d.body.appendChild(d.createElement('div'));
redParent.style.color = 'red';
const blueParent = d.body.appendChild(d.createElement('div'));
blueParent.style.color = 'blue';
const subtree = redParent.appendChild(d.createElement('section'));
const inherited = subtree.appendChild(d.createElement('span'));
const inheritedStyle = window.getComputedStyle(inherited);
assert(() => 'red' === inheritedStyle.color);

blueParent.appendChild(subtree);
assert(() => 'blue' === inheritedStyle.color);

const structuralStyle = d.head.appendChild(d.createElement('style'));
structuralStyle.textContent = '.marker + .item { color: green; }';
const item = d.body.appendChild(d.createElement('div'));
item.className = 'item';
const itemStyle = window.getComputedStyle(item);
assert(() => initialColor === itemStyle.color);

const marker = d.createElement('div');
marker.className = 'marker';
d.body.insertBefore(marker, item);
assert(() => 'green' === itemStyle.color);

marker.remove();
assert(() => initialColor === itemStyle.color);

const sourceStyle = d.head.appendChild(d.createElement('style'));
sourceStyle.textContent = '.moved-rule { color: teal; }';
const destinationStyle = d.createElement('style');
const sourceSheet = sourceStyle.sheet;
const destinationSheet = destinationStyle.sheet;
const movedTarget = d.body.appendChild(d.createElement('div'));
movedTarget.className = 'moved-rule';
const movedTargetStyle = window.getComputedStyle(movedTarget);
assert(() => 'teal' === movedTargetStyle.color);
assert(() => 1 === sourceSheet.cssRules.length);
assert(() => 0 === destinationSheet.cssRules.length);

destinationStyle.appendChild(sourceStyle.firstChild);
assert(() => initialColor === movedTargetStyle.color);
assert(() => 0 === sourceSheet.cssRules.length);
assert(() => 1 === destinationSheet.cssRules.length);

d.head.appendChild(destinationStyle);
assert(() => 'teal' === movedTargetStyle.color);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(characterDataStyleInvalidation)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = '.target { color: red; }';
const target = document.body.appendChild(document.createElement('div'));
target.className = 'target';

const computed = window.getComputedStyle(target);
const sheet = style.sheet;
const rules = sheet.cssRules;
const text = style.firstChild;

assert(() => 'red' === computed.color);

text.data = '.target { color: blue; }';
assert(() => 'blue' === computed.color);
assert(() => '.target {color: blue}' === rules[0].cssText);

text.nodeValue = '.target { color: green; }';
assert(() => 'green' === computed.color);
assert(() => '.target {color: green}' === rules[0].cssText);

text.textContent = '.target { color: purple; }';
assert(() => 'purple' === computed.color);
assert(() => '.target {color: purple}' === rules[0].cssText);

text.data = '.target { color: orang; }';
text.insertData(text.data.indexOf('orang') + 5, 'e');
assert(() => 'orange' === computed.color);
assert(() => '.target {color: orange}' === rules[0].cssText);

text.data = '.target { color: rred; }';
text.deleteData(text.data.indexOf('rred'), 1);
assert(() => 'red' === computed.color);
assert(() => '.target {color: red}' === rules[0].cssText);

text.data = '.target { color: black; }';
text.replaceData(text.data.indexOf('black'), 5, 'teal');
assert(() => 'teal' === computed.color);
assert(() => '.target {color: teal}' === rules[0].cssText);

text.data = '.target { color: blu';
text.appendData('e; }');
assert(() => 'blue' === computed.color);
assert(() => '.target {color: blue}' === rules[0].cssText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(computedStyleRegistration)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { window, Document } from 'noto:dom';

const document = Document.html();
const target = document.body.appendChild(document.createElement('div'));
export const cssStyleRegistration = {
    target,
    first: window.getComputedStyle(target),
    second: window.getComputedStyle(target)
};
    )JS", context.get(), "css-style-registration-1");

    eval(R"JS(
cssStyleRegistration.first = null;
    )JS", context.get(), "css-style-registration-2");

    JS_RunGC(JS_GetRuntime(context.get()));

    eval(R"JS(
import { assert } from 'noto:assert';

cssStyleRegistration.target.style.color = 'red';
assert(() => 'red' === cssStyleRegistration.second.color);
    )JS", context.get(), "css-style-registration-3");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(sharedStyleSheetState)
{
    bridge::Context context{notojs::testing::engine->get_context()};

    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = '.shared-state { color: red; }';
const target = document.body.appendChild(document.createElement('div'));
target.className = 'shared-state';
const sheet = style.sheet;
const computed = window.getComputedStyle(target);
assert(() => 1 === sheet.cssRules.length);
assert(() => '.shared-state {color: red}' === sheet.cssRules[0].cssText);
assert(() => 'red' === computed.color);

export const sharedStyleSheet = { style, target, sheet, computed };
    )JS", context.get(), "shared-style-sheet-1");

    eval(R"JS(
sharedStyleSheet.sheet = null;
    )JS", context.get(), "shared-style-sheet-2");

    JS_RunGC(JS_GetRuntime(context.get()));

    eval(R"JS(
import { assert } from 'noto:assert';

sharedStyleSheet.style.textContent = '.shared-state { color: blue; }';
assert(() => 'blue' === sharedStyleSheet.computed.color);

const sheet = sharedStyleSheet.style.sheet;
const rules = sheet.cssRules;
assert(() => 1 === rules.length);
assert(() => '.shared-state {color: blue}' === rules[0].cssText);

sheet.insertRule('.shared-state { color: green !important; }', rules.length);
assert(() => 2 === rules.length);
assert(() => 'green' === sharedStyleSheet.computed.color);

sheet.deleteRule(1);
assert(() => 1 === rules.length);
assert(() => 'blue' === sharedStyleSheet.computed.color);
    )JS", context.get(), "shared-style-sheet-3");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(styleSheetMutation)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { window, Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = '.mutation { color: red; }';
const target = document.body.appendChild(document.createElement('div'));
target.className = 'mutation';
const computed = window.getComputedStyle(target);
const sheet = style.sheet;
const rules = sheet.cssRules;

assert(() => 1 === rules.length);
assert(() => 'red' === computed.color);
assert(() => throws(() => sheet.deleteRule(-1), 'IndexSizeError'));
assert(() => throws(() => sheet.deleteRule(rules.length), 'IndexSizeError'));
assert(() => throws(() => sheet.insertRule('.mutation {}', -1), 'IndexSizeError'));
assert(() => throws(() => sheet.insertRule('.mutation {}', rules.length + 1), 'IndexSizeError'));

const original = style.textContent;
assert(() => throws(() => sheet.insertRule(''), 'SyntaxError'));
assert(() => throws(() => sheet.insertRule('.one {} .two {}'), 'SyntaxError'));
assert(() => original === style.textContent);
assert(() => 1 === rules.length);
assert(() => 'red' === computed.color);

assert(() => 0 === sheet.insertRule('.mutation { color: blue; }', 0));
assert(() => 2 === rules.length);
assert(() => 'red' === computed.color);

assert(() => 2 === sheet.insertRule('.mutation { color: green; }', rules.length));
assert(() => 3 === rules.length);
assert(() => 'green' === computed.color);

sheet.deleteRule(2);
assert(() => 2 === rules.length);
assert(() => 'red' === computed.color);

const c1 = sheet.cssRules;
assert(() => c1 === sheet.cssRules);

sheet.replaceSync('.mutation { color: purple; }');
assert(() => 1 === rules.length);
assert(() => 'purple' === computed.color);
assert(() => c1 === sheet.cssRules);

const replaced = await sheet.replace('.mutation { color: orange; }');
assert(() => sheet === replaced);
assert(() => 1 === rules.length);
assert(() => 'orange' === computed.color);
assert(() => c1 === sheet.cssRules);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(invalidRulesAreNotExposed)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = '.super 1% { color: red; }';
const sheet = style.sheet;
const rules = sheet.cssRules;

assert(() => 0 === rules.length);
assert(() => null === rules.item(0));
assert(() => undefined === rules[0]);

style.textContent = `
    .before { color: red; }
    .super 1% { color: green; }
    .after { color: blue; }
`;
assert(() => 2 === rules.length);
assert(() => '.before {color: red}' === rules[0].cssText);
assert(() => '.after {color: blue}' === rules[1].cssText);

assert(() => 2 === sheet.insertRule('.inserted { color: purple; }', rules.length));
assert(() => 3 === rules.length);
assert(() => '.before {color: red}' === rules[0].cssText);
assert(() => '.after {color: blue}' === rules[1].cssText);
assert(() => '.inserted {color: purple}' === rules[2].cssText);

sheet.deleteRule(1);
assert(() => 2 === rules.length);
assert(() => '.before {color: red}' === rules[0].cssText);
assert(() => '.inserted {color: purple}' === rules[1].cssText);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(cssRuleType)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { window, Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = `
    .type { color: red; }
    @media screen {}
    @font-face {}
    @supports (display: grid) {}
`;
const sheet = style.sheet;
const rules = sheet.cssRules;
const styleRule = rules[0];
const mediaRule = rules[1];
const fontFaceRule = rules[2];
const supportsRule = rules[3];

assert(() => window.CSSRule.STYLE_RULE === styleRule.type);
assert(() => window.CSSRule.MEDIA_RULE === mediaRule.type);
assert(() => window.CSSRule.FONT_FACE_RULE === fontFaceRule.type);
assert(() => window.CSSRule.SUPPORTS_RULE === supportsRule.type);

sheet.replaceSync('.replacement {}');
assert(() => window.CSSRule.STYLE_RULE === styleRule.type);
assert(() => window.CSSRule.MEDIA_RULE === mediaRule.type);
assert(() => window.CSSRule.FONT_FACE_RULE === fontFaceRule.type);
assert(() => window.CSSRule.SUPPORTS_RULE === supportsRule.type);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(sheetBackedRules)
{
    eval(R"JS(
import { assert } from 'noto:assert';
import { Document } from 'noto:dom';

const document = Document.html();
const style = document.head.appendChild(document.createElement('style'));
style.textContent = '.first { color: red; } .second { color: blue; }';
const sheet = style.sheet;
const rules = sheet.cssRules;
const first = rules[0];
const second = rules.item(1);

assert(() => first === rules.item(0));
assert(() => second === rules[1]);
assert(() => sheet === first.parentStyleSheet);
assert(() => sheet === second.parentStyleSheet);
assert(() => null === first.parentRule);
assert(() => '.first {color: red}' === first.cssText);
assert(() => '.second {color: blue}' === second.cssText);

sheet.insertRule('.inserted { color: green; }', 0);
assert(() => 3 === rules.length);
assert(() => first === rules[1]);
assert(() => second === rules[2]);
assert(() => '.first {color: red}' === first.cssText);

sheet.deleteRule(0);
assert(() => first === rules[0]);
assert(() => second === rules[1]);

sheet.deleteRule(0);
assert(() => 1 === rules.length);
assert(() => null === first.parentStyleSheet);
assert(() => '.first {color: red}' === first.cssText);
assert(() => second === rules[0]);

sheet.replaceSync('.replacement { color: purple; }');
assert(() => null === second.parentStyleSheet);
assert(() => '.second {color: blue}' === second.cssText);
assert(() => 1 === rules.length);
assert(() => '.replacement {color: purple}' === rules[0].cssText);
assert(() => first !== rules[0]);
assert(() => second !== rules[0]);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
