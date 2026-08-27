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

    bool canHaveChild(Node const &child, Node const *reference = nullptr,
                      Node const *replaced = nullptr) const;

    std::shared_ptr<Backend> doc;
    void *node;
};

struct MutationCandidate
{
    Node const *node;
    std::uint16_t type;
};

struct MutationPlan
{
    Node const &parent;
    Node const *reference;
    std::vector<Node const *> removed;
    std::vector<MutationCandidate> inserted;

    operator bool () const;
};

} // namespace notojs:dom
