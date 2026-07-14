#pragma once
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include "bridge.hpp"

#include <optional>
#include <string>
#include <vector>

namespace notojs {

struct IBlob : bridge::Interface<IBlob, void*>
{
    virtual std::string type() const = 0;
    virtual std::shared_ptr<std::vector<std::uint8_t>> copy() const = 0;
    virtual std::pair<std::uint8_t const *, std::size_t> data() const = 0;
    struct Static
    {
        static JSValue make(JSContext *, std::vector<std::uint8_t> &&, std::string const &type = "");
        static JSValue make(JSContext *, std::uint8_t const *, std::size_t, std::string const &type = "");
    };
    virtual ~IBlob() {}
};

struct IURL : bridge::Interface<IURL, void*>
{
    virtual std::string href() const = 0;
    struct Static
    {
        static JSValue make(JSContext *ctx, boost::urls::url &&);
        static std::optional<boost::urls::url> parse(char const *);
    };
    virtual ~IURL() {}
};

namespace facade {

using Blob = notojs::IBlob::Impl;
using URL = notojs::IURL::Impl;

JSValue clog(JSContext *,
    int,
    JSValueConst *);

JSValue fetch(JSContext *,
    boost::beast::http::request<boost::beast::http::string_body> &&,
    boost::urls::url &&,
    JSValue(*)(
        JSContext *, JSValue,
        boost::beast::http::response<boost::beast::http::string_body> const &
    ));

JSValue print(JSContext *,
    int,
    JSValueConst *);

} // namespace facade
} // namespace notojs

namespace noto {
using namespace notojs::facade;
} // namespace noto
