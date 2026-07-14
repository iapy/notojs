#include <notojs/module/dom/html_element.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <notojs/notojs.hpp>
#include <iostream>

namespace notojs::dom {
namespace {

lxb_status_t match(lxb_dom_node_t*, lxb_css_selector_specificity_t, void *ctx)
{
    *reinterpret_cast<bool*>(ctx) = true;
    return LXB_STATUS_STOP;
}

void append(lxb_dom_node_t *node, std::string &text)
{
    lxb_dom_character_data_t *cd;
    switch(node->type)
    {
    case LXB_DOM_NODE_TYPE_COMMENT:
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        return;
    case LXB_DOM_NODE_TYPE_TEXT:
        cd = lxb_dom_interface_character_data(node);
        for(std::size_t i = 0; i < cd->data.length; ++i)
            if(char const *ch = (char const *)cd->data.data + i; *ch != '\n') text.append(ch, 1);
            else if(text.empty() || !(' ' == text.back() || '\n' == text.back())) text.append(ch, 1);
        break;
    case LXB_DOM_NODE_TYPE_ELEMENT:
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_P:
        case LXB_TAG_UL:
        case LXB_TAG_OL:
        case LXB_TAG_LI:
            if(!text.empty() && '\n' != text.back()) text.append("\n", 1);
            break;
        case LXB_TAG_BR:
            text.append("\n", 1);
            break;
        }
    default:
        for(auto *n = lxb_dom_node_first_child(node); n; n = lxb_dom_node_next(n))
            append(n, text);
        break;
    }
}

std::string text_to_html(std::string_view const &text)
{
    bool b = false;
    std::string html;
    for(auto ch : text)
    {
        if(ch == '\n')
        {
            if(!std::exchange(b, true)) html.append("<br>");
        }
        else
        {
            b = false;
            html.append(&ch, 1);
        }
    }
    return html;
}

void insert_ns(std::string &data, std::string_view const &ns)
{
    if(auto p = data.find('>'); p != std::string::npos && data.find("xmlns=") > p)
    {
        if(p && data[p - 1] == '/') --p;
        data.insert(p, "\"");
        data.insert(p, ns);
        data.insert(p, " xmlns=\"");
    }
}

} // namespace

lxb_dom_node_t *HTMLElement::closest(std::string_view const &s, std::optional<std::string> &err) const
{
    bool matched{false};
    lxb_dom_node_t *result{nullptr};
    HTMLBackend *bke = dynamic_cast<dom::HTMLBackend *>(this->doc.get());

    auto *css = lxb_dom_interface_document(bke->doc.get())->css;
    if(auto *lst = lxb_css_selectors_parse(css->parser, (const lxb_char_t *) s.data(), s.size()))
    {
        for(lxb_dom_node_t *n = *this; n && !result; n = n->parent)
        {
            if(LXB_DOM_NODE_TYPE_ELEMENT == n->type && lxb_selectors_match_node(
                css->selectors, n, lst, match, &matched
            ); matched) { result = n; break; }
        }
        lxb_css_selector_list_destroy(lst);
    }
    return result;
}

bool HTMLElement::matches(std::string_view const &s, std::optional<std::string> &err) const
{
    bool matched{false};
    HTMLBackend *bke = dynamic_cast<dom::HTMLBackend *>(this->doc.get());

    auto *css = lxb_dom_interface_document(bke->doc.get())->css;
    if(auto *lst = lxb_css_selectors_parse(css->parser, (const lxb_char_t *) s.data(), s.size()))
    {
        lxb_selectors_match_node(css->selectors, *this, lst, match, &matched);
        lxb_css_selector_list_destroy(lst);
    }
    return matched;
}

std::vector<Attr::Name> HTMLElement::attrs() const
{
    std::size_t nlen;
    std::vector<Attr::Name> result;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
    {
        auto nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
        result.emplace_back(Attr::Name{std::string{nstr, nlen}, attr->node.ns});
    }
    return result;
}

std::uint64_t HTMLElement::attributesCount() const
{
    std::uint64_t count{0};
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
        ++count;
    return count;
}

void HTMLElement::clear()
{
    lxb_dom_node_t *node = *this;
    while(node->first_child)
        lxb_dom_node_remove(node->first_child);
}

std::optional<std::string_view> HTMLElement::lookupNS(uintptr_t ns) const
{
    return dynamic_cast<dom::HTMLBackend *>(doc.get())->lookupNS(ns);
}

std::optional<Attr::Name> HTMLElement::getAttributeName(std::int64_t i) const
{
    std::size_t nlen;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
        if(!i--)
        {
            auto nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
            return Attr::Name{std::string{nstr, nlen}, attr->node.ns};
        }
    return std::nullopt;
}

