#include <notojs/module/noto.hpp>
#include <notojs/detail/cellid.hpp>
#include <notojs/global.hpp>
#include <notojs/folder.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <rapidjson/document.h>
#include <bridge.hpp>
#include <global.hpp>
#include <fstream>

namespace notojs {
namespace {

namespace noto {
    struct Config;
} // namespace noto

detail::Config const *cfg{nullptr};

struct noto::Config : bridge::Interface<Config, std::string>
{
    JSValue toJSON(JSContext *ctx) const
    {
        if(auto sub = cfg->get_child_optional(ref()))
        {
            std::ostringstream oss;
            boost::property_tree::write_json(oss, *sub);

            std::string const data = oss.str();
            return JS_ParseJSON(ctx, data.c_str(), data.size(), "<config>");
        }
        return JS_NULL;
    }

    JSValue toString(JSContext *ctx)
    {
        if(ref().empty())
        {
            std::ifstream ifs{*cfg->source};
            return bridge::String{ctx, std::string{
                std::istreambuf_iterator<char>(ifs),
                std::istreambuf_iterator<char>()
            }};
        }
        return JS_ThrowTypeError(ctx, "toString() works only on top-level config");
    }

    JSValue get_property(JSContext *ctx, char const *n) const
    {
        std::string path{n};
        if(!ref().empty())
        {
            path.insert(0, ref());
            path.insert(ref().size(), ".");
        }
        if(auto sub = cfg->get_child_optional(path))
        {
            if(sub->empty()) return bridge::String{ctx, sub->get_value<std::string>()};
            return Config::from(ctx, std::move(path));
        }
        return JS_UNDEFINED;
    }

    using ctor = bridge::Unconstructable<Config>;
    static JSCFunctionListEntry const funcs[];
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods noto::Config::exoticMethods = {
    .get_property = &bridge::get_property<Config>
};

JSCFunctionListEntry const noto::Config::funcs[] = {
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&Config::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Config>::toJSON)
};

struct Cell : bridge::Interface<Cell, std::pair<std::string, bridge::Array>>
{
    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            for(std::uint32_t j = 0; j < ref.second.size(); ++j)
            {
                if(auto obj = ref.second.at<bridge::Object>(j))
                {
                    if(auto type = obj->get<bridge::String>("type"); !type)
                    {
                        continue;
                    }
                    else if(auto const &types = static_cast<std::string_view>(*type); "notojs.Output" == types)
                    {
                        if(auto data = obj->get<bridge::Array>("data"); data)
                        {
                            for(std::uint32_t k = 0; k < data->size(); ++k)
                            {
                                if(auto row = data->at<bridge::Array>(k))
                                {
                                    bridge::Array{ctx, output}.append(row->release());
                                }
                                else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d:%d", ref.first.c_str(), j, k);
                            }
                        }
                        else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d", ref.first.c_str(), j);
                    }
                    else if("notojs.Render" == types)
                    {
                        if(auto data = obj->get<bridge::Array>("data"); data)
                        {
                            for(std::uint32_t k = 0; k < data->size(); ++k)
                            {
                                if(auto r = data->at<bridge::String>(k))
                                {
                                    Global::Context::ptr(ctx)->renderers.insert(static_cast<std::string>(*r));
                                }
                                else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d:%d", ref.first.c_str(), j, k);
                            }
                        }
                        else return JS_ThrowTypeError(ctx, "Ivalid data at %s:%d", ref.first.c_str(), j);
                    }
                    else return JS_ThrowTypeError(ctx, "Invalid output type [%s] at %s:%d", types.data(), ref.first.c_str(), j);
                }
                else return JS_ThrowTypeError(ctx, "Invalid type at %s:%d", ref.first.c_str(), j);
            }
            return JS_UNDEFINED;
        }
    };

    using Base::get;
    using ctor = bridge::Unconstructable<Cell>;
    using impl = bridge::Implements<I>;
};

