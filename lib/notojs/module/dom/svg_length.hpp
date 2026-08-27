#pragma once
#include <notojs/module/dom/attr.hpp>
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct SVGLength : HTMLElement
{
    BOOST_FORCEINLINE SVGLength(std::shared_ptr<Backend> doc, lxb_html_element_t *node, char const *name)
    : HTMLElement(std::move(doc), node), name{name, LXB_NS__UNDEF} {}

    Attr::Name name;
};

} // namespace notojs:dom
