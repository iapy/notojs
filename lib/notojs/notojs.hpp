#pragma once
#include <bridge.hpp>

namespace notojs {

struct HTML : bridge::Interface<HTML>
{
    using Base::Base;
    static constexpr bool constructible = false;

    struct Interface : bridge::Interface<Interface, void*>
    {
        virtual bool json() const = 0;
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct Image : bridge::Interface<Image>
{
    using Base::Base;
    static constexpr bool constructible = false;

    struct Interface : bridge::Interface<Interface, void*>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct __Markdown : bridge::Interface<__Markdown>
{
    using Base::Base;
    static constexpr bool constructible = false;

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct SVG : bridge::Interface<SVG>
{
    using Base::Base;
    static constexpr bool constructible = false;

    struct Interface : bridge::Interface<Interface, void*>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    JSValue viewbox(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[2];
};

struct XML : bridge::Interface<XML>
{
    using Base::Base;
    static constexpr bool constructible = false;

    struct Interface : bridge::Interface<Interface, void*>
    {
        virtual std::string get() const = 0;
        virtual ~Interface() {}
    };

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

} // namespace notojs