struct Output : bridge::Interface<Output, bridge::Array>
{
    using Base::Base;

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number{ctx, ref().size()};
    }

    JSValue get_property(JSContext *ctx, const char *name) const
    {
        if(!name || *name == '\0') return JS_UNDEFINED;

        char *end;
        std::uint32_t i = std::strtoul(name, &end, 10);
        if(end == name || *end != '\0' || errno == ERANGE)
            return JS_UNDEFINED;

        if(auto c = ref().at<bridge::String>(i))
        {
            auto const json = static_cast<std::string_view>(*c);
            if(bridge::Strong<void> j{ctx, JS_ParseJSON(ctx, json.data(), json.size(), name), false}; bridge::Error::check(ctx, j))
            {
                return j.release();
            }
            else if(!bridge::Array::check(ctx, j))
            {
                return JS_ThrowTypeError(ctx, "Expecting Array at index %s", name);
            }
            else
            {
                return Cell::from(ctx, {detail::cell_id(i), bridge::Array{ctx, j}}, j);
            }
        }
        else if(i < ref().size()) JS_ThrowTypeError(ctx, "Expecting String at index %s", name);
        return JS_UNDEFINED;
    }

    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            for(std::uint32_t i = 0; i < ref.size(); ++i)
            {
                auto name = detail::cell_id(i);
                if(auto v = ref.at<bridge::String>(i))
                {
                    auto const json = static_cast<std::string_view>(*v);
                    if(bridge::Strong<void> j{ctx, JS_ParseJSON(ctx, json.data(), json.size(), name.c_str()), false}; bridge::Error::check(ctx, j))
                    {
                        return j.release();
                    }
                    else if(!bridge::Array::check(ctx, j))
                    {
                        return JS_ThrowTypeError(ctx, "Expecting Array at index %d", i);
                    }
                    else
                    {
                        Cell::Wrapped w{std::move(name), bridge::Array{ctx, j}};
                        if(bridge::Strong<void> r{ctx, Cell::I{w}.print(ctx, output), false}; bridge::Error::check(ctx, r))
                            return r.release();
                    }
                }
                else return JS_ThrowTypeError(ctx, "Expecting String at index %d", i);
            }
            return JS_UNDEFINED;
        }
    };

    using Base::get;
    using ctor = bridge::Unconstructable<Output>;
    using impl = bridge::Implements<I>;
    static JSClassExoticMethods exoticMethods;
    static JSCFunctionListEntry const funcs[];
};

JSClassExoticMethods Output::exoticMethods = {
    .get_property = &bridge::get_property<Output>
};

JSCFunctionListEntry const Output::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&Output::length>, NULL)
};

struct Notebook_
{
    std::string const name;

