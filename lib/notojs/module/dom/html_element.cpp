#include <notojs/module/dom/html_element.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <notojs/notojs.hpp>

namespace notojs::dom {
namespace {

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

lxb_dom_node_t *HTMLElement::root() const
{
    auto *root = static_cast<lxb_dom_node_t *>(*this);
    while(root->parent != nullptr)
    {
        if(LXB_DOM_NODE_TYPE_ELEMENT != root->parent->type) break;
        root = root->parent;
    }
    return root;
}

lxb_dom_node_t *HTMLElement::closest(lxb_tag_id_t tag) const
{
    lxb_dom_node_t *result{nullptr};
    for(lxb_dom_node_t *n = *this; n && !result; n = n->parent)
        if(tag == lxb_dom_node_tag_id(n)) result = n;
    return result;
}

lxb_dom_node_t *HTMLElement::directChild(lxb_tag_id_t tag) const
{
    for(lxb_dom_node_t *node = static_cast<lxb_dom_node_t *>(*this)->first_child; node; node = node->next)
        if(tag == lxb_dom_node_tag_id(node)) return node;
    return nullptr;
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
        if(!Attr::eq_ns(*this, attr->node.ns, name.ns)) continue;
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
        if(!Attr::eq_ns(*this, attr->node.ns, name.ns)) continue;
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
        dynamic_cast<dom::HTMLBackend *>(doc.get())->doc.get(), *this,
        reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
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
    lxb_dom_node_t *node = *this;
    lxb_dom_node_t *parent = node->parent;
    auto *backend = dynamic_cast<dom::HTMLBackend *>(doc.get());
    lxb_dom_element_t *context = LXB_DOM_NODE_TYPE_ELEMENT == parent->type
        ? lxb_dom_interface_element(parent)
        : lxb_dom_interface_element(backend->doc->body);

    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        backend->doc.get(), context,
        reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        while(fragment->first_child)
            lxb_dom_node_insert_before_spec(parent, fragment->first_child, node);
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
        if(!Attr::eq_ns(*this, attr->node.ns, name.ns)) continue;
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
    if(force)
    {
        if(!hasAttribute(a)) setAttribute(a, "");
        return true;
    }
    return removeAttribute(a), false;
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

    auto *attr = lxb_dom_element_set_attribute(*this,
        reinterpret_cast<lxb_char_t const *>(nstr), nlen,
        reinterpret_cast<lxb_char_t const *>(v.data()), v.size());
    lxb_dom_attr_set_name(attr, reinterpret_cast<lxb_char_t const *>(nstr), nlen, false);
    attr->node.ns = a.ns;
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
