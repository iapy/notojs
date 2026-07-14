#include <notojs/module/dom/css_rule_list.hpp>
#include <iostream>

namespace notojs::dom {

std::int64_t CSSRuleList::length(CSSStyleSheet &sheet) const
{
    sheet.update();
    std::int64_t result{0};
    for(auto *list = sheet.sst->root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
            ++result;
    }
    return result;
}

std::optional<std::pair<std::string, std::uint16_t>> CSSRuleList::item(CSSStyleSheet &sheet, std::int64_t index) const
{
    sheet.update();
    std::optional<std::pair<std::string, std::uint16_t>> result;
    for(auto *list = sheet.sst->root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;

        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
        if(!index--)
        {
            lxb_css_rule_serialize(rule, [](lxb_char_t const *data, size_t len, void *ctx) -> lxb_status_t {
                static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
                return LXB_STATUS_OK;
            }, &result.emplace().first);
            result->second = rule->type;
            break;
        }
    }
    return result;
}

} // namespace notojs::dom