std::optional<std::string_view> HTMLElement::getAttribute(Attr::Name::View const &name) const
{
    std::size_t nlen;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
    {
        if(attr->node.ns != name.ns) continue;
        if(char const *nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
            std::string_view{nstr, nlen} == name.name)
        {
            char const *vstr = reinterpret_cast<char const *>(lxb_dom_attr_value(attr, &nlen));
            return std::string_view{vstr, nlen};
        }
    }
    return std::nullopt;
}

bool HTMLElement::hasAttribute(Attr::Name::View const &name) const
{
    std::size_t nlen = 0;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
    {
        if(attr->node.ns != name.ns) continue;
        if(char const *nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
            std::string_view{nstr, nlen} == name.name) return true;
    }
    return false;
}

std::string HTMLElement::innerHTML() const
{
    std::string data;
    for(auto *child = static_cast<lxb_dom_node_t *>(*this)->first_child; child; child = child->next)
        lxb_html_serialize_tree_cb(child, [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
            static_cast<std::string*>(ctx)->append(reinterpret_cast<char const *>(data), len);
            return LXB_STATUS_OK;
        }, &data);
    return data;
}

std::string HTMLElement::innerText() const
{
    std::string text;
    append(*this, text);
    return text;
}

template<> void HTMLElement::innerHTML<std::string_view>(std::string_view const &src)
{
    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
        *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        lxb_dom_node_t *node = *this;
        while(node->first_child)
            lxb_dom_node_remove(node->first_child);

        while(fragment->first_child)
            lxb_dom_node_append_child(node, fragment->first_child);
    }
}

template<> void HTMLElement::innerHTML<HTML>(HTML const &h)
{
    if(auto data = h.get<bridge::String>("data"); data)
        innerHTML(static_cast<std::string_view const &>(*data));
}

void HTMLElement::innerText(std::string_view const &text)
{
    auto const html = text_to_html(text);
    innerHTML(std::string_view{html.data(), html.size()});
}

template<> void HTMLElement::outerHTML<std::string_view>(std::string_view const &src)
{
    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
        *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        lxb_dom_node_t *node = *this;
        while(fragment->first_child)
            lxb_dom_node_insert_before_spec(node->parent, fragment->first_child, node);
        lxb_dom_node_remove(node);
    }
}

void HTMLElement::outerText(std::string_view const &text)
{
    auto const html = text_to_html(text);
    outerHTML(std::string_view{html.data(), html.size()});
}

template<> void HTMLElement::outerHTML<HTML>(HTML const &h)
{
    if(auto data = h.get<bridge::String>("data"); data)
        outerHTML(static_cast<std::string_view const &>(*data));
}

void HTMLElement::removeAttribute(Attr::Name::View const &name)
{
    std::size_t nlen = 0;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
    {
        if(attr->node.ns != name.ns) continue;
        if(char const *nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
            std::string_view{nstr, nlen} == name.name)
        {
            lxb_dom_element_attr_remove(lxb_dom_interface_element(node), attr);
            return;
        }
    }
}

bool HTMLElement::toggleAttribute(Attr::Name::View const &a)
{
    if(hasAttribute(a)) return removeAttribute(a), false;
    else return setAttribute(a, ""), true;
}

bool HTMLElement::toggleAttribute(Attr::Name::View const &a, bool force)
{
    if(force) return setAttribute(a, ""), true;
    else return removeAttribute(a), false;
}

void HTMLElement::setAttribute(Attr::Name::View const &a, std::string_view const &v)
{
    char const *nstr = a.name.data();
    std::size_t nlen = a.name.size();
    if(auto pos = a.name.find(':'); pos != std::string::npos)
    {
        nstr += (pos + 1);
        nlen -= (pos + 1);
    }

    lxb_dom_element_set_attribute(*this,
        reinterpret_cast<lxb_char_t const *>(nstr), nlen,
        reinterpret_cast<lxb_char_t const *>(v.data()), v.size())->node.ns = a.ns;
}

std::string HTMLElement::toString() const
{
    std::string data;
    lxb_html_serialize_tree_cb(*this, [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        static_cast<std::string*>(ctx)->append(reinterpret_cast<char const *>(data), len);
        return LXB_STATUS_OK;
    }, &data);
    if(LXB_NS_HTML != static_cast<lxb_dom_node_t *>(*this)->ns)
    {
        if(auto ns = lookupNS(static_cast<lxb_dom_node_t *>(*this)->ns); ns)
            insert_ns(data, *ns);
    }
    return data;
}

template<> lxb_dom_node_t *HTMLElement::appendChild<HTML>(HTML const &h, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    if(auto data = h.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
            *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            while(fragment->first_child != nullptr)
                lxb_dom_node_append_child(*this, child = fragment->first_child);
        }
    }
    if(!child) error.emplace("Invalid HTML fragment");
    return child;
}

