#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Setter, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Setters)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Variant } from 'setter.so';

const v = new Variant();
assert(() => throws(() =>  { v.value = {}; }, "Variant: no matching setter found"));

v.value = 10;
assert(() => "10" == v.value);

v.value = "dummy";
assert(() => "dummy" == v.value);

const w = new Variant(v);
assert(() => "dummy" == w.value);

w.value = 10;
assert(() => "10" == w.value);
assert(() => "dummy" == v.value);

w.value = v;
assert(() => "dummy" == w.value);

w.value = w;
assert(() => "dummy" == w.value);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
