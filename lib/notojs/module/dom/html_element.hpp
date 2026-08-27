#pragma once
#include <notojs/module/dom/element.hpp>

namespace notojs::dom {

struct HTMLElement : Element
{
    BOOST_FORCEINLINE HTMLElement(Node const &node)
    : Element(node.doc, static_cast<lxb_dom_node_t *>(node.node)) {}

    BOOST_FORCEINLINE HTMLElement(std::shared_ptr<Backend> doc, lxb_html_element_t *node)
    : Element(std::move(doc), node) {}

    BOOST_FORCEINLINE operator lxb_dom_node_t *() const
    {
        return static_cast<lxb_dom_node_t *>(node);
    }

    BOOST_FORCEINLINE operator lxb_dom_element_t *() const
    {
        return lxb_dom_interface_element(static_cast<lxb_dom_node_t *>(node));
    }

    BOOST_FORCEINLINE operator lxb_html_element_t *() const
    {
        return lxb_html_interface_element(static_cast<lxb_dom_node_t *>(node));
    }

    lxb_dom_node_t *root() const;
    lxb_dom_node_t *closest(lxb_tag_id_t) const;
    lxb_dom_node_t *directChild(lxb_tag_id_t) const;

    std::vector<Attr::Name> attrs() const override;
    std::uint64_t attributesCount() const override;
    void clear() override;
    std::optional<std::string_view> lookupNS(uintptr_t) const override;
    std::optional<Attr::Name> getAttributeName(std::int64_t) const override;
    std::optional<std::string_view> getAttribute(Attr::Name::View const &a) const override;
    bool hasAttribute(Attr::Name::View const &) const override;

    std::string innerHTML() const;
    std::string innerText() const;

    template<typename T>
    void innerHTML(T const &);
    void innerText(std::string_view const &);
    void outerText(std::string_view const &);

    template<typename T>
    void outerHTML(T const &);

    void removeAttribute(Attr::Name::View const &) override;
    void setAttribute(Attr::Name::View const &, std::string_view const &) override;
    bool toggleAttribute(Attr::Name::View const &) override;
    bool toggleAttribute(Attr::Name::View const &, bool) override;
    std::string toString() const;

    static std::string className(std::string_view const &str);
};

} // namespace notojs:dom
