#pragma once
#include <boost/config.hpp>
#include <string>

namespace notojs::detail {

BOOST_FORCEINLINE std::string cell_id(std::size_t id)
{
    std::string str("cell-XXX");

    str[7] = '0' + (id % 10);
    str[6] = '0' + ((id /= 10) % 10);
    str[5] = '0' + ((id /= 10) % 10);

    return str;
}

} // namespace notojs::detail
