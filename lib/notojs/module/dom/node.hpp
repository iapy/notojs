#pragma once
#include <notojs/module/dom/backend.hpp>

namespace notojs::dom {

struct Node
{
    BOOST_FORCEINLINE Node(std::shared_ptr<Backend> doc, void *node)
    : doc{std::move(doc)}, node{node} {}

    BOOST_FORCEINLINE Node(Node &&) = default;
    Node(Node const &) = delete;

    BOOST_FORCEINLINE Node &operator = (Node &&) = default;
    Node &operator = (Node const &other) = delete;

    std::shared_ptr<Backend> doc;
    void *node;
};

} // namespace notojs:dom
