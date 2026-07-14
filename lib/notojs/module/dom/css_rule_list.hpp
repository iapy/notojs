#pragma once
#include <notojs/module/dom/css_style_sheet.hpp>

namespace notojs::dom {

struct CSSRuleList : HTMLElement
{
    BOOST_FORCEINLINE CSSRuleList(HTMLElement const &el)
    : HTMLElement{el.doc, el} {}

    std::int64_t length(CSSStyleSheet &) const;
    std::optional<std::pair<std::string, std::uint16_t>> item(CSSStyleSheet &, std::int64_t) const;
};

} // namespace notojs:dom
