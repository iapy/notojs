#pragma once
#include <notojs/module/dom/node.hpp>

namespace notojs::dom {

struct Attr : Node 
{
    struct Name
    {
        std::string name;
        std::uintptr_t ns{LXB_NS_HTML};

        struct View
        {
            std::string_view name;
            std::uintptr_t ns{LXB_NS_HTML};
        };

        BOOST_FORCEINLINE operator View() const
        {
            return {{name.c_str(), name.size()}, ns};
        }

        BOOST_FORCEINLINE bool operator < (Name const &other) const
        {
            return ns == other.ns ? name < other.name : ns < other.ns;
        }

        BOOST_FORCEINLINE bool operator == (Name const &other) const
        {
            return ns == other.ns && name == other.name;
        }

        struct Hash
        {
            BOOST_FORCEINLINE std::size_t operator () (Name const &name) const
            {
                std::size_t h = std::hash<std::string>{}(name.name);
                h ^= std::hash<std::uintptr_t>{}(name.ns)+ 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };
    };

    BOOST_FORCEINLINE Attr(std::shared_ptr<Backend> doc, void *node, Name &&name)
    : Node(doc, node), name{std::move(name)} {}

    BOOST_FORCEINLINE Attr(std::shared_ptr<Backend> doc, void *node, Name const &name)
    : Node(doc, node), name{name} {}

    BOOST_FORCEINLINE Attr(std::shared_ptr<Backend> doc, Name const &name, std::string &&value)
    : Node{doc, nullptr}, name{name}, value{value} {}

    Name name;
    std::optional<std::string> value;
};

} // namespace notojs:dom
