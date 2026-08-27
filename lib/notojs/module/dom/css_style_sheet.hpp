#pragma once
#include <notojs/module/dom/css_style_sheet_state.hpp>
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct CSSStyleSheet : HTMLElement
{
    enum class MutationError
    {
        none,
        index_size,
        syntax
    };

    CSSStyleSheet(HTMLElement const &el);
    MutationError deleteRule(std::int64_t);
    std::pair<MutationError, std::int64_t> insertRule(std::int64_t, std::string_view const &);
    void replace(std::string_view const &);
    void free();

    static CSSStyleSheetState *getState(HTMLElement const &);

private:
    bool update();

private:
    CSSStyleSheetState *state;
    friend class CSSRuleList;
};

} // namespace notojs:dom
