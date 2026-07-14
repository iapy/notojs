#include <notojs/notojs.hpp>
#include <sstream>

namespace notojs {

JSValue HTML::toJSON(JSContext *ctx) const
{
    if(auto p = get<bridge::Boolean>(".json"); p && !*p)
        return JS_ThrowTypeError(ctx, "HTML cannot be serialized");

    bridge::Object res{ctx};
    res.set("type", bridge::String(ctx, std::string_view{"notojs.HTML"}));
    res.set("data", (*this)["data"].release());
    return res;
}

JSCFunctionListEntry const HTML::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<HTML>::toJSON),
};

JSValue Image::toJSON(JSContext *ctx) const
{
    bridge::Object res{ctx};
    res.set("type", bridge::String(ctx, std::string_view{"notojs.Image"}));
    res.set("data", (*this)["data"].release());
    return res;
}

JSCFunctionListEntry const Image::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Image>::toJSON),
};

JSValue __Markdown::toJSON(JSContext *ctx) const
{
    bridge::Object res{ctx};
    res.set("type", bridge::String(ctx, std::string_view{"notojs.Markdown"}));
    res.set("data", (*this)["data"].release());
    return res;
}

JSCFunctionListEntry const __Markdown::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<__Markdown>::toJSON),
};

JSValue SVG::toJSON(JSContext *ctx) const
{
    bridge::Object res{ctx};
    res.set("type", bridge::String(ctx, std::string_view{"notojs.HTML"}));
    res.set("data", (*this)["data"].release());
    return res;
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
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<SVG>::toJSON),
};

JSValue XML::toJSON(JSContext *ctx) const
{
    bridge::Object res{ctx};
    res.set("type", bridge::String(ctx, std::string_view{"notojs.XML"}));
    res.set("data", (*this)["data"].release());
    return res;
}

JSCFunctionListEntry const XML::funcs[1] = {
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<XML>::toJSON)
};

} // namespace notojs
