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

    JSValue createCDATASection(std::string_view const &);
    JSValue createComment(std::string_view const &);
    JSValue createProcessingInstruction(std::string_view const &, std::string_view const &);
    JSValue createElement(std::string_view const &);
    JSValue createTextNode(std::string_view const &);
    JSValue documentElement() const;

    std::string toString() const;
};

} // namespace notojs:dom
