#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct DOMStringMap : HTMLElement
{
    BOOST_FORCEINLINE DOMStringMap(HTMLElement const &el)
    : HTMLElement{el.doc, el} {}

    Attr::Name del_property(char const *);
    JSValue get_property(char const *) const;
    bool has_property(char const *) const;
    std::vector<std::string> own_properties() const;
    bool own_property(char const *, JSPropertyDescriptor *) const;
    void set_property(char const *, std::string_view const &);
};

} // namespace notojs:dom