template<> lxb_dom_node_t *HTMLElement::appendChild<Image>(Image const &i, std::optional<std::string> &)
{
    auto *img = lxb_dom_document_create_element(lxb_dom_interface_document(dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get()),
        (lxb_char_t const *)"img", 3, NULL);

    if(auto data = i.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        lxb_dom_element_set_attribute(img,
            reinterpret_cast<lxb_char_t const *>("src"), 3,
            reinterpret_cast<lxb_char_t const *>(src.data()), src.size()
        )->node.ns = LXB_NS_HTML;
    }

    lxb_dom_node_append_child(*this, lxb_dom_interface_node(img));
    return lxb_dom_interface_node(img);
}

template<> lxb_dom_node_t *HTMLElement::appendChild<SVG>(SVG const &s, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    if(auto data = s.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
            *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            if(!static_cast<lxb_dom_node_t *>(*this)->first_child
                && fragment->first_child
                && fragment->first_child == fragment->last_child
                && LXB_DOM_NODE_TYPE_ELEMENT == fragment->first_child->type
                && LXB_TAG_SVG == lxb_dom_node_tag_id(fragment->first_child)
                && std::invoke([](std::string_view &&name) {
                    return "symbol" == name
                        || "pattern" == name
                        || "marker" == name;
                }, lexbor::get_name(*this)))
            {
                while(fragment->first_child->first_child)
                    lxb_dom_node_append_child(*this, child = fragment->first_child->first_child);

                std::size_t nlen;
                lxb_dom_element_t *svg = lxb_dom_interface_element(fragment->first_child);
                for(auto *attr = lxb_dom_element_first_attribute(svg); attr; attr = lxb_dom_element_next_attribute(attr))
                {
                    auto const *name = lxb_dom_attr_qualified_name(attr, &nlen);
                    lxb_dom_element_set_attribute(*this, name, nlen, attr->value->data, attr->value->length)->node.ns = attr->node.ns;
                }
            }
            else while(fragment->first_child != nullptr)
                lxb_dom_node_append_child(*this, child = fragment->first_child);
        }
    }
    if(!child) error.emplace("Invalid SVG fragment");
    return child;
}

template<> lxb_dom_node_t *HTMLElement::appendChild<bridge::String>(bridge::String const &s, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    auto const &src = static_cast<std::string_view const &>(s);
    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
        *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        while(fragment->first_child != nullptr)
            lxb_dom_node_append_child(*this, child = fragment->first_child);
    }
    return child;
}

template<typename T>
lxb_dom_node_t *HTMLElement::insertBefore(T const &h, Node const &r, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    lxb_dom_node_t *rn = static_cast<lxb_dom_node_t *>(r.node);

    if(auto data = h.template get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
            *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            while(fragment->first_child != nullptr)
                lxb_dom_node_insert_before_spec(*this, child = fragment->first_child, rn);
        }
    }
    if(!child)
    {
        if constexpr (std::is_same_v<T, SVG>) error.emplace("Invalid SVG fragment");
        else error.emplace("Invalid HTML fragment");
    }
    return child;
}

template lxb_dom_node_t *HTMLElement::insertBefore<SVG>(SVG const &, Node const &, std::optional<std::string> &);
template lxb_dom_node_t *HTMLElement::insertBefore<HTML>(HTML const &, Node const &, std::optional<std::string> &);

template<> lxb_dom_node_t *HTMLElement::insertBefore<Image>(Image const &i, Node const &r, std::optional<std::string> &error)
{
    auto *img = lxb_dom_document_create_element(lxb_dom_interface_document(dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get()),
        (lxb_char_t const *)"img", 3, NULL);

    lxb_dom_node_t *rn = static_cast<lxb_dom_node_t *>(r.node);
    if(auto data = i.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        lxb_dom_element_set_attribute(img,
            reinterpret_cast<lxb_char_t const *>("src"), 3,
            reinterpret_cast<lxb_char_t const *>(src.data()), src.size()
        )->node.ns = LXB_NS_HTML;
    }

    lxb_dom_node_insert_before_spec(*this, lxb_dom_interface_node(img), rn);
    return lxb_dom_interface_node(img);
}

template<> lxb_dom_node_t *HTMLElement::insertBefore<bridge::String>(bridge::String const &h, Node const &r, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    lxb_dom_node_t *rn = static_cast<lxb_dom_node_t *>(r.node);

    auto const &src = static_cast<std::string_view const &>(h);
    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(),
        *this, reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        while(fragment->first_child != nullptr)
            lxb_dom_node_insert_before_spec(*this, child = fragment->first_child, rn);
    }

    error.emplace("Invalid HTML fragment");
    return child;
}

std::string HTMLElement::className(std::string_view const &str)
{
    std::string result{'.'};
    for(auto const ch: str)
    {
        if(std::isspace(ch))
        {
            if('.' != result.back()) result.append(".");
        }
        else switch(ch)
        {
        case ',':
        case '.':
        case '#':
        case '>':
            result.append("\\");
        default:
            result.append(&ch, 1);
        }
    }
    return result;
}

} // namespace notojs:dom
