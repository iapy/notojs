#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct CSSStyleSheet : HTMLElement
{
    CSSStyleSheet(HTMLElement const &el);
    void deleteRule(std::int64_t);
    std::int64_t insertRule(std::int64_t, std::string_view const &);
    void free();

private:
    bool update();

private:
    std::optional<std::string> data;
    std::unique_ptr<lxb_css_parser_t, HTMLBackend::Deleter> parser;
    std::unique_ptr<lxb_css_stylesheet_t, HTMLBackend::Deleter> sst;
    mutable std::uint32_t gen{std::numeric_limits<std::uint32_t>::max()};
    friend class CSSRuleList;
};

} // namespace notojs:dom
