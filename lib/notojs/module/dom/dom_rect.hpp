#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct DOMRect : HTMLElement
{
    BOOST_FORCEINLINE DOMRect(HTMLElement const &el, Attr::Name &&attr)
    : HTMLElement(el.doc, el), attr{std::move(attr)} {}

    Attr::Name const attr;
};

} // namespace notojs:dom
