#pragma once

#ifndef NOTOJS_VERSION_NS
#ifndef NOTOJS_INTERNAL_MODULE
#cmakedefine NOTOJS_VERSION_NS @NOTOJS_VERSION_NS@
#else
#error "NOTOJS_VERSION_NS must be defined"
#endif
#endif

#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <bridge.hpp>
#include <optional>
#include <chrono>
#include <string>
#include <vector>

namespace notojs::NOTOJS_VERSION_NS {

struct Blob : bridge::Interface<Blob>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string type() const = 0;
        virtual std::shared_ptr<std::vector<std::uint8_t>> copy() const = 0;
        virtual std::pair<std::uint8_t const *, std::size_t> data() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *, std::vector<std::uint8_t> &&, std::string const &type = "");
    static JSValue make(JSContext *, std::uint8_t const *, std::size_t, std::string const &type = "");
};

struct File : bridge::Interface<File>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string name() const = 0;
        virtual std::string type() const = 0;
        virtual std::int64_t last_modified() const = 0;
        virtual std::shared_ptr<std::vector<std::uint8_t>> copy() const = 0;
        virtual std::pair<std::uint8_t const *, std::size_t> data() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *, std::vector<std::uint8_t> &&, std::string const &name, std::int64_t, std::string const &type = "");
    static JSValue make(JSContext *, std::uint8_t const *, std::size_t, std::string const &name, std::int64_t, std::string const &type = "");
};

struct Print : bridge::Interface<Print>
{
    using Base::Base;
    struct Methods
    {
        virtual JSValue print(JSContext *ctx, bridge::Array) const = 0;
        virtual ~Methods() = default;
    };
};

struct Storage : bridge::Interface<Storage>
{
    using Base::Base;
    class type
    {
    public:
        type(std::string &&);

        JSValue key(JSContext *, bridge::Number const &) const;

        void set(JSContext *, bridge::String &, bridge::String &);
        void set(JSContext *, bridge::String &, bridge::Value &);

        JSValue get(JSContext *, bridge::String &);

        void remove(JSContext *, bridge::String &);
        void remove(JSContext *) const;

        std::uint64_t count(JSContext *) const;

    private:
        std::string name;
    };
    struct Methods
    {
        virtual type &get() const = 0;
        virtual ~Methods() = default;
    };
};

struct URL : bridge::Interface<URL>
{
    using Base::Base;
    struct Methods
    {
        virtual std::string href() const = 0;
        virtual ~Methods() = default;
    };

    static JSValue make(JSContext *ctx, boost::urls::url &&);
    static std::optional<boost::urls::url> parse(char const *);
};

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

JSValue version(JSContext *,
    char const *);

} // namespace notojs::NOTOJS_VERSION_NS

#ifndef NOTOJS_INTERNAL_MODULE
namespace noto = notojs::NOTOJS_VERSION_NS;
#endif
