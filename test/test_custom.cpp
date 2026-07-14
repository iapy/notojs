#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Custom, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Creation)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Custom, consume, factory } from 'custom.so';

const a = new Custom();
assert(() => a instanceof Custom);

const b = new Custom({data: 'test'});
assert(() => b instanceof Custom);
assert(() => 'test' == b.data);

const c = factory();
assert(() => c instanceof Custom);
assert(() => 'default' == c.data);

const d = factory('custom');
assert(() => d instanceof Custom);
assert(() => 'custom' == d.data);

const e = d.toJSON();
assert(() => 'Custom' == e.type);
assert(() => 'custom' == e.data);
assert(() => 'custom' == consume(d));
assert(() => throws(() => consume({})));
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
