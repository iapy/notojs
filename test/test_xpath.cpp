#include <boost/test/unit_test.hpp>
#include <notojs/module/dom/xpath.hpp>

bool css_to_xpath(std::string_view const &sv, std::string &xpath)
{
    return notojs::dom::css_to_xpath(std::begin(sv), std::end(sv), xpath);
}

BOOST_AUTO_TEST_SUITE(XPath)

BOOST_AUTO_TEST_CASE(Any)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("*", xpath));
    BOOST_TEST(".//*" == xpath);
}

BOOST_AUTO_TEST_CASE(Attribute)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("a[b]", xpath));
    BOOST_TEST(".//a[@b]" == xpath);

    BOOST_TEST(css_to_xpath("a[b='foo']", xpath));
    BOOST_TEST(".//a[@b='foo']" == xpath);
}

BOOST_AUTO_TEST_CASE(Pseudo)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("a:first-child", xpath));
    BOOST_TEST(".//a[not(preceding-sibling::*)]" == xpath);

    BOOST_TEST(css_to_xpath("a:last-child", xpath));
    BOOST_TEST(".//a[not(following-sibling::*)]" == xpath);

    BOOST_TEST(css_to_xpath("a:first-child:last-child", xpath));
    BOOST_TEST(".//a[not(preceding-sibling::*)][not(following-sibling::*)]" == xpath);
}

BOOST_AUTO_TEST_CASE(Relation)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("a b", xpath));
    BOOST_TEST(".//a//b" == xpath);

    BOOST_TEST(css_to_xpath("a > b", xpath));
    BOOST_TEST(".//a/b" == xpath);

    BOOST_TEST(css_to_xpath("a>b", xpath));
    BOOST_TEST(".//a/b" == xpath);

    BOOST_TEST(css_to_xpath("a>b c", xpath));
    BOOST_TEST(".//a/b//c" == xpath);

    BOOST_TEST(css_to_xpath("a>b> c", xpath));
    BOOST_TEST(".//a/b/c" == xpath);

    BOOST_TEST(css_to_xpath("a b> c", xpath));
    BOOST_TEST(".//a//b/c" == xpath);
}

BOOST_AUTO_TEST_CASE(QName)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("ns:a", xpath));
    BOOST_TEST(".//ns:a" == xpath);

    BOOST_TEST(css_to_xpath("ns:a:first-child", xpath));
    BOOST_TEST(".//ns:a[not(preceding-sibling::*)]" == xpath);

    BOOST_TEST(css_to_xpath("ns:a:last-child", xpath));
    BOOST_TEST(".//ns:a[not(following-sibling::*)]" == xpath);
}

BOOST_AUTO_TEST_CASE(Union)
{
    std::string xpath;
    BOOST_TEST(css_to_xpath("a:first-child, b:eq(1)", xpath));
    BOOST_TEST(".//a[not(preceding-sibling::*)] | .//b[2]" == xpath);
}

BOOST_AUTO_TEST_SUITE_END()
