#pragma once
#include <boost/url/decode_view.hpp>
#include <string>

namespace notojs::parser {

template<typename I>
std::string decodeURIComponent(I b, I e, bool plus = false)
{
    boost::urls::decode_view dv{std::string_view{b, static_cast<size_t>(e - b)}, {plus}};
    return std::string(std::begin(dv), std::end(dv));
}

} // notojs::parser
