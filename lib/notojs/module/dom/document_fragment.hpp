#pragma once
#include <notojs/module/dom/attr.hpp>

namespace notojs::dom {

struct DocumentFragment : Node
{
    BOOST_FORCEINLINE DocumentFragment(Node const &node)
    : Node(node.doc, static_cast<lxb_dom_node_t *>(node.node)) {}

    BOOST_FORCEINLINE DocumentFragment(std::shared_ptr<Backend> doc, lxb_dom_node_t *node)
    : Node(std::move(doc), node) {}

    BOOST_FORCEINLINE operator lxb_dom_node_t *() const
    {
        return static_cast<lxb_dom_node_t *>(node);
    }
};

} // namespace notojs:dom
