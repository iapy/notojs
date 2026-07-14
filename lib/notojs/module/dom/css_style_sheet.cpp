#include <notojs/module/dom/css_style_sheet.hpp>
#include <notojs/module/dom/lexbor.hpp>

namespace notojs::dom {

CSSStyleSheet::CSSStyleSheet(HTMLElement const &el)
: HTMLElement{el.doc, el}
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(this->doc.get());
    doc->csss.reg(static_cast<lxb_dom_node_t *>(*this));
}

void CSSStyleSheet::deleteRule(std::int64_t index)
{
    update();
    std::string data;
    for(auto *list = sst->root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
        {
            if(index--) lxb_css_rule_serialize(rule, [](lxb_char_t const *data, size_t len, void *ctx) -> lxb_status_t {
                static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
                return LXB_STATUS_OK;
            }, &data);
        }
    }
    if(data != this->data) doc->textContent(*this, data);
}

std::int64_t CSSStyleSheet::insertRule(std::int64_t index, std::string_view const &d)
{
    update();
    std::string data;
    std::int64_t cnt{0};
    for(auto *list = sst->root; list; list = list->next)
    {
        if(LXB_CSS_RULE_LIST != list->type) continue;
        for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next, ++cnt)
        {
            if(!index--) data.append(d.data(), d.size());
            lxb_css_rule_serialize(rule, [](lxb_char_t const *data, size_t len, void *ctx) -> lxb_status_t {
                static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
                return LXB_STATUS_OK;
            }, &data);
        }
    }
    if(index >= 0)
    {
        data.append(d.data(), d.size());
        index = 0;
    }
    doc->textContent(*this, data);
    return cnt + index;
}

bool CSSStyleSheet::update()
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(this->doc.get());
    if(doc->csss.upd(*this, gen))
    {
        data.emplace(lexbor::get_text(*this));

        auto *css = lxb_dom_interface_document(doc->doc.get())->css;
        parser.reset(lxb_css_parser_create());

        if(LXB_STATUS_OK != lxb_css_parser_init(parser.get(), NULL))
            return false;

        sst.reset(lxb_css_stylesheet_create(nullptr));
        if(LXB_STATUS_OK != lxb_css_stylesheet_parse(sst.get(), parser.get(), (lxb_char_t const *)data->c_str(), data->size()))
            return false;
    }
    return true;
}

void CSSStyleSheet::free()
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(this->doc.get());
    doc->csss.erase(static_cast<lxb_dom_node_t *>(*this));
    doc->sheets.erase(*this);
}

} // namespace notojs::dom
