#include <notojs/module/dom/css_rule_list.hpp>

namespace notojs::dom {

std::int64_t CSSRuleList::length(CSSStyleSheet &sheet) const
{
    if(!sheet.update()) return 0;
    return sheet.state->ruleCount();
}

std::optional<CSSRule> CSSRuleList::item(CSSStyleSheet &sheet, std::int64_t index) const
{
    if(index < 0 || !sheet.update()) return std::nullopt;

    auto id = sheet.state->ruleId(index);
    auto *rule = sheet.state->rule(index);
    if(!id || !rule) return std::nullopt;

    std::string text;
    if(LXB_STATUS_OK != lxb_css_rule_serialize(rule,
        [](lxb_char_t const *data, size_t len, void *ctx) -> lxb_status_t {
            static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
            return LXB_STATUS_OK;
        }, &text))
        return std::nullopt;

    return CSSRule{sheet, *id, std::move(text), *rule};
}

void CSSRuleList::free()
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(this->doc.get());
    doc->cssrules.erase(*this);
}

} // namespace notojs::dom
