#pragma once
#include <boost/config.hpp>
#include <iostream>
#include <cstring>
#include <chrono>

namespace notojs::detail {

struct Logger : std::streambuf
{
    Logger(): underlying{std::clog.rdbuf(this)}
    {
    }

    ~Logger()
    {
        underlying->pubsync();
        std::clog.rdbuf(underlying);
    }

private:
    static constexpr char format[] = "%Y-%m-%d %H:%M:%S";
    static constexpr std::size_t fmtcnt = sizeof("9999-12-31 23:59:29");

    BOOST_FORCEINLINE void timestamp()
    {
        auto const t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto const n = std::strftime(buffer, fmtcnt, format, std::localtime(&t));
        underlying->sputn(buffer, n);
        underlying->sputn(" -- ", 4);
    }

protected:
    int_type overflow(int_type ch) override
    {
        if(std::exchange(prefix, false)) timestamp();
        prefix = ('\n' == ch);
        return underlying->sputc(ch);
    }

    std::streamsize xsputn(const char_type *s, std::streamsize count) override
    {
        if(count && std::exchange(prefix, false)) timestamp();
        for(std::size_t i = 0; i < count; ++i)
        {
            if(s[i] == '\n')
            {
                prefix = true;
                return underlying->sputn(s, i + 1) + xsputn(s + i + 1, count - i - 1);
            }
        }
        return underlying->sputn(s, count);
    }

    int sync() override
    {
        return underlying->pubsync();
    }

private:
    bool prefix{true};
    char buffer[fmtcnt];
    std::streambuf *underlying;
};

} // namespace notojs::detail
