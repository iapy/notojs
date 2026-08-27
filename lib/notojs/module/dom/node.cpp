#include <notojs/module/dom/node.hpp>
#include <algorithm>

namespace notojs::dom {

namespace {

struct Entry
{
    void *node;
    std::uint16_t type;
};

BOOST_FORCEINLINE bool validChildType(std::uint16_t type)
{
    return 1 == type || 3 == type || 4 == type || 7 == type || 8 == type || 10 == type;
}

BOOST_FORCEINLINE  bool sameNode(Node const *node, void *handle)
{
    return node && node->node == handle;
}

} // namespace

MutationPlan::operator bool () const
{
    auto const &plan = *this;
    auto const parentType = plan.parent.doc->nodeType(plan.parent);
    if(1 != parentType && 9 != parentType && 11 != parentType) return false;

    std::vector<Entry> inserted;
    for(auto const &candidate: plan.inserted)
    {
        if(!candidate.node)
        {
            if(!validChildType(candidate.type)) return false;
            inserted.push_back({nullptr, candidate.type});
            continue;
        }

        if(plan.parent.doc != candidate.node->doc) return false;
        if(candidate.node->node == plan.parent.node
            || plan.parent.doc->contains(*candidate.node, plan.parent)) return false;

        auto const type = candidate.node->doc->nodeType(*candidate.node);
        if(type != candidate.type || (11 != type && !validChildType(type))) return false;

        if(11 == type)
        {
            for(auto current = candidate.node->doc->first(*candidate.node); current;
                current = candidate.node->doc->next(*current))
            {
                auto const childType = candidate.node->doc->nodeType(*current);
                if(!validChildType(childType)) return false;
                inserted.push_back({current->node, childType});
            }
        }
        else inserted.push_back({candidate.node->node, type});
    }

    std::vector<Entry> result;
    std::size_t insertion = 0;
    bool foundReference = !plan.reference;

    for(auto current = plan.parent.doc->first(plan.parent); current;
        current = plan.parent.doc->next(*current))
    {
        if(sameNode(plan.reference, current->node))
        {
            insertion = result.size();
            foundReference = true;
        }

        bool removed = false;
        for(auto const *node: plan.removed)
        {
            if(sameNode(node, current->node))
            {
                removed = true;
                break;
            }
        }
        if(removed) continue;

        bool moving = false;
        for(auto const &entry: inserted)
        {
            if(entry.node && entry.node == current->node)
            {
                moving = true;
                break;
            }
        }
        if(moving) continue;

        auto const type = plan.parent.doc->nodeType(*current);
        if(!validChildType(type)) return false;
        result.push_back({current->node, type});
    }

    if(!plan.reference || !foundReference) insertion = result.size();

    for(auto const &entry: inserted)
    {
        if(entry.node)
        {
            auto existing = std::find_if(std::begin(result), std::end(result),
                [&entry](Entry const &current) { return current.node == entry.node; });
            if(existing != std::end(result))
            {
                auto const index = static_cast<std::size_t>(std::distance(std::begin(result), existing));
                if(index < insertion) --insertion;
                result.erase(existing);
            }
        }

        result.insert(std::begin(result) + insertion, entry);
        ++insertion;
    }

    if(9 != parentType)
    {
        for(auto const &entry: result)
            if(10 == entry.type) return false;
        return true;
    }

    std::size_t elements = 0;
    std::size_t doctypes = 0;
    bool foundElement = false;
    for(auto const &entry: result)
    {
        switch(entry.type)
        {
            case 1:
                if(++elements > 1) return false;
                foundElement = true;
                break;
            case 7:
            case 8:
                break;
            case 10:
                if(++doctypes > 1 || foundElement) return false;
                break;
            default:
                return false;
        }
    }
    return true;
}

bool Node::canHaveChild(Node const &child, Node const *reference,
                        Node const *replaced) const
{
    MutationPlan plan{*this, reference, {}, {{&child, child.doc->nodeType(child)}}};
    if(replaced) plan.removed.push_back(replaced);
    return static_cast<bool>(plan);
}

} // namespace notojs::dom
