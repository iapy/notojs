#include <notojs/module/dom/lexbor.hpp>
#include <notojs/notojs.hpp>

namespace notojs::dom::lexbor {
namespace {

void append(lxb_dom_node_t *node, std::string &text)
{
    lxb_dom_character_data_t *cd;
    switch(node->type)
    {
    case LXB_DOM_NODE_TYPE_COMMENT:
    case LXB_DOM_NODE_TYPE_TEXT:
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        cd = lxb_dom_interface_character_data(node);
        text.append((char const *)cd->data.data, cd->data.length);
        break;
    default:
        for(auto *n = lxb_dom_node_first_child(node); n; n = lxb_dom_node_next(n))
            append(n, text);
        break;
    }
}

} // namespace

lxb_dom_node_t *head(lxb_dom_node_t *doc)
{
    auto *node = lxb_dom_node_first_child(doc);
    for(;node; node = lxb_dom_node_next(node))
    {
        if(LXB_DOM_NODE_TYPE_ELEMENT != node->type || LXB_TAG_HTML != lxb_dom_node_tag_id(node))
            continue;
        break;
    }

    if(!node) return NULL;
    node = lxb_dom_node_first_child(node);

    for(;node; node = lxb_dom_node_next(node))
    {
        if(LXB_DOM_NODE_TYPE_ELEMENT != node->type || LXB_TAG_HEAD != lxb_dom_node_tag_id(node))
            continue;
        return node;
    }
    return NULL;
}

lxb_dom_node_t *title(lxb_dom_node_t *head, bool make)
{
    for(auto *node = lxb_dom_node_first_child(head);
        node; node = lxb_dom_node_next(node))
    {
        if(LXB_DOM_NODE_TYPE_ELEMENT != node->type)
            continue;
        if(LXB_TAG_TITLE == lxb_dom_node_tag_id(node))
            return node;
    }
    if(make)
    {
        auto *e = lxb_dom_document_create_element(head->owner_document,
            (lxb_char_t const *)"title", 5, NULL);

        lxb_dom_node_append_child(head, lxb_dom_interface_node(e));
        return lxb_dom_interface_node(e);
    }
    return NULL;
}

std::string_view get_name(uintptr_t name)
{
    size_t len = 0;
    lxb_char_t const *str = lxb_tag_name_by_id((lxb_tag_id_t)name, &len);

    if(str != nullptr) return std::string_view{(char const *)str, len};
    return std::string_view{};
}

std::string_view get_name(lxb_dom_element_t *node)
{
    std::size_t len;
    char const *str = (char const *)lxb_dom_element_tag_name(node, &len);
    return std::string_view{str, len};
}

std::string_view get_name(lxb_dom_document_type_t *node)
{
    std::size_t len;
    char const *str = (char const *)lxb_dom_document_type_name(node, &len);
    return std::string_view{str, len};
}

std::string get_text(lxb_dom_node_t *node)
{
    std::string text;
    return append(node, text), text;
}

void del_text(lxb_dom_node_t *node)
{
    lxb_dom_node_t *dn;
    while((dn = lxb_dom_node_first_child(node)))
        lxb_dom_node_remove(dn);
}

void set_text(lxb_dom_node_t *node, std::string_view const &text)
{
    del_text(node);
    auto *n = lxb_dom_interface_node(node);
    lxb_dom_text_t *t = lxb_dom_document_create_text_node(n->owner_document,
        (lxb_char_t const *)text.data(), text.size());
    lxb_dom_node_append_child(n, lxb_dom_interface_node(t));
}

namespace {

lxb_dom_element_t *fragmentContext(lxb_dom_node_t *node)
{
    return LXB_DOM_NODE_TYPE_ELEMENT == node->type
        ? lxb_dom_interface_element(node)
        : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body);
}

bool svgContainer(lxb_dom_node_t *node, lxb_dom_node_t *fragment)
{
    return LXB_DOM_NODE_TYPE_ELEMENT == node->type && !node->first_child
        && fragment->first_child
        && fragment->first_child == fragment->last_child
        && LXB_DOM_NODE_TYPE_ELEMENT == fragment->first_child->type
        && LXB_TAG_SVG == lxb_dom_node_tag_id(fragment->first_child)
        && std::invoke([](std::string_view &&name) {
            return "symbol" == name || "pattern" == name || "marker" == name;
        }, get_name(lxb_dom_interface_element(node)));
}

lxb_dom_node_t *appendFragment(lxb_dom_node_t *node, void *prepared)
{
    auto *fragment = static_cast<lxb_dom_node_t *>(prepared);
    lxb_dom_node_t *child = nullptr;
    while(fragment && fragment->first_child)
        lxb_dom_node_append_child(node, child = fragment->first_child);
    return child;
}

lxb_dom_node_t *insertFragment(lxb_dom_node_t *node, void *prepared, lxb_dom_node_t *reference)
{
    auto *fragment = static_cast<lxb_dom_node_t *>(prepared);
    lxb_dom_node_t *child = nullptr;
    while(fragment && fragment->first_child)
        lxb_dom_node_insert_before_spec(node, child = fragment->first_child, reference);
    return child;
}

} // namespace

