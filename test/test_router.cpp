#include <boost/test/unit_test.hpp>
#include <notojs/detail/router.hpp>

BOOST_AUTO_TEST_SUITE(Router)

constexpr auto router = notojs::detail::router(
    notojs::detail::route("/foo"),
    notojs::detail::route("/bar/")
);

BOOST_AUTO_TEST_CASE(Test)
{
    std::string tail;
    BOOST_TEST(router["/foo"] == router.find("/foo", tail));
    BOOST_TEST(tail.empty());

    BOOST_TEST(router.routes.size() == router.find("/foo/", tail));
    BOOST_TEST(tail.empty());

    BOOST_TEST(router["/bar/"] == router.find("/bar/", tail));
    BOOST_TEST(tail.empty());

    BOOST_TEST(router["/bar/"] == router.find("/bar/x", tail));
    BOOST_TEST("x" == tail);

    BOOST_TEST(router["/bar/"] == router.find("/bar/a+b", tail));
    BOOST_TEST("a b" == tail);

    BOOST_TEST(router["/bar/"] == router.find("/bar/a%20b", tail));
    BOOST_TEST("a b" == tail);

    BOOST_TEST(router.routes.size() == router.find("/baz", tail));
    BOOST_TEST(!tail.empty());
}

BOOST_AUTO_TEST_SUITE_END()
