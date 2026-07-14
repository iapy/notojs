#pragma once
#include <string_view>

namespace notojs::detail {

constexpr std::string_view EXEC_TIME{"X-NotoJS-Exec-Time"};
constexpr std::string_view EXEC_PROC{"X-NotoJS-Worker-Id"};
constexpr std::string_view CACHE_USE{"X-NotoJS-Cache-Use"};

} // namespace notojs::detail
