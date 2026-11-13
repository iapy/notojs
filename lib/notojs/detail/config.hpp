#pragma once
#include <boost/property_tree/ptree.hpp>
#include <filesystem>

namespace notojs::detail {

struct Config : boost::property_tree::ptree
{
    Config(bool server = false, std::filesystem::path const *source = nullptr)
    : boost::property_tree::ptree{}, server{server}, source{source} {}

    bool const server;
    std::filesystem::path const * const source;
};

} // namespace notojs::detail
