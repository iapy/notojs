#pragma once
#include <boost/config.hpp>
#include <filesystem>

namespace notojs::detail {

template<typename F>
BOOST_FORCEINLINE void iterate(std::filesystem::path const &p, F &&functor)
{
    std::error_code ec;
    for(std::filesystem::directory_iterator it{p,
        std::filesystem::directory_options::skip_permission_denied, ec}, end;
        it != end; it.increment(ec))
    {
        if(ec)
        {
            ec.clear();
            continue;
        }
        functor(it->path());
    }
}

} // namespace notojs::detail