template<> bool prepare<HTML>(lxb_dom_node_t *node, HTML &html, std::optional<std::string> &error)
{
    if(html.fragment) return true;
    if(auto data = html.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(auto *parsed = lxb_html_document_parse_fragment(
            lxb_html_interface_document(node->owner_document), fragmentContext(node),
            reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            auto *fragment = lxb_dom_document_create_document_fragment(node->owner_document);
            auto *fragmentNode = lxb_dom_interface_node(fragment);
            while(parsed->first_child)
                lxb_dom_node_append_child(fragmentNode, parsed->first_child);
            html.fragment = fragmentNode;
        }
    }
    if(auto *fragment = static_cast<lxb_dom_node_t *>(html.fragment);
        fragment && fragment->first_child) return true;

    html.fragment = nullptr;
    error.emplace("Invalid HTML fragment");
    return false;
}

template<> bool prepare<Image>(lxb_dom_node_t *node, Image &image, std::optional<std::string> &)
{
    if(image.fragment) return true;

    auto *fragment = lxb_dom_document_create_document_fragment(node->owner_document);
    auto *img = lxb_dom_document_create_element(node->owner_document,
        reinterpret_cast<lxb_char_t const *>("img"), 3, nullptr);

    if(auto data = image.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        lxb_dom_element_set_attribute(img,
            reinterpret_cast<lxb_char_t const *>("src"), 3,
            reinterpret_cast<lxb_char_t const *>(src.data()), src.size()
        )->node.ns = LXB_NS_HTML;
    }

    lxb_dom_node_append_child(lxb_dom_interface_node(fragment), lxb_dom_interface_node(img));
    image.fragment = lxb_dom_interface_node(fragment);
    return true;
}

template<> bool prepare<SVG>(lxb_dom_node_t *node, SVG &svg, std::optional<std::string> &error)
{
    if(svg.fragment) return true;
    if(auto data = svg.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(auto *parsed = lxb_html_document_parse_fragment(
            lxb_html_interface_document(node->owner_document), fragmentContext(node),
            reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            auto *fragment = lxb_dom_document_create_document_fragment(node->owner_document);
            auto *fragmentNode = lxb_dom_interface_node(fragment);
            while(parsed->first_child)
                lxb_dom_node_append_child(fragmentNode, parsed->first_child);
            svg.fragment = fragmentNode;
        }
    }
    if(auto *fragment = static_cast<lxb_dom_node_t *>(svg.fragment);
        fragment && fragment->first_child
            && (!svgContainer(node, fragment) || fragment->first_child->first_child)) return true;

    svg.fragment = nullptr;
    error.emplace("Invalid SVG fragment");
    return false;
}

template<> lxb_dom_node_t *appendPrepared<HTML>(lxb_dom_node_t *node, HTML &html)
{
    return appendFragment(node, html.fragment);
}

template<> lxb_dom_node_t *appendPrepared<Image>(lxb_dom_node_t *node, Image &image)
{
    return appendFragment(node, image.fragment);
}

template<> lxb_dom_node_t *appendPrepared<SVG>(lxb_dom_node_t *node, SVG &svg)
{
    auto *fragment = static_cast<lxb_dom_node_t *>(svg.fragment);
    if(!fragment || !svgContainer(node, fragment)) return appendFragment(node, fragment);

    lxb_dom_node_t *child = nullptr;
    while(fragment->first_child->first_child)
        lxb_dom_node_append_child(node, child = fragment->first_child->first_child);

    std::size_t length;
    auto *source = lxb_dom_interface_element(fragment->first_child);
    for(auto *attr = lxb_dom_element_first_attribute(source); attr;
        attr = lxb_dom_element_next_attribute(attr))
    {
        auto const *name = lxb_dom_attr_qualified_name(attr, &length);
        lxb_dom_element_set_attribute(lxb_dom_interface_element(node), name, length,
            attr->value->data, attr->value->length)->node.ns = attr->node.ns;
    }
    return child;
}

template<> lxb_dom_node_t *insertPrepared<HTML>(lxb_dom_node_t *node, HTML &html, lxb_dom_node_t *reference)
{
    return insertFragment(node, html.fragment, reference);
}

template<> lxb_dom_node_t *insertPrepared<Image>(lxb_dom_node_t *node, Image &image, lxb_dom_node_t *reference)
{
    return insertFragment(node, image.fragment, reference);
}

template<> lxb_dom_node_t *insertPrepared<SVG>(lxb_dom_node_t *node, SVG &svg, lxb_dom_node_t *reference)
{
    return insertFragment(node, svg.fragment, reference);
}

template<> lxb_dom_node_t *appendChild<HTML>(lxb_dom_node_t *node, HTML const &h, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    if(auto data = h.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            lxb_html_interface_document(node->owner_document),
            LXB_DOM_NODE_TYPE_ELEMENT == node->type
                ? lxb_dom_interface_element(node)
                : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body),
            reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            while(fragment->first_child != nullptr)
                lxb_dom_node_append_child(node, child = fragment->first_child);
        }
    }
    if(!child) error.emplace("Invalid HTML fragment");
    return child;
}

