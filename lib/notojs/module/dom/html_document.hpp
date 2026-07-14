#pragma once
#include <notojs/module/dom/node.hpp>
#include <optional>

namespace notojs::dom {

struct HTMLDocument : Node
{
    BOOST_FORCEINLINE HTMLDocument(std::shared_ptr<HTMLBackend> const &backend)
    : Node{backend, lxb_dom_interface_node(backend->doc.get())} {}

    BOOST_FORCEINLINE operator lxb_dom_node_t *() const
    {
        return static_cast<lxb_dom_node_t *>(node);
    }

    BOOST_FORCEINLINE operator lxb_dom_element_t *() const
    {
        return lxb_dom_interface_element(static_cast<lxb_dom_node_t *>(node));
    }

    BOOST_FORCEINLINE operator lxb_dom_document_t *() const
    {
        return lxb_dom_interface_document(std::dynamic_pointer_cast<HTMLBackend>(doc)->doc.get());
    }

    BOOST_FORCEINLINE operator lxb_html_document_t *() const
    {
        return std::dynamic_pointer_cast<HTMLBackend>(doc)->doc.get();
    }

    JSValue body() const;
    JSValue head() const;
    JSValue createElement(std::string_view const &);
    JSValue createElement(std::string_view const &, uintptr_t ns);
    JSValue createTextNode(std::string_view const &);
    JSValue documentElement() const;
    JSValue getElementById(std::string_view const &) const;
    std::uintptr_t lookupNS(std::string_view const &) const;

    void title(std::optional<std::string_view> const &);
    std::optional<std::string> title() const;

    std::string toString() const;
};

} // namespace notojs:dom
