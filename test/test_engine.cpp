#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Engine, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Preprocessor)
{
    std::string code{"<[[\n]]>"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code == "print($(\"\"));/*[\n]*/");
}

BOOST_AUTO_TEST_CASE(UnnamedMarkdown)
{
    std::string code{R"JS(
import { assert } from 'noto:assert';
let interpolations = 0;
<[[
# "Quoted" and 'single-quoted'
C:\notes\file.md
`${++interpolations}` stays literal.
]]>
assert(() => interpolations === 0);
)JS"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code.find("print($(\"") != std::string::npos);
    eval(code.c_str());

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 1u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), R"MD(# "Quoted" and 'single-quoted'
C:\notes\file.md
`${++interpolations}` stays literal.)MD"));
}

BOOST_AUTO_TEST_CASE(NamedMarkdown)
{
    std::string code{R"JS(
import { assert } from 'noto:assert';
import { Markdown } from 'noto:core';
<[note[
# Named
]]>
assert(() => note instanceof Markdown);
assert(() => Object.isFrozen(note));
assert(() => note.data === '# Named');
print(note);
)JS"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code.find("const note = $(\"# Named\");") != std::string::npos);
    eval(code.c_str());

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 1u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "# Named"));
}

BOOST_AUTO_TEST_CASE(ExportedMarkdown)
{
    std::string code{R"JS(
import { assert } from 'noto:assert';
import { Markdown } from 'noto:core';
<[!note[
# Exported
]]>
assert(() => note instanceof Markdown);
assert(() => Object.isFrozen(note));
assert(() => note.data === '# Exported');
print(note);
)JS"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code.find("export const note = $(\"# Exported\");") != std::string::npos);
    eval(code.c_str());

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 1u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), "# Exported"));
}

BOOST_AUTO_TEST_CASE(SlideMarkdown)
{
    std::string code{"<[:[\n# Slide\n]]>"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code.find("print[':']($(\"# Slide\"));") == 0u);
    eval(code.c_str());

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 1u);
    BOOST_REQUIRE(out[0].Size() == 2u);
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Grid"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), ":"));
    BOOST_TEST(!strcmp(out[0][1]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][1]["data"].GetString(), "# Slide"));
}

BOOST_AUTO_TEST_CASE(EchoMarkdown)
{
    std::string code{R"JS(
import { assert } from 'noto:assert';
let executions = 0;
<[![
print(`run ${++executions}: "quoted" \\path`);
]]>
assert(() => executions === 1);
)JS"};
    notojs::Engine::preprocess(code);
    BOOST_TEST(code.find("print($(\"```js!noplay\\n") != std::string::npos);
    eval(code.c_str());

    BOOST_REQUIRE(get_error() == std::nullopt);
    BOOST_REQUIRE(get_output() != std::nullopt);
    auto const &out = get_output()->get();
    BOOST_REQUIRE(out.Size() == 2u);
    BOOST_REQUIRE(out[0].Size() == 1u);
    BOOST_REQUIRE(out[1].Size() == 1u);
    BOOST_TEST(!strcmp(out[0][0]["type"].GetString(), "notojs.Markdown"));
    BOOST_TEST(!strcmp(out[0][0]["data"].GetString(), R"MD(```js!noplay
print(`run ${++executions}: "quoted" \\path`);
```)MD"));
    BOOST_TEST(!strcmp(out[1][0].GetString(), R"TXT(run 1: "quoted" \path)TXT"));
}

BOOST_AUTO_TEST_CASE(MarkdownConstructorsDisabled)
{
    std::string code{R"JS(
import { assert, throws } from 'noto:assert';
import { Markdown } from 'noto:core';
<[note[
# Factory-created
]]>
assert(() => note instanceof Markdown);
assert(() => Object.isFrozen(note));
assert(() => throws(() => new Markdown(), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown('text'), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown({data: 'text'}), 'Markdown: no constructor'));
assert(() => throws(() => new Markdown(note), 'Markdown: no constructor'));
)JS"};
    notojs::Engine::preprocess(code);
    eval(code.c_str());

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
