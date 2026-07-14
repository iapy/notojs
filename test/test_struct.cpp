#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Struct, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Struct)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { Configurable } from 'struct.so';

assert(() => throws(() => new Configurable(), "Configurable: no matching constructor found"));
assert(() => throws(() => new Configurable({path: 1}), "Configurable: no matching constructor found"));
assert(() => throws(() => new Configurable({body: 1}), "Configurable: no matching constructor found"));
assert(() => throws(() => new Configurable({method: 'XXX'}), "Configurable constructor: invalid method [XXX]"));

let a = new Configurable({});
assert(() => '' == a.config);

let b = new Configurable({path: 'foo'});
assert(() => 'path=foo' == b.config);

let c = new Configurable({path: 'foo', method: 'GET'});
assert(() => 'method=GET;path=foo' == c.config);

let d = new Configurable({path: 'foo', method: 'POST', body: 'foobar'});
assert(() => 'body=foobar;method=POST;path=foo' == d.config);

let e = new Configurable({path: 'foo', method: 'POST', body: {'foo':'bar'}});
assert(() => 'body={"foo":"bar"};method=POST;path=foo' == e.config);
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
