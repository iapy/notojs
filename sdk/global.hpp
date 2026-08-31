#pragma once
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "bridge.hpp"
#include <chrono>
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

struct IFile : bridge::Interface<IFile, void*>
{
    virtual std::string name() const = 0;
    virtual std::string type() const = 0;
    virtual std::int64_t last_modified() const = 0;
    virtual std::shared_ptr<std::vector<std::uint8_t>> copy() const = 0;
    virtual std::pair<std::uint8_t const *, std::size_t> data() const = 0;
    struct Static
    {
        static JSValue make(JSContext *, std::vector<std::uint8_t> &&, std::string const &name, std::int64_t, std::string const &type = "");
        static JSValue make(JSContext *, std::uint8_t const *, std::size_t, std::string const &name, std::int64_t, std::string const &type = "");
    };
    virtual ~IFile() {}
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

struct IPrint : bridge::Interface<IPrint, void*>
{
    virtual JSValue print(JSContext *ctx, bridge::Array) const = 0;
    virtual ~IPrint() {}
};

namespace facade {

using Blob = notojs::IBlob::Impl;
using File = notojs::IFile::Impl;
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
    ),
    std::chrono::milliseconds timeout = std::chrono::milliseconds{10000});

JSValue print(JSContext *,
    int,
    JSValueConst *);

JSValue import(JSContext *,
    char const *);

} // namespace facade
} // namespace notojs

namespace noto {
using namespace notojs::facade;
} // namespace noto
