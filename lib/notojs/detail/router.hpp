#pragma once
#include <notojs/parser/uri.hpp>

#include <array>
#include <utility>
#include <algorithm>
#include <functional>
#include <string_view>

namespace notojs::detail {

template<std::size_t N>
constexpr auto route(char const (&data)[N])
{
    return std::string_view(data, N - 1);
}

template<std::size_t N>
struct Router
{
    using P = std::pair<std::string_view, std::size_t>;
    std::array<P, N> routes;

    constexpr Router(std::initializer_list<std::string_view> paths)
    {
        auto it = std::begin(paths);
        for(std::size_t i = 0; i < N; ++i, ++it)
        {
            std::size_t j = 0;
            for(; j < i; ++j)
            {
                if(*it < routes[j].first)
                {
                    for(std::size_t k = i; k > j; --k)
                    {
                        routes[k].first = routes[k - 1].first;
                        routes[k].second = routes[k - 1].second;
                    }
                    break;
                }
                if(*it == routes[j].first)
                {
                    break;
                }
            }
            routes[j].first = *it;
            routes[j].second = i;
        }
    }

    constexpr std::size_t operator [] (std::string_view const v) const
    {
        for(std::size_t i = 0; i < N && v >= routes[i].first; ++i)
        {
            if(v == routes[i].first) return routes[i].second;
        }
        return N;
    }

    struct Comparator
    {
        std::size_t index{1};
        bool operator () (P const &v1, std::string_view const &v2) const
        {
            for(std::size_t i = index; i < std::min(v1.first.size(), v2.size()); ++i)
            {
                if(v2[i] == '/') return v1.first[i] < '/';
                if(v1.first[i] == '/') return v2[i] > '/';
                if(v1.first[i] != v2[i]) return v1.first[i] < v2[i];
            }
            return v1.first.size() < v2.size();
        }
        bool operator () (std::string_view const &v1, P const &v2) const
        {
            for(std::size_t i = index; i < std::min(v1.size(), v2.first.size()); ++i)
            {
                if(v1[i] == '/') return v2.first[i] > '/';
                if(v2.first[i] == '/') return v1[i] < '/';
                if(v1[i] != v2.first[i]) return v1[i] < v2.first[i];
            }
            return v1.size() < v2.first.size();
        }
    };

    std::size_t find(std::string_view v, std::string &tail) const
    {
        Comparator c;
        auto b = std::begin(routes), e = std::end(routes);
        auto l = std::lower_bound(b, e, v, c), u = std::upper_bound(l, e, v, c);
        while(std::distance(l, u) > 1)
        {
            c.index = v.find('/', c.index + 1) + 1;
            if(l->first.size() == c.index) return l->second;

            b = l;
            e = u;
            l = std::lower_bound(b, e, v, c);
            u = std::upper_bound(l, e, v, c);
        }
        return (l == u ? N : (std::invoke([b=l->first.size(), e=v.size(), &v, &tail]{
            parser::decodeURIComponent(&v[b], &v[e], true).swap(tail);
        }), l->second));
    }
};

template<typename ...Rs>
constexpr auto router(Rs ...rs)
{
    return Router<sizeof ...(Rs)>({rs...});
}

} // namespace notojs::detail
