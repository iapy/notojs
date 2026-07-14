#pragma once
#include <lexbor/style/style.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/dom.h>

#include <optional>
#include <string>

namespace notojs::dom::lexbor {

lxb_dom_node_t *head(lxb_dom_node_t *);
lxb_dom_node_t *title(lxb_dom_node_t *, bool);

std::string_view get_name(uintptr_t name);
std::string_view get_name(lxb_dom_element_t *node);
std::string_view get_name(lxb_dom_document_type_t *node);
std::string get_text(lxb_dom_node_t *node);

void del_text(lxb_dom_node_t *node);
void set_text(lxb_dom_node_t *node, std::string_view const &);

} // namespace notojs::dom::lexbor
