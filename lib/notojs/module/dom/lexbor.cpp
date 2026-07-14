#include <notojs/module/dom/lexbor.hpp>
#include <iostream>

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

} // namespace notojs::dom::lexbor
