#pragma once
#include <boost/config.hpp>
#include <filesystem>
#include <iostream>
#include <utility>
#include <chrono>
#include <thread>

#include <rapidjson/writer.h>

namespace notojs {
namespace detail {

struct Values
{
    struct Raw
    {
        std::string const value;
    };

    BOOST_FORCEINLINE Raw raw(std::string &&value) const
    {
        return Raw{value};
    }

    template<typename ...Ts>
    BOOST_FORCEINLINE std::tuple<Ts&...> operator () (Ts&&... args) const
    {
        return std::tuple<Ts&...>(args...);
    }

    class Writer
    {
    public:
        Writer(std::ostream &stream)
        : stream{stream} {}

        using Ch = char;
        void Put(Ch ch)
        {
            stream.write(&ch, 1);
        }
        void Flush()
        {
            stream.flush();
        }

    private:
        std::ostream &stream;
    };

    template<typename T, std::size_t I, std::size_t ...Is>
    static void write(rapidjson::Writer<Values::Writer> &w, T const &tuple, std::index_sequence<I, Is...>)
    {
        auto const &value = std::get<I>(tuple);
        using U = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<U, std::thread::id>)
        {
            static std::hash<std::thread::id> hash;
            w.Uint64(hash(value) % 10000);
        }
        if constexpr (std::is_same_v<U, char const *>)
        {
            w.String(value, std::strlen(value));
        }
        if constexpr (std::is_same_v<U, std::string>)
        {
            w.String(value.c_str(), value.size());
        }
        if constexpr (std::is_integral_v<U>)
        {
            if constexpr (std::is_unsigned_v<U>)
                w.Uint64(value);
            else
                w.Int64(value);
        }
        if constexpr (std::is_floating_point_v<U>)
        {
            w.Double(value);
        }
        if constexpr (std::is_same_v<U, std::chrono::system_clock::duration>)
        {
            w.StartArray();
            w.Int64(std::chrono::duration_cast<std::chrono::milliseconds>(value).count());
            w.String("ms", 2);
            w.EndArray();
        }
        if constexpr (std::is_same_v<U, Raw>)
        {
            w.RawValue(value.value.c_str(), value.value.size(), rapidjson::kObjectType);
        }
        if constexpr (std::is_same_v<U, std::filesystem::path>)
        {
            auto const s = value.u8string();
            w.String(s.c_str(), s.size());
        }
        if constexpr (sizeof...(Is))
        {
            write(w, tuple, std::index_sequence<Is...>{});
        }
    }
};

} // namespace detail

constexpr auto values = notojs::detail::Values{};

} // namespace notojs

BOOST_FORCEINLINE std::ostream &operator << (std::ostream &stream, notojs::detail::Values const &values)
{
    stream << '\n';
    stream.flush();
    return stream;
}

template<typename ...Ts>
std::ostream &operator << (std::ostream &stream, std::tuple<Ts&...> const &tup)
{
    notojs::detail::Values::Writer writer(stream);
    rapidjson::Writer<decltype(writer)> w(writer);
    stream << ' ';
    w.StartArray();
    notojs::detail::Values::write(
        w, tup, std::make_index_sequence<sizeof ...(Ts)>{}
    );
    w.EndArray();
    stream << '\n';
    stream.flush();
    return stream;
}

#include <boost/preprocessor.hpp>

#define NOTOJS_PP_EMIT_CAPTURE(r, data, i, elem) \
    , v##i=(elem)

#define NOTOJS_PP_CAPTURE(seq) \
    BOOST_PP_SEQ_FOR_EACH_I(NOTOJS_PP_EMIT_CAPTURE, _, seq)

#define NOTOJS_PP_EMIT_VALUES(r, data, i, elem) \
    BOOST_PP_COMMA_IF(i) v##i

#define NOTOJS_PP_VALUES(seq) \
    BOOST_PP_SEQ_FOR_EACH_I(NOTOJS_PP_EMIT_VALUES, _, seq)

#define NOTOJS_LOG_2(msg, seq) \
    boost::asio::post(*get<notojs::Server>().disk, [message=msg NOTOJS_PP_CAPTURE(seq)]{ \
        std::clog << message << notojs::values(NOTOJS_PP_VALUES(seq)); \
    })

#define NOTOJS_LOG_3(ctx, msg, seq) \
    boost::asio::post(*(ctx)->get<notojs::Server>().disk, [message=msg NOTOJS_PP_CAPTURE(seq)]{ \
        std::clog << message << notojs::values(NOTOJS_PP_VALUES(seq)); \
    })

#define NOTOJS_LOG(...) \
    BOOST_PP_OVERLOAD(NOTOJS_LOG_, __VA_ARGS__)(__VA_ARGS__)

#define NOTOJS_LOG_RAW_2(msg, raw) \
    boost::asio::post(*get<notojs::Server>().disk, [message=msg, r=raw]{ \
        std::clog << message << ' ' << r << '\n'; \
    })

#define NOTOJS_LOG_RAW_3(ctx, msg, raw) \
    boost::asio::post(*(ctx)->get<notojs::Server>().disk, [message=msg, r=raw]{ \
        std::clog << message << ' ' << r << '\n'; \
    })

#define NOTOJS_LOG_RAW(...) \
    BOOST_PP_OVERLOAD(NOTOJS_LOG_RAW_, __VA_ARGS__)(__VA_ARGS__)

#define NOTOJS_LOG_MSG_1(msg) \
    boost::asio::post(*get<notojs::Server>().disk, [message=msg]{ \
        std::clog << message << notojs::values; \
    })

#define NOTOJS_LOG_MSG_2(ctx, msg) \
    boost::asio::post(*(ctx)->get<notojs::Server>().disk, [message=msg]{ \
        std::clog << message << notojs::values; \
    })

#define NOTOJS_LOG_MSG(...) \
    BOOST_PP_OVERLOAD(NOTOJS_LOG_MSG_, __VA_ARGS__)(__VA_ARGS__)

#ifdef DEBUG_LOGGING
#define NOTOJS_LOG_THIS_THREAD std::clog << __FILE__ << ':' << __LINE__ << notojs::values(std::this_thread::get_id());
#else
#define NOTOJS_LOG_THIS_THREAD
#endif
