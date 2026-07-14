#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct CSSStyleDeclaration : HTMLElement
{
    BOOST_FORCEINLINE CSSStyleDeclaration(std::shared_ptr<Backend> doc, lxb_html_element_t *node)
    : HTMLElement(std::move(doc), node) {}
};

} // namespace notojs::dom