template<> lxb_dom_node_t *appendChild<Image>(lxb_dom_node_t *node, Image const &i, std::optional<std::string> &)
{
    auto *img = lxb_dom_document_create_element(lxb_dom_interface_document(node->owner_document),
        (lxb_char_t const *)"img", 3, NULL);

    if(auto data = i.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        lxb_dom_element_set_attribute(img,
            reinterpret_cast<lxb_char_t const *>("src"), 3,
            reinterpret_cast<lxb_char_t const *>(src.data()), src.size()
        )->node.ns = LXB_NS_HTML;
    }

    lxb_dom_node_append_child(node, lxb_dom_interface_node(img));
    return lxb_dom_interface_node(img);
}

template<> lxb_dom_node_t *appendChild<SVG>(lxb_dom_node_t *node, SVG const &s, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    if(auto data = s.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            lxb_html_interface_document(node->owner_document),
            LXB_DOM_NODE_TYPE_ELEMENT == node->type
                ? lxb_dom_interface_element(node)
                : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body),
            reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            if(LXB_DOM_NODE_TYPE_ELEMENT == node->type && !node->first_child
                && fragment->first_child
                && fragment->first_child == fragment->last_child
                && LXB_DOM_NODE_TYPE_ELEMENT == fragment->first_child->type
                && LXB_TAG_SVG == lxb_dom_node_tag_id(fragment->first_child)
                && std::invoke([](std::string_view &&name) {
                    return "symbol" == name
                        || "pattern" == name
                        || "marker" == name;
                }, get_name(lxb_dom_interface_element(node))))
            {
                while(fragment->first_child->first_child)
                    lxb_dom_node_append_child(node, child = fragment->first_child->first_child);

                std::size_t nlen;
                lxb_dom_element_t *svg = lxb_dom_interface_element(fragment->first_child);
                for(auto *attr = lxb_dom_element_first_attribute(svg); attr; attr = lxb_dom_element_next_attribute(attr))
                {
                    auto const *name = lxb_dom_attr_qualified_name(attr, &nlen);
                    lxb_dom_element_set_attribute(lxb_dom_interface_element(node), name, nlen, attr->value->data, attr->value->length)->node.ns = attr->node.ns;
                }
            }
            else while(fragment->first_child != nullptr)
                lxb_dom_node_append_child(node, child = fragment->first_child);
        }
    }
    if(!child) error.emplace("Invalid SVG fragment");
    return child;
}

