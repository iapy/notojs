#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct DOMTokenList : HTMLElement
{
    BOOST_FORCEINLINE DOMTokenList(HTMLElement const &el, Attr::Name &&attr)
    : HTMLElement(el.doc, el), attr{std::move(attr)} {}

    JSValue item(std::int64_t const, JSValue) const;
    std::uint64_t length() const;
    JSValue values() const;

    BOOST_FORCEINLINE static bool isspace(char ch)
    {
        return 0x09 == ch || 0x0A == ch || 0x0C == ch || 0x0D == ch || 0x20 == ch;
    }

    Attr::Name const attr;
};

} // namespace notojs:dom
