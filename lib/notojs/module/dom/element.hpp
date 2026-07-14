#pragma once
#include <notojs/module/dom/attr.hpp>

namespace notojs::dom {

struct Element : Node
{
    BOOST_FORCEINLINE Element(std::shared_ptr<Backend> doc, void *node)
    : Node(std::move(doc), node) {}

    BOOST_FORCEINLINE operator pugi::xml_node_struct *() const
    {
        return static_cast<pugi::xml_node_struct *>(node);
    }

    virtual std::uint64_t attributesCount() const;
    virtual std::vector<Attr::Name> attrs() const;
    virtual void clear();
    virtual std::optional<std::string_view> lookupNS(uintptr_t) const;
    virtual std::optional<Attr::Name> getAttributeName(std::int64_t) const;
    virtual std::optional<std::string_view> getAttribute(Attr::Name::View const &) const;
    virtual bool hasAttribute(Attr::Name::View const &) const;
    virtual void removeAttribute(Attr::Name::View const &);
    virtual void setAttribute(Attr::Name::View const &, std::string_view const &);
    virtual bool toggleAttribute(Attr::Name::View const &);
    virtual bool toggleAttribute(Attr::Name::View const &, bool);
    std::string toString() const;

    static bool isTagName(std::string_view const &sv);
};

} // namespace notojs:dom
