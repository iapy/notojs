#pragma once
#include <boost/property_tree/ptree.hpp>
#include <filesystem>

namespace notojs::detail {

struct Config : boost::property_tree::ptree
{
    std::filesystem::path const *source;
};

} // namespace notojs::detail
