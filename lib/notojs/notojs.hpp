#pragma once
#include <notojs/detail/bridge.hpp>

namespace notojs {

struct Content
{
    Content() = delete;
    static JSValue make(JSContext *ctx, std::string_view, JSValue);
};

struct HTML : bridge::Content<HTML>
{
    using Base::Base;

    void *fragment{nullptr};

    struct Interface : bridge::Interface<Interface>
    {
        virtual bool json() const = 0;
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct Image : bridge::Content<Image>
{
    using Base::Base;

    void *fragment{nullptr};

    struct Interface : bridge::Interface<Interface>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct Markdown : bridge::Content<Markdown>
{
    using Base::Base;

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct SVG : bridge::Content<SVG>
{
    using Base::Base;

    void *fragment{nullptr};

    struct Interface : bridge::Interface<Interface>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    JSValue viewbox(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[2];
};

struct XML : bridge::Content<XML>
{
    using Base::Base;

    struct Interface : bridge::Interface<Interface>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

} // namespace notojs
