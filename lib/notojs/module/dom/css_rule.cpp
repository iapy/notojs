#include <notojs/module/dom/css_rule.hpp>

namespace notojs::dom {
namespace {

lxb_status_t serialize(lxb_char_t const *data, size_t len, void *ctx)
{
    static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
    return LXB_STATUS_OK;
}

bool equals(lexbor_str_t const &name, std::string_view expected)
{
    if(name.length != expected.size()) return false;
    for(std::size_t i = 0; i < name.length; ++i)
    {
        auto c = static_cast<char>(name.data[i]);
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if(c != expected[i]) return false;
    }
    return true;
}

std::uint16_t cssom_type(lxb_css_rule_t const *rule)
{
    if(LXB_CSS_RULE_STYLE == rule->type) return 1;
    if(LXB_CSS_RULE_AT_RULE != rule->type) return 0;

    auto const *at = lxb_css_rule_at(rule);
    auto type = at->type;
    if(LXB_CSS_AT_RULE__UNDEF == type && at->u.undef)
        type = at->u.undef->type;

    switch(type)
    {
    case LXB_CSS_AT_RULE_FONT_FACE: return 5;
    case LXB_CSS_AT_RULE_MEDIA: return 4;
    case LXB_CSS_AT_RULE_NAMESPACE: return 10;
    default: break;
    }

    if(LXB_CSS_AT_RULE__CUSTOM != at->type || !at->u.custom) return 0;
    auto const &name = at->u.custom->name;

    if(equals(name, "import")) return 3;
    if(equals(name, "page")) return 6;
    if(equals(name, "keyframes") || equals(name, "-webkit-keyframes")) return 7;
    if(equals(name, "counter-style")) return 11;
    if(equals(name, "supports")) return 12;
    if(equals(name, "font-feature-values")) return 14;

    if(equals(name, "top-left-corner") || equals(name, "top-left")
        || equals(name, "top-center") || equals(name, "top-right")
        || equals(name, "top-right-corner") || equals(name, "bottom-left-corner")
        || equals(name, "bottom-left") || equals(name, "bottom-center")
        || equals(name, "bottom-right") || equals(name, "bottom-right-corner")
        || equals(name, "left-top") || equals(name, "left-middle")
        || equals(name, "left-bottom") || equals(name, "right-top")
        || equals(name, "right-middle") || equals(name, "right-bottom"))
        return 9;

    return 0;
}

} // namespace

CSSRule::CSSRule(HTMLElement const &element, std::uint64_t id,
                 std::string text, lxb_css_rule_t const &rule)
: HTMLElement{element.doc, element}, id{id}, snapshot{std::move(text)}, snapshot_type{cssom_type(&rule)}
{}

bool CSSRule::attached() const
{
    auto *backend = dynamic_cast<HTMLBackend *>(doc.get());
    auto it = backend->states.find(static_cast<lxb_html_element_t *>(*this));
    return it != std::end(backend->states)
        && it->second.update(*this)
        && it->second.ruleIndex(id).has_value();
}

std::string CSSRule::cssText() const
{
    auto *backend = dynamic_cast<HTMLBackend *>(doc.get());
    auto it = backend->states.find(static_cast<lxb_html_element_t *>(*this));
    if(it != std::end(backend->states))
    {
        if(it->second.update(*this))
            if(auto index = it->second.ruleIndex(id))
                if(auto *rule = it->second.rule(*index))
                {
                    std::string result;
                    if(LXB_STATUS_OK == lxb_css_rule_serialize(rule, serialize, &result))
                        return result;
                }
    }
    return snapshot;
}

std::uint16_t CSSRule::type() const
{
    auto *backend = dynamic_cast<HTMLBackend *>(doc.get());
    auto it = backend->states.find(static_cast<lxb_html_element_t *>(*this));
    if(it != std::end(backend->states))
    {
        if(it->second.update(*this))
            if(auto index = it->second.ruleIndex(id))
                if(auto *rule = it->second.rule(*index))
                    return cssom_type(rule);
    }
    return snapshot_type;
}

void CSSRule::free()
{
    dynamic_cast<HTMLBackend *>(doc.get())->rules.erase(id);
}

} // namespace notojs::dom
