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
    assert(() => e.code === 42);
}

try {
    throw new TestError();
} catch(e) {
    assert(() => e instanceof Error);
    assert(() => e instanceof TestError);
}

try {
    throw new TestError(1);
} catch(e) {
    assert(() => e instanceof Error);
    assert(() => !(e instanceof TestError));
}

try {
    throw new TestError({code: 42});
} catch(e) {
    assert(() => e instanceof Error);
    assert(() => e instanceof TestError);
    assert(() => e.code === 42);
}
    )JS");

    BOOST_TEST(get_error() == std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
