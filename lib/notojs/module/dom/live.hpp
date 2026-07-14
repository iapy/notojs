#pragma once
#include <lexbor/dom/interfaces/node.h>
#include <boost/config.hpp>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace notojs::dom {

struct LiveCollection
{
    BOOST_FORCEINLINE void pub()
    {
        ++version;
    }
    BOOST_FORCEINLINE bool upd(std::uint32_t &v)
    {
        return version != std::exchange(v, version);
    }

    struct Map : private std::unordered_map<lxb_dom_node_t *, LiveCollection>
    {
        BOOST_FORCEINLINE void pub(lxb_dom_node_t *node)
        {
            if(empty()) return;
            for(;node;node = node->parent)
                if(auto it = find(node); it != end())
                    it->second.pub();
        }
        BOOST_FORCEINLINE void reg(lxb_dom_node_t *node)
        {
            ++operator [](node).count;
        }
        BOOST_FORCEINLINE bool upd(lxb_dom_node_t *node, std::uint32_t &v)
        {
            if(auto it = find(node); it != end())
                return it->second.upd(v);
            return false;
        }
        BOOST_FORCEINLINE void erase(lxb_dom_node_t *node)
        {
            if(auto it = find(node); it != end() && !--it->second.count)
                std::unordered_map<lxb_dom_node_t *, LiveCollection>::erase(it);
        }
        using std::unordered_map<lxb_dom_node_t *, LiveCollection>::size;
    };

private:
    std::size_t count{0};
    std::uint32_t version{0};
};

struct LiveStyle
{
    BOOST_FORCEINLINE void pub()
    {
        ++version;
    }
    BOOST_FORCEINLINE bool upd(std::uint32_t &v)
    {
        return version != std::exchange(v, version);
    }

    struct Map : private std::unordered_map<lxb_dom_node_t *, LiveStyle>
    {
        BOOST_FORCEINLINE void pub()
        {
            for(auto &[_, live]: *this) live.pub();
        }
        BOOST_FORCEINLINE void pub(lxb_dom_node_t *node)
        {
            if(auto it = find(node); it != end())
                it->second.pub();
        }
        BOOST_FORCEINLINE void reg(lxb_dom_node_t *node)
        {
            emplace(node, LiveStyle{});
        }
        BOOST_FORCEINLINE bool upd(lxb_dom_node_t *node, std::uint32_t &v)
        {
            if(auto it = find(node); it != end())
                return it->second.upd(v);
            return false;
        }
        using std::unordered_map<lxb_dom_node_t *, LiveStyle>::erase;
        using std::unordered_map<lxb_dom_node_t *, LiveStyle>::size;
    };

private:
    std::uint32_t version{0};
};

} // namespace notojs::dom