    JSValue saved(JSContext *ctx)
    {
        auto url = facade::URL::parse(("noto:/r/" + name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{
            boost::beast::http::verb::get,
            url->path(),
            11
        };

        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    static JSValue response(JSContext *ctx, JSValue resp, boost::beast::http::response<boost::beast::http::string_body> const &response)
    {
        if(boost::beast::http::status::ok == response.result())
        {
            bridge::Strong<void> result{ctx, JS_ParseJSON(ctx, response.body().data(), response.body().size(), "<output>"), false};

            if(bridge::Error::check(ctx, +result)) return result.release();
            if(bridge::Array::check(ctx, +result)) return Output::from(ctx, bridge::Array{ctx, result}, result);

            return JS_ThrowTypeError(ctx, "Expecting Array");
        }
        return JS_ThrowInternalError(ctx, "HTTP status code %d", response.result_int());
    }

    static JSValue print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int, JSValue *output)
    {
        return Output::I{Output::get(*argv)}.print(ctx, bridge::Array{ctx, *output});
    }
};

struct Notebook : bridge::Interface<Notebook, Notebook_>
{
    JSValue saved(JSContext *ctx)
    {
        return ref().saved(ctx);
    }

    template<boost::beast::http::verb verb>
    JSValue execute_0(JSContext *ctx)
    {
        auto url = facade::URL::parse(("noto:/r/" + ref().name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{verb, url->path(), 11};
        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    template<boost::beast::http::verb verb>
    JSValue execute_1(JSContext *ctx, bridge::String input)
    {
        auto url = facade::URL::parse(("noto:/r/" + ref().name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{verb, url->path(), 11};
        request.body() = static_cast<std::string_view>(input);

        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    template<boost::beast::http::verb verb>
    JSValue execute_2(JSContext *ctx, bridge::Value input)
    {
        auto url = facade::URL::parse(("noto:/r/" + ref().name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{verb, url->path(), 11};
        request.body() = static_cast<std::string_view>(input.json());
        request.set(boost::beast::http::field::content_type, "application/json");

        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    using execute = bridge::Function
    <
        &Notebook::execute_0<boost::beast::http::verb::post>,
        &Notebook::execute_1<boost::beast::http::verb::post>,
        &Notebook::execute_2<boost::beast::http::verb::post>
    >;

    using update = bridge::Function
    <
        &Notebook::execute_0<boost::beast::http::verb::put>,
        &Notebook::execute_1<boost::beast::http::verb::put>,
        &Notebook::execute_2<boost::beast::http::verb::put>
    >;

    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            return bridge::Strong<bridge::Promise>{ctx, ref.saved(ctx)}.wrap(
                &Notebook_::print,
                [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
                    return JS_Throw(ctx, JS_DupValue(ctx, argv[0]));
                },
                1, +output
            ).release();
        }
    };

    using ctor = bridge::Unconstructable<Notebook>;
    using impl = bridge::Implements<I>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Notebook::funcs[] = {
    JS_CFUNC_DEF("saved", 0, &bridge::Function<&Notebook::saved>::invoke),
    JS_CFUNC_DEF("update", 1, &Notebook::update::invoke),
    JS_CFUNC_DEF("execute", 1, &Notebook::execute::invoke)
};

JSValue notebook(JSContext *ctx, bridge::String name)
{
    return Notebook::from(ctx, Notebook_{name});
}

JSValue application_0(JSContext *ctx, bridge::String name)
{
    return bridge::String{ctx, "noto:/a/" + static_cast<std::string>(name) + "/"};
}

JSValue application_1(JSContext *ctx, bridge::String name, bridge::String path)
{
    auto const p = static_cast<std::string>(path);
    if(p.empty() || p[0] != '/') return JS_ThrowSyntaxError(ctx, "Path should be absolute");
    return bridge::String{ctx, "noto:/a/" + static_cast<std::string>(name) + static_cast<std::string>(path)};
}

using application = bridge::Function<&application_0, &application_1>;

JSValue packages_0(JSContext *ctx)
{
    if(std::string data; boost::beast::http::status::ok != Global::ptr(ctx)->get<Folder>().get_packages(data))
        return JS_ThrowInternalError(ctx, "Could not load packages config");
    else
        return bridge::String(ctx, std::move(data));
}

JSValue packages_1(JSContext *ctx, bridge::String config)
{
    if(std::string data = config; boost::beast::http::status::ok != Global::ptr(ctx)->get<Folder>().set_packages(data))
        return JS_ThrowInternalError(ctx, "%s", data.c_str());
    else
        return JS_UNDEFINED;
}

using packages = bridge::Function<&packages_0, &packages_1>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("application", 0, application::invoke),
    JS_CFUNC_DEF("notebook", 0, &bridge::Function<&notebook>::invoke),
    JS_CFUNC_DEF("packages", 0, &packages::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    if(cfg)
    {
        noto::Config::init(ctx, m);
        JS_SetModuleExport(ctx, m, "config", noto::Config::from(ctx, ""));
    }
    Notebook::init(ctx, m);
    Output::init(ctx, m);
    Cell::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_noto()
{
    if(cfg) noto::Config::init();
    Notebook::init();
    Output::init();
    Cell::init();
}

void notojs_init_noto(JSRuntime *rt)
{
    if(cfg) noto::Config::init(rt);
    Notebook::init(rt);
    Output::init(rt);
    Cell::init(rt);
}

void notojs_init_noto(detail::Config const &cfg)
{
    notojs::cfg = &cfg;
}

JSModuleDef *notojs_init_noto(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    if(cfg) JS_AddModuleExport(ctx, mod, "config");
    JS_AddModuleExport(ctx, mod, noto::Config::name());
    JS_AddModuleExport(ctx, mod, Notebook::name());
    JS_AddModuleExport(ctx, mod, Output::name());
    JS_AddModuleExport(ctx, mod, Cell::name());
    return mod;
}

} // namespace notojs
