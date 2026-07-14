#pragma once
#include <boost/config.hpp>
#include <lmdbxx/lmdb++.h>
#include <string>
#include <vector>
#include <array>

namespace notojs::detail {

struct INI
{
    enum class Line : char
    {
        SYNTAX = 0,
        MODULE = 1,
        SCRIPT = 2
    };

    using size_type = std::uint16_t;
    static constexpr std::size_t tail_size = 2 + sizeof(size_type);
    static constexpr std::size_t tail_null = 1 + sizeof(size_type);

    struct Value
    {
        std::vector<char> raw;
        lmdb::val val;

        BOOST_FORCEINLINE void make()
        {
            val.assign(&raw[0], raw.size());
        }

        BOOST_FORCEINLINE void text(std::string const &s)
        {
            raw.assign(s.c_str(), s.c_str() + s.size());
        }

        BOOST_FORCEINLINE void line(size_type sz)
        {
            raw.push_back(0);
            sz = htons(sz);

            char const *p = reinterpret_cast<char const *>(&sz);
            raw.insert(std::end(raw), p, p + sizeof(sz));
        }

        BOOST_FORCEINLINE void path(Line type, std::string const &p, size_type sz)
        {
            raw.assign(p.c_str(), p.c_str() + p.size() + 1);
            raw.push_back(static_cast<char>(type));
            sz = htons(sz);

            char const *q = reinterpret_cast<char const *>(&sz);
            raw.insert(std::end(raw), q, q + sizeof(sz));
        }
    };

    BOOST_FORCEINLINE static Line line_type(lmdb::val const &v)
    {
        return v.size() ? static_cast<Line>(v.data()[v.size() - tail_null]) : Line::SYNTAX;
    }

    BOOST_FORCEINLINE static bool is_module(lmdb::val const &v)
    {
        return v.size() && static_cast<Line>(v.data()[v.size() - tail_null]) == Line::MODULE;
    }

    BOOST_FORCEINLINE static bool is_script(lmdb::val const &v)
    {
        return v.size() && static_cast<Line>(v.data()[v.size() - tail_null]) == Line::SCRIPT;
    }

    using Entry = std::array<Value, 2>;
};

} // namespace notojs::detail
