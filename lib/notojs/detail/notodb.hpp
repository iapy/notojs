#pragma once
#include <boost/config.hpp>
#include <lmdbxx/lmdb++.h>

namespace notojs::detail {

template<bool Full>
BOOST_FORCEINLINE static bool is_object(lmdb::val const &f)
{
    if constexpr (Full)
    {
        if(!f.size()) return false;
    }
    return !f.data()[f.size() - 1];
}

} // namespace notojs::detail
