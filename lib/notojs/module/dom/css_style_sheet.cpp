#include <notojs/module/dom/css_style_sheet.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <algorithm>

namespace notojs::dom {
namespace {

BOOST_FORCEINLINE bool exposed_rule(lxb_css_rule_t const *rule)
{
    return LXB_CSS_RULE_BAD_STYLE != rule->type;
}

lxb_status_t serialize(lxb_char_t const *data, size_t len, void *ctx)
{
    static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
    return LXB_STATUS_OK;
}

std::uint32_t rule_count(lxb_css_rule_t *root)
{
    std::uint32_t count{0};
    for(auto *list = root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
            if(exposed_rule(rule)) ++count;
    }
    return count;
}

std::optional<std::string> parse_rule(std::string_view const &text)
{
    using Deleter = CSSStyleSheetState::Deleter;
    std::unique_ptr<lxb_css_parser_t, Deleter> parser{lxb_css_parser_create()};
    if(!parser || LXB_STATUS_OK != lxb_css_parser_init(parser.get(), NULL))
        return std::nullopt;

    std::unique_ptr<lxb_css_stylesheet_t, Deleter> sheet{lxb_css_stylesheet_create(nullptr)};
    if(!sheet || LXB_STATUS_OK != lxb_css_stylesheet_parse(sheet.get(), parser.get(),
        reinterpret_cast<lxb_char_t const *>(text.data()), text.size()))
        return std::nullopt;

    lxb_css_rule_t *parsed{nullptr};
    for(auto *list = sheet->root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
        {
            if(parsed || LXB_CSS_RULE_BAD_STYLE == rule->type)
                return std::nullopt;
            parsed = rule;
        }
    }
    if(!parsed) return std::nullopt;

    std::string result;
    if(LXB_STATUS_OK != lxb_css_rule_serialize(parsed, serialize, &result))
        return std::nullopt;
    return result;
}

} // namespace

lxb_css_rule_t *CSSStyleSheetState::rule(std::size_t index) const
{
    for(auto *list = root(); list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *item = lxb_css_rule_list(list)->first; item; item = item->next)
            if(exposed_rule(item) && !index--) return item;
    }
    return nullptr;
}

std::optional<std::uint64_t> CSSStyleSheetState::ruleId(std::size_t index) const
{
    if(index >= rule_ids.size()) return std::nullopt;
    return rule_ids[index];
}

std::optional<std::size_t> CSSStyleSheetState::ruleIndex(std::uint64_t id) const
{
    auto it = std::find(std::begin(rule_ids), std::end(rule_ids), id);
    if(it == std::end(rule_ids)) return std::nullopt;
    return std::distance(std::begin(rule_ids), it);
}

CSSStyleSheetState *CSSStyleSheet::getState(HTMLElement const &el)
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    auto *node = static_cast<lxb_html_element_t *>(el);
    auto [it, inserted] = doc->states.try_emplace(node);
    if(inserted) doc->csss.reg(static_cast<lxb_dom_node_t *>(el));
    return &it->second;
}

CSSStyleSheet::CSSStyleSheet(HTMLElement const &el)
: HTMLElement{el.doc, el}, state{getState(el)}
{}

CSSStyleSheet::MutationError CSSStyleSheet::deleteRule(std::int64_t index)
{
    if(!update()) return MutationError::syntax;
    if(index < 0 || index >= rule_count(state->root())) return MutationError::index_size;

    std::string data;
    std::uint32_t position{0};
    for(auto *list = state->root(); list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
        {
            if(!exposed_rule(rule)) continue;
            if(position++ != index && LXB_STATUS_OK != lxb_css_rule_serialize(rule, serialize, &data))
                return MutationError::syntax;
        }
    }

    auto ids = state->rule_ids;
    ids.erase(std::begin(ids) + index);
    state->pending_rule_ids.emplace(std::move(ids));
    doc->textContent(*this, data);
    return MutationError::none;
}

std::pair<CSSStyleSheet::MutationError, std::int64_t> CSSStyleSheet::insertRule(
    std::int64_t index, std::string_view const &text)
{
    if(!update()) return {MutationError::syntax, 0};

    auto const count = rule_count(state->root());
    if(index < 0 || index > count) return {MutationError::index_size, 0};

    auto parsed = parse_rule(text);
    if(!parsed) return {MutationError::syntax, 0};

    std::string data;
    std::uint32_t position{0};
    for(auto *list = state->root(); list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
        {
            if(!exposed_rule(rule)) continue;
            if(position++ == index) data.append(*parsed);
            if(LXB_STATUS_OK != lxb_css_rule_serialize(rule, serialize, &data))
                return {MutationError::syntax, 0};
        }
    }
    if(index == count) data.append(*parsed);

    auto *backend = dynamic_cast<HTMLBackend *>(doc.get());
    auto ids = state->rule_ids;
    ids.insert(std::begin(ids) + index, backend->next_rule_id++);
    state->pending_rule_ids.emplace(std::move(ids));
    doc->textContent(*this, data);
    return {MutationError::none, index};
}

void CSSStyleSheet::replace(std::string_view const &text)
{
    state->pending_rule_ids.reset();
    doc->textContent(*this, text);
}

bool CSSStyleSheetState::update(HTMLElement const &el)
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    if(doc->csss.upd(static_cast<lxb_dom_node_t *>(el), gen))
    {
        auto pending = std::move(pending_rule_ids);
        pending_rule_ids.reset();
        data.emplace(lexbor::get_text(el));
        valid = false;
        rule_ids.clear();
        sst.reset();
        parser.reset(lxb_css_parser_create());

        if(!parser || LXB_STATUS_OK != lxb_css_parser_init(parser.get(), NULL))
            return false;

        sst.reset(lxb_css_stylesheet_create(nullptr));
        if(!sst || LXB_STATUS_OK != lxb_css_stylesheet_parse(sst.get(), parser.get(),
            reinterpret_cast<lxb_char_t const *>(data->data()), data->size()))
        {
            sst.reset();
            return false;
        }

        auto const count = rule_count(sst->root);
        if(pending && pending->size() == count)
            rule_ids = std::move(*pending);
        else
            for(std::uint32_t i = 0; i < count; ++i)
                rule_ids.push_back(doc->next_rule_id++);
        valid = true;
    }
    return valid;
}

bool CSSStyleSheet::update()
{
    return state->update(*this);
}

void CSSStyleSheet::free()
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(this->doc.get());
    doc->sheets.erase(*this);
}

} // namespace notojs::dom
