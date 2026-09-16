#include <notojs/notojs.hpp>
#include <cstdint>
#include <sstream>

namespace notojs {
namespace {

std::uint32_t fnv1a(std::uint32_t hash, std::string_view string)
{
    for(unsigned char byte : string)
        hash = (hash ^ byte) * 16777619u;
    return hash;
}

BOOST_FORCEINLINE JSValue content(JSContext *ctx, std::string_view &&type, bridge::Object const &content)
{
    auto data = content["data"];
    if(JS_IsException(data)) return data.release();
    return Content::make(ctx, std::move(type), data.release());
}

} // namespace

JSValue Content::make(JSContext *ctx, std::string_view type, JSValue data)
{
    std::uint32_t hash = fnv1a(2166136261u, type);
    if(JS_IsString(data))
    {
        std::size_t length;
        char const *string = JS_ToCStringLen(ctx, &length, data);
        if(!string)
        {
            JS_FreeValue(ctx, data);
            return JS_EXCEPTION;
        }
        hash = fnv1a(hash * 16777619u, std::string_view{string, length});
        JS_FreeCString(ctx, string);
    }

    char sign[8];
    constexpr char digits[] = "0123456789abcdef";
    for(std::size_t i = 0; i < sizeof(sign); ++i)
    {
        sign[sizeof(sign) - i - 1] = digits[hash & 15];
        hash >>= 4;
    }

    bridge::Object res{ctx};
    res.set("data", data);
    res.set("type", bridge::String{ctx, type});
    res.set("sign", bridge::String{ctx, std::string_view{sign, sizeof(sign)}});
    return res;
}

JSValue HTML::toJSON(JSContext *ctx) const
{
    if(auto p = get<bridge::Boolean>(".json"); p && !*p)
        return JS_ThrowTypeError(ctx, "HTML cannot be serialized");
    return content(ctx, "notojs.HTML", *this);
}

JSCFunctionListEntry const HTML::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&HTML::toJSON>::invoke),
};

JSValue Image::toJSON(JSContext *ctx) const
{
    return content(ctx, "notojs.Image", *this);
}

JSCFunctionListEntry const Image::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&Image::toJSON>::invoke),
};

JSValue Markdown::toJSON(JSContext *ctx) const
{
    return content(ctx, "notojs.Markdown", *this);
}

JSCFunctionListEntry const Markdown::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&Markdown::toJSON>::invoke),
};

JSValue SVG::toJSON(JSContext *ctx) const
{
    return content(ctx, "notojs.HTML", *this);
}

JSValue SVG::viewbox(JSContext *ctx) const
{
    bridge::Array arr{ctx};
    if(auto data = get<bridge::String>("data"); data)
    {
        auto sv = static_cast<std::string_view const>(*data);

        auto pos = sv.find("viewBox");
        if (pos == std::string_view::npos) return arr;

        pos = sv.find('=', pos);
        if (pos == std::string_view::npos) return arr;

        pos = sv.find('"', pos);
        if (pos == std::string_view::npos) return arr;

        std::istringstream ss{sv.data() + pos + 1};
        for(std::size_t i = 0; i < 4; ++i)
        {
            double value;
            ss >> value;
            arr.append(bridge::Number{ctx, JS_NewFloat64(ctx, value)});
        }
    }
    return arr;
}

JSCFunctionListEntry const SVG::funcs[2] = {
    JS_CGETSET_DEF("viewbox", &bridge::Getter<&SVG::viewbox>, NULL),
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&SVG::toJSON>::invoke),
};

JSValue XML::toJSON(JSContext *ctx) const
{
    return content(ctx, "notojs.XML", *this);
}

JSCFunctionListEntry const XML::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&XML::toJSON>::invoke)
};

} // namespace notojs
