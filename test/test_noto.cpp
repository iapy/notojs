#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Noto, notojs::testing::ContextFixture)

BOOST_AUTO_TEST_CASE(Notebook)
{
    auto const status = eval(R"JS(
import { assert } from 'noto:assert';
import { HTML } from 'noto:core';
import { notebook, Output } from 'noto:noto';

const b = notebook('Notebook');
const output = await b.exec();
assert(() => output instanceof Output);
assert(() => output.length === 0);
assert(() => !Array.isArray(output));
assert(() => [...output].length === 0);
let calls = 0;
output.forEach(() => ++calls);
assert(() => calls === 0);

for (const update of [false, true]) {
    const defaultOutput = await b.exec({update});
    assert(() => defaultOutput instanceof Output);
    assert(() => defaultOutput.length === 0);

    const json = await b.exec({update, result: JSON});
    assert(() => json instanceof Output);
    assert(() => json.length === 0);

    const html = await b.exec({update, result: HTML});
    assert(() => html instanceof HTML);
    assert(() => html.data === '<html></html>');

    const minimal = await b.exec({update, result: null});
    assert(() => minimal === null);
}
    )JS");

    BOOST_TEST(status == boost::beast::http::status::ok);
    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(NotebookLoad)
{
    auto const status = eval(R"JS(
import { assert, throws } from 'noto:assert';
import { HTML } from 'noto:core';
import { notebook, Output } from 'noto:noto';

const b = notebook('Notebook');
const output = await b.load();
assert(() => output instanceof Output);
assert(() => output.length === 0);

const defaultOutput = await b.load({});
assert(() => defaultOutput instanceof Output);
assert(() => defaultOutput.length === 0);

const json = await b.load({result: JSON});
assert(() => json instanceof Output);
assert(() => json.length === 0);

const html = await b.load({result: HTML});
assert(() => html instanceof HTML);
assert(() => html.data === '<html></html>');

assert(() => throws(() => b.load({result: null})));
assert(() => throws(() => b.load({result: 'text/html'})));
assert(() => throws(() => b.load({result: html})));

await print(b);
    )JS");

    BOOST_TEST(status == boost::beast::http::status::ok);
    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_CASE(Output)
{
    auto const status = eval(R"JS(
import { assert, throws } from 'noto:assert';
import { notebook, Output, Cell } from 'noto:noto';

const output = await notebook('Output').load();
assert(() => output instanceof Output);
assert(() => !Array.isArray(output));
assert(() => output.length === 2);

assert(() => output[0] instanceof Cell);
assert(() => output[0] === output['cell-000']);
assert(() => output[1] === output['cell-001']);
assert(() => Object.keys(output).join(',') === 'cell-000,cell-001');
assert(() => 0 in output && 'cell-000' in output);
assert(() => Object.getOwnPropertyDescriptor(output, '0').value === output['cell-000']);
assert(() => Object.getOwnPropertyDescriptor(output, 'length').writable === false);

for (const key of ['2', 'cell-002', '-1', '00', '01', '1.0', '1e0', 'cell-1', 'cell-0000'])
    assert(() => output[key] === undefined);

const names = [];
for (const name in output) names.push(name);
assert(() => names.join(',') === 'cell-000,cell-001');
for (let i = 0; i < output.length; ++i)
    assert(() => output[i] === output['cell-00' + i]);

const cells = [];
for (const cell of output) cells.push(cell);
assert(() => cells.length === 2 && cells[0] === output[0] && cells[1] === output[1]);
assert(() => Array.from(output.values())[1] === output[1]);
const iterator = output.values();
assert(() => iterator[Symbol.iterator]() === iterator);
assert(() => iterator.next().value === output[0]);
assert(() => iterator.next().value === output[1]);
assert(() => iterator.next().done && iterator.next().done);

const indices = [];
const self = {};
const result = output.forEach(function (cell, index, source) {
    assert(() => this === self && source === output && cell === output[index]);
    indices.push(index);
}, self);
assert(() => result === undefined && indices.join(',') === '0,1');
let calls = 0;
output.forEach(() => ++calls);
assert(() => calls === 2);

const error = new Error('stop');
calls = 0;
let caught;
try { output.forEach(() => { ++calls; throw error; }); }
catch (e) { caught = e; }
assert(() => caught === error && calls === 1);
assert(() => throws(() => output.forEach(null)));
assert(() => throws(() => Output.prototype.values.call({})));
assert(() => throws(() => new Output()));

print(output);
print(output['cell-001']);
await print(notebook('Output'));

Object.defineProperty(output, 'cell-000', {get() { throw error; }});
caught = undefined;
try { output.values().next(); } catch (e) { caught = e; }
assert(() => caught === error);
    )JS");

    std::cout << *get_error() << '\n';

    BOOST_TEST(status == boost::beast::http::status::ok);
    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &rows = get_output()->get();
    BOOST_REQUIRE(rows.Size() == 5u);
    BOOST_TEST(std::string(rows[0][0].GetString()) == "first");
    BOOST_TEST(std::string(rows[1][0].GetString()) == "second");
    BOOST_TEST(std::string(rows[2][0].GetString()) == "second");
    BOOST_TEST(std::string(rows[3][0].GetString()) == "first");
    BOOST_TEST(std::string(rows[4][0].GetString()) == "second");
}

BOOST_AUTO_TEST_CASE(Cell)
{
    auto const status = eval(R"JS(
import { assert, throws } from 'noto:assert';
import { notebook, Cell } from 'noto:noto';

const cell = (await notebook('Output').load())['cell-000'];
assert(() => cell instanceof Cell && !Array.isArray(cell));
assert(() => cell.name === 'cell-000');
assert(() => Array.isArray(cell.data));
assert(() => cell.data[0].type === 'notojs.Output');
assert(() => cell.data[0].data[0][0] === 'first');
assert(() => Object.keys(cell).join(',') === 'name,data');
assert(() => !Object.prototype.hasOwnProperty.call(cell, '.allocator'));
assert(() => throws(() => new Cell()));
assert(() => throws(() => { cell.name = 'changed'; }));
assert(() => throws(() => { cell.data = []; }));

print(cell);
const data = (await notebook('Output').load())['cell-001'].data;
assert(() => data[0].data[0][0] === 'second');

// The parsed data is a normal JS property, including for cycle collection.
cell.data.push(cell);
    )JS");

    BOOST_TEST(status == boost::beast::http::status::ok);
    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &rows = get_output()->get();
    BOOST_REQUIRE(rows.Size() == 1u);
    BOOST_TEST(std::string(rows[0][0].GetString()) == "first");
}

BOOST_AUTO_TEST_CASE(OutputResponseValidation)
{
    auto const status = eval(R"JS(
import { assert } from 'noto:assert';
import { notebook, Output, Cell } from 'noto:noto';

const remote = notebook('Output');
const output = await remote.exec({input: ['[]', '[]']});
assert(() => output instanceof Output && output.length === 2);
assert(() => output[0] instanceof Cell && output[0] === output['cell-000']);

for (const [input, type] of [
    ['{', SyntaxError],
    [{}, TypeError],
    [[42], TypeError],
    [['{'], SyntaxError],
    [['{}'], TypeError],
    [Array(1001).fill('[]'), RangeError]
]) {
    let caught;
    try { await remote.exec({input}); } catch (error) { caught = error; }
    assert(() => caught instanceof type);
}

const full = await remote.exec({input: Array(1000).fill('[]')});
assert(() => full.length === 1000);
assert(() => full[999] === full['cell-999']);
assert(() => full[1000] === undefined && full['cell-1000'] === undefined);
    )JS");

    BOOST_TEST(status == boost::beast::http::status::ok);
    BOOST_TEST(get_error() == std::nullopt);
}


BOOST_AUTO_TEST_SUITE_END()
