#pragma once
#include <notojs/module/dom/node.hpp>

namespace notojs::dom {

struct XMLDocument : Node
{
    BOOST_FORCEINLINE XMLDocument(std::shared_ptr<XMLBackend> const &backend)
    : Node{backend, backend->doc.internal_object()} {}

    BOOST_FORCEINLINE operator pugi::xml_node_struct *() const
    {
        return static_cast<pugi::xml_node_struct *>(node);
    }

    JSValue createElement(std::string_view const &);
    JSValue createTextNode(std::string_view const &);
    JSValue documentElement() const;

    std::string toString() const;
};

} // namespace notojs:dom
