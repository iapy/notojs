#pragma once
#include <boost/spirit/home/x3.hpp>
#include <notojs/parser/uri.hpp>

namespace notojs::parser {

namespace x3 = boost::spirit::x3;

template<typename Z>
struct Search : protected Z
{
    template<typename ...Args>
    Search(Args &&... args): Z{std::forward<Args>(args)...} {}

    template<typename V>
    static std::string decode(V const &val)
    {
        std::string res;
        res.reserve(val.size());

        char const *b = std::begin(val);
        return decodeURIComponent(b, b + val.size(), true);
    }

    struct Key
    {
        template<typename Ctx>
        void operator ()(Ctx &ctx)
        {
            auto val = x3::_attr(ctx);
            x3::_val(ctx).Z::key(decode(val));
        }
    };

    struct Val
    {
        template<typename Ctx>
        void operator ()(Ctx &ctx)
        {
            auto val = x3::_attr(ctx);
            x3::_val(ctx).Z::val(decode(val));
        }
    };

    Z &parse(std::string_view const &s);
    BOOST_FORCEINLINE Z const * operator -> () const { return this; }
};

template<typename Z> auto const T = *(x3::char_ - '&' - '=');
template<typename Z> auto const P = x3::raw[T<Z>][typename Search<Z>::Key()] >> -(x3::lit("=") >> x3::raw[T<Z>][typename Search<Z>::Val()]);
template<typename Z> auto const Q = -(P<Z> % '&');

template<typename Z>
auto const form = std::invoke([]{
    x3::rule<class fprm, Search<Z>> const style = "form";
    return style = Q<Z>;
});

template<typename Z>
BOOST_FORCEINLINE Z& Search<Z>::parse(std::string_view const &s)
{
    x3::parse(s.data(), s.data() + s.size(), form<Z>, *this);
    return *this;
}

} // notojs::parser
