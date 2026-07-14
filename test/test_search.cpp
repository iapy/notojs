#include <boost/test/unit_test.hpp>
#include <notojs/parser/search.hpp>
#include <memory.hpp>
#include <optional>
#include <string>
#include <vector>

#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(SearchParams, notojs::testing::Fixture)

struct Handler
{
    BOOST_FORCEINLINE void key(std::string &&s)
    {
        result.emplace_back(std::make_pair(std::move(s), std::nullopt));
    }
    BOOST_FORCEINLINE void val(std::string &&s)
    {
        result.back().second = std::move(s);
    }

    std::vector<std::pair<std::string, std::optional<std::string>>> &result;
};

BOOST_AUTO_TEST_CASE(Parser)
{
    using namespace notojs::parser;

    std::vector<std::pair<std::string, std::optional<std::string>>> result;
    (void)Search<Handler>{result}.parse("a=1");

    BOOST_TEST(1 == result.size());
    BOOST_TEST("a" == result[0].first);
    BOOST_TEST("1" == *result[0].second);

    result.clear();
    (void)Search<Handler>{result}.parse("a=1&b=2");

    BOOST_TEST(2 == result.size());
    BOOST_TEST("a" == result[0].first);
    BOOST_TEST("1" == *result[0].second);
    BOOST_TEST("b" == result[1].first);
    BOOST_TEST("2" == *result[1].second);

    result.clear();
    (void)Search<Handler>{result}.parse("x=a+b");

    BOOST_TEST(1 == result.size());
    BOOST_TEST("x" == result[0].first);
    BOOST_TEST("a b" == *result[0].second);

    result.clear();
    (void)Search<Handler>{result}.parse("y=a%20b");

    BOOST_TEST(1 == result.size());
    BOOST_TEST("y" == result[0].first);
    BOOST_TEST("a b" == *result[0].second);

    result.clear();
    (void)Search<Handler>{result}.parse("url=https%3a%2F%2Fgoogle.com");

    BOOST_TEST(1 == result.size());
    BOOST_TEST("url" == result[0].first);
    BOOST_TEST("https://google.com" == *result[0].second);
}

BOOST_AUTO_TEST_CASE(JS)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';

const fd = new URLSearchParams();
assert(() => !fd.has("value"));

fd.append("value", "10");
assert(() => fd.has("value"));
assert(() => "10" == fd.get("value"));

fd.append("value", "20");
assert(() => fd.has("value"));
assert(() => 2 == fd.getAll("value").length);

assert(() => 1 == [...fd.keys()].length);
assert(() => 2 == [...fd.values()].length);

fd.set("value", "30");
assert(() => fd.has("value"));
assert(() => 1 == fd.getAll("value").length);
assert(() => "30" == fd.get("value"));

fd.append("value", "40");
assert(() => fd.has("value"));
assert(() => 2 == fd.getAll("value").length);
assert(() => "30" == fd.get("value"));
assert(() => "30" == fd.getAll("value")[0]);
assert(() => "40" == fd.getAll("value")[1]);

fd.set("value", "50");
assert(() => fd.has("value"));
assert(() => 1 == fd.getAll("value").length);

fd.delete("value");
assert(() => !fd.has("value"));
assert(() => 0 == [...fd.keys()].length);

fd.append("foo", "bar");
fd.append("bar", "foo");

const k0 = [...fd.keys()];
assert(() => "foo" == k0[0]);
assert(() => "bar" == k0[1]);

fd.sort();

const k1 = [...fd.keys()];
assert(() => "bar" == k1[0]);
assert(() => "foo" == k1[1]);

assert(() => "bar=foo&foo=bar" == `${fd}`);
assert(() => "bar=foo&foo=bar" == fd.toJSON());
assert(() => "bar=foo&foo=bar" == fd.toString());
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
