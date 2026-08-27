#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct SVGElement : HTMLElement
{
    BOOST_FORCEINLINE SVGElement(Node const &node)
    : HTMLElement(node) {}

    BOOST_FORCEINLINE SVGElement(std::shared_ptr<Backend> doc, lxb_html_element_t *node)
    : HTMLElement(std::move(doc), node) {}

    std::unordered_map<Attr::Name, JSValue, Attr::Name::Hash> attributes;
};

} // namespace notojs::dom
