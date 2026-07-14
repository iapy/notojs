#pragma once
#include <boost/spirit/home/x3.hpp>
#include <string>

namespace notojs::dom {
namespace detail {

namespace x3 = boost::spirit::x3;

struct Handler
{
    Handler(std::string &xpath): xpath{xpath.assign(".//")}
    {}

    struct Attr
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto val = x3::_attr(ctx);
            auto &xp = x3::_val(ctx).xpath;
            xp.append("[@");
            xp.append(std::begin(val), std::end(val));
            xp.append("]");
        }
    };

    struct Child
    {
        template<typename C>
        void operator () (C &ctx)
        {
            x3::_val(ctx).xpath.append("/");
        }
    };

    struct Comma
    {
        template<typename C>
        void operator () (C &ctx)
        {
            x3::_val(ctx).xpath.append(" | .//");
        }
    };

    struct Eq
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto val = x3::_attr(ctx);
            auto &xp = x3::_val(ctx).xpath;

            xp.append("[");
            xp.append(std::to_string(++val));
            xp.append("]");
        }
    };

    struct First
    {
        template<typename C>
        void operator () (C &ctx)
        {
            x3::_val(ctx).xpath.append("[not(preceding-sibling::*)]");
        }
    };

    struct Last
    {
        template<typename C>
        void operator () (C &ctx)
        {
            x3::_val(ctx).xpath.append("[not(following-sibling::*)]");
        }
    };

    struct Tag
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto val = x3::_attr(ctx);
            auto &xp = x3::_val(ctx).xpath;

            if('/' != xp.back()) xp.append("//");

            auto const v = std::string_view(std::begin(val), val.size());
            xp.append(v);
        }
    };

    std::string &xpath;
};

auto const V1 = x3::lit("=\"") >> *(x3::char_ - '"') >> '"';
auto const V2 = x3::lit("='") >> *(x3::char_ - '\'') >> '\'';

auto const P = x3::raw[x3::lit(":first-child")][Handler::First()] |
               x3::raw[x3::lit(":last-child")][Handler::Last()] |
               (x3::lit(":eq(") >> x3::uint_[Handler::Eq()] >> ')');

auto const R = x3::lit(":first-child") | x3::lit(":last-child") | (x3::lit(":eq(") >> x3::uint_ >> ')');

auto const E = x3::alpha >> *(x3::alnum | '-' | '_');
auto const Q = x3::raw[E >> -(!R >> ':' >> E)][Handler::Tag()];
auto const S = x3::raw['*'][Handler::Tag()];

auto const A = '[' >> x3::raw[E >> -(':' >> E) >> -(V1 | V2)][Handler::Attr()] >> ']';
auto const T = (((S >> *P) | (Q >> *P)) >> *A) | ((S | Q) >> *A >> *P);

auto const Single = T >> *((+x3::char_(' ') >> T) | (*x3::char_(' ') >> x3::char_('>')[Handler::Child()] >> *x3::char_(' ') >> T));
auto const Parser = Single >> *x3::char_(' ') >> *(x3::raw[','][Handler::Comma()] >> *x3::char_(' ') >> Single >> *x3::char_(' ')) >> x3::eoi;

auto const parser = std::invoke([]{
    x3::rule<class parser, Handler> const parser = "parser";
    return parser = Parser;
});

} // namespace detail

template<typename I>
BOOST_FORCEINLINE bool css_to_xpath(I b, I e, std::string &xpath)
{
    detail::Handler handler{xpath};
    return detail::x3::parse(b, e, detail::parser, handler);
}

} // namespace notojs::dom
