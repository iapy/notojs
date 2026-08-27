#pragma once


#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace notojs::parser {

struct Multipart
{
    struct Part
    {
        std::string name;
        std::optional<std::string> filename;

        std::string_view type;
        std::string_view body;
    };

    std::vector<Part> parts;
    std::string error;

    explicit operator bool() const noexcept;

    static bool is(std::string_view content_type);
    static Multipart parse(std::string_view content_type, std::string_view body);
};

} // namespace notojs::parser
