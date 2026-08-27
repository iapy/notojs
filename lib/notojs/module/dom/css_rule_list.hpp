#pragma once
#include <notojs/module/dom/css_rule.hpp>

namespace notojs::dom {

struct CSSRuleList : HTMLElement
{
    BOOST_FORCEINLINE CSSRuleList(HTMLElement const &el)
    : HTMLElement{el.doc, el} {}

    void free();
    std::int64_t length(CSSStyleSheet &) const;
    std::optional<CSSRule> item(CSSStyleSheet &, std::int64_t) const;
};

} // namespace notojs:dom
