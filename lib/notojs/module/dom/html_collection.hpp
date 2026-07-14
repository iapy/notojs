#pragma once
#include <notojs/module/dom/backend.hpp>
#include <notojs/module/dom/node.hpp>
#include <iostream>

namespace notojs::dom {

struct HTMLCollection : Node
{
    BOOST_FORCEINLINE HTMLCollection(Node &&node, std::vector<void*> &&out)
    : Node{std::move(node)}, out{std::move(out)}
    {
    }

    BOOST_FORCEINLINE HTMLCollection(Node &&node, std::string &&sel, uintptr_t ns = LXB_NS__UNDEF)
    : Node{std::move(node)}, ns{ns}, sel{std::move(sel)}
    {
        init();
    }

    BOOST_FORCEINLINE operator lxb_dom_node_t *() const
    {
        return static_cast<lxb_dom_node_t *>(node);
    }

    BOOST_FORCEINLINE std::uint64_t size() const
    {
        if(!sel.empty()) update();
        return static_cast<std::uint64_t>(out.size());
    }

    BOOST_FORCEINLINE JSValue at(std::uint64_t i, JSValue def) const
    {
        if(!sel.empty()) update();
        return doc->collection_item(out, i, def);
    }

    BOOST_FORCEINLINE operator JSValue() const
    {
        if(!sel.empty()) update();
        return doc->collection(out);
    }

    void free();

private:
    void init();
    void update() const;

private:
    uintptr_t const ns{LXB_NS__UNDEF};
    std::string const sel;
    mutable std::vector<void*> out;
    mutable std::uint32_t gen{std::numeric_limits<std::uint32_t>::max()};
};

} // namespace notojs:dom
