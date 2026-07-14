#pragma once
#include <boost/config.hpp>
#include <filesystem>
#include <optional>
#include <cstdint>
#include <utility>
#include <vector>

namespace notojs::detail {

using Bytecode = std::vector<std::uint8_t>;
using Textcode = std::vector<std::string>;
using Notebook = std::vector<Bytecode>;
using Version = std::filesystem::file_time_type;

struct Cached
{
    Version version{};
    std::optional<Notebook> code;
};

BOOST_FORCEINLINE bool disabled(std::string &code)
{
    return std::exchange(code.data()[code.size()], 0);
}

} // namespace notojs::detail
