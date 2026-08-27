#pragma once
#include <notojs/module/dom/css_style_sheet.hpp>

namespace notojs::dom {

struct CSSRule : HTMLElement
{
    CSSRule(HTMLElement const &, std::uint64_t, std::string, lxb_css_rule_t const &);

    std::string cssText() const;
    std::uint16_t type() const;
    bool attached() const;
    void free();

    std::uint64_t id;

private:
    std::string snapshot;
    std::uint16_t snapshot_type;
};

} // namespace notojs::dom
