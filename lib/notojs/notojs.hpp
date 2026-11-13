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

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

struct Image : bridge::Content<Image>
{
    using Base::Base;
    void *fragment{nullptr};

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

    JSValue toJSON(JSContext *ctx) const;
    JSValue viewbox(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[2];
};

struct Version : bridge::Interface<Version, std::string>
{
    JSValue toString(JSContext *ctx);
    std::vector<std::string> own_properties() const;

    JSValue major(JSContext *ctx) const;
    JSValue minor(JSContext *ctx) const;
    JSValue patch(JSContext *ctx) const;

    using ctor = bridge::Unconstructable<Version>;
    static JSCFunctionListEntry const funcs[5];
    static JSClassExoticMethods exoticMethods;
};

struct XML : bridge::Content<XML>
{
    using Base::Base;

    JSValue toJSON(JSContext *ctx) const;
    static JSCFunctionListEntry const funcs[1];
};

} // namespace notojs
