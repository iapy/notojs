#pragma once
#include <notojs/module/dom/html_element.hpp>

namespace notojs::dom {

struct NamedNodeMap : protected Element
{
    BOOST_FORCEINLINE NamedNodeMap(Element const &el)
    : Element(el.doc, el) {}

    using Element::doc;
    using Element::node;

    using Element::attrs;
    using Element::attributesCount;
    using Element::getAttribute;
    using Element::getAttributeName;
    using Element::hasAttribute;
    using Element::removeAttribute;
    using Element::setAttribute;

    virtual JSValue ownerElement()
    {
        return dynamic_cast<XMLBackend*>(doc.get())->make(static_cast<pugi::xml_node_struct *>(node));
    }

    std::unordered_map<Attr::Name, JSValue, Attr::Name::Hash> attributes;
};

struct HTMLNamedNodeMap : NamedNodeMap
{
    BOOST_FORCEINLINE HTMLNamedNodeMap(HTMLElement const &el)
    : NamedNodeMap(el) {}

    std::vector<Attr::Name> attrs() const override
    {
        return static_cast<HTMLElement>(*this).attrs();
    }

    std::uint64_t attributesCount() const override
    {
        return static_cast<HTMLElement>(*this).attributesCount();
    }

    std::optional<std::string_view> getAttribute(Attr::Name::View const &a) const override
    {
        return static_cast<HTMLElement>(*this).getAttribute(a);
    }

    std::optional<Attr::Name> getAttributeName(std::int64_t i) const override
    {
        return static_cast<HTMLElement>(*this).getAttributeName(i);
    }

    bool hasAttribute(Attr::Name::View const &a) const override
    {
        return static_cast<HTMLElement>(*this).hasAttribute(a);
    }

    void removeAttribute(Attr::Name::View const &a) override
    {
        static_cast<HTMLElement>(*this).removeAttribute(a);
    }

    void setAttribute(Attr::Name::View const &a, std::string_view const &v) override
    {
        static_cast<HTMLElement>(*this).setAttribute(a, v);
    }

    JSValue ownerElement() override
    {
        return dynamic_cast<HTMLBackend*>(doc.get())->make(static_cast<lxb_dom_node_t *>(node));
    }
};

} // namespace notojs::dom
