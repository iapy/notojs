#include <boost/test/unit_test.hpp>
#include "test_engine.hpp"

BOOST_FIXTURE_TEST_SUITE(Errors, notojs::testing::Fixture)

BOOST_AUTO_TEST_CASE(Throw)
{
    eval(R"JS(
import { assert, throws } from 'noto:assert';
import { raise, TestError } from 'errors.so';

assert(() => throws(() => raise()));

try {
    raise();
} catch(e) {
    assert(() => e instanceof Error);
    assert(() => e instanceof TestError);
    assert(() => e.name === 'TestError');
    assert(() => e.constructor === TestError);
    assert(() => Object.getPrototypeOf(e) === TestError.prototype);
    assert(() => e.code === 42);
}

for(const args of [[], [1], [{code: 42}]]) {
    try {
        Reflect.construct(TestError, args);
        assert(() => false);
    } catch(e) {
        assert(() => e instanceof TypeError);
        assert(() => !(e instanceof TestError));
    }
}

class DerivedError extends TestError {}
try {
    new DerivedError();
    assert(() => false);
} catch(e) {
    assert(() => e instanceof TypeError);
    assert(() => !(e instanceof TestError));
    assert(() => !(e instanceof DerivedError));
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