template<> lxb_dom_node_t *appendChild<bridge::String>(lxb_dom_node_t *node, bridge::String const &s, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    auto const &src = static_cast<std::string_view const &>(s);

    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        lxb_html_interface_document(node->owner_document),
        LXB_DOM_NODE_TYPE_ELEMENT == node->type
            ? lxb_dom_interface_element(node)
            : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body),
        reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        while(fragment->first_child != nullptr)
            lxb_dom_node_append_child(node, child = fragment->first_child);
    }
    if(!child) error.emplace("Invalid HTML fragment");
    return child;
}

template<typename T>
lxb_dom_node_t *insertBefore(lxb_dom_node_t *node, T const &h, lxb_dom_node_t *ref, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;

    if(auto data = h.template get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
            lxb_html_interface_document(node->owner_document),
            LXB_DOM_NODE_TYPE_ELEMENT == node->type
                ? lxb_dom_interface_element(node)
                : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body),
            reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
        {
            while(fragment->first_child != nullptr)
                lxb_dom_node_insert_before_spec(node, child = fragment->first_child, ref);
        }
    }
    if(!child)
    {
        if constexpr (std::is_same_v<T, SVG>) error.emplace("Invalid SVG fragment");
        else error.emplace("Invalid HTML fragment");
    }
    return child;
}

template lxb_dom_node_t *insertBefore<SVG>(lxb_dom_node_t *node, SVG const &, lxb_dom_node_t *, std::optional<std::string> &);
template lxb_dom_node_t *insertBefore<HTML>(lxb_dom_node_t *node, HTML const &, lxb_dom_node_t *, std::optional<std::string> &);

template<> lxb_dom_node_t *insertBefore<Image>(lxb_dom_node_t *node, Image const &i, lxb_dom_node_t *ref, std::optional<std::string> &error)
{
    auto *img = lxb_dom_document_create_element(lxb_dom_interface_document(node->owner_document),
        (lxb_char_t const *)"img", 3, NULL);

    if(auto data = i.get<bridge::String>("data"); data)
    {
        auto const &src = static_cast<std::string_view const &>(*data);
        lxb_dom_element_set_attribute(img,
            reinterpret_cast<lxb_char_t const *>("src"), 3,
            reinterpret_cast<lxb_char_t const *>(src.data()), src.size()
        )->node.ns = LXB_NS_HTML;
    }

    lxb_dom_node_insert_before_spec(node, lxb_dom_interface_node(img), ref);
    return lxb_dom_interface_node(img);
}

template<> lxb_dom_node_t *insertBefore<bridge::String>(lxb_dom_node_t *node, bridge::String const &h, lxb_dom_node_t *ref, std::optional<std::string> &error)
{
    lxb_dom_node_t *child = nullptr;
    auto const &src = static_cast<std::string_view const &>(h);

    if(lxb_dom_node_t *fragment = lxb_html_document_parse_fragment(
        lxb_html_interface_document(node->owner_document),
        LXB_DOM_NODE_TYPE_ELEMENT == node->type
            ? lxb_dom_interface_element(node)
            : lxb_dom_interface_element(lxb_html_interface_document(node->owner_document)->body),
        reinterpret_cast<const lxb_char_t *>(src.data()), src.size()))
    {
        while(fragment->first_child != nullptr)
            lxb_dom_node_insert_before_spec(node, child = fragment->first_child, ref);
    }

    if(!child) error.emplace("Invalid HTML fragment");
    return child;
}

} // namespace notojs::dom::lexbor
