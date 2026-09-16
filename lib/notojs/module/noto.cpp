#include <notojs/module/noto.hpp>
#include <notojs/detail/cellid.hpp>
#include <notojs/global.hpp>
#include <notojs/folder.hpp>
#include <notojs/notojs.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <rapidjson/document.h>
#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>
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
    JS_CFUNC_DEF("toJSON", 0, &bridge::Function<&Config::toJSON>::invoke)
};

struct Cell : bridge::Interface<Cell, bridge::Object>
{
    using Base::Base;

    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            auto name = ref.get<bridge::String>("name");
            auto records = ref.get<bridge::Array>("data");
            if(!name || !records) return JS_ThrowTypeError(ctx, "Invalid Cell name or data");
            auto const id = static_cast<std::string>(*name);

            for(std::uint32_t j = 0; j < records->size(); ++j)
            {
                if(auto obj = records->at<bridge::Object>(j))
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
                                else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d:%d", id.c_str(), j, k);
                            }
                        }
                        else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d", id.c_str(), j);
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
                                else return JS_ThrowTypeError(ctx, "Invalid data at %s:%d:%d", id.c_str(), j, k);
                            }
                        }
                        else return JS_ThrowTypeError(ctx, "Ivalid data at %s:%d", id.c_str(), j);
                    }
                    else return JS_ThrowTypeError(ctx, "Invalid output type [%s] at %s:%d", types.data(), id.c_str(), j);
                }
                else return JS_ThrowTypeError(ctx, "Invalid type at %s:%d", id.c_str(), j);
            }
            return JS_UNDEFINED;
        }
    };

    using impl = bridge::Implements<I>;
    static constexpr bool constructible = false;
};

struct Output : bridge::Interface<Output, bridge::Object>
{
    using Base::Base;

    std::uint32_t size() const
    {
        auto length = get<bridge::Number>("length");
        return length ? static_cast<std::int64_t>(*length) : 0;
    }

    bool own_property(JSContext *ctx, char const *n, JSPropertyDescriptor *desc)
    {
        std::string key{"cell-"};
        for(char const *p = n; *p; ++p)
        {
            if(*p < '0' || *p > '9' || key.size() > 8 || '0' == key.back()) return false;
            key.append(p, 1);
        }
        while(key.size() != 8) key.insert(5, "0");

        JSAtom cell = JS_NewAtom(ctx, key.c_str());
        if(!cell) return false;

        int result = JS_GetOwnProperty(ctx, desc, value, cell);
        JS_FreeAtom(ctx, cell);

        if(result > 0 && desc) desc->flags &= ~JS_PROP_ENUMERABLE;
        return result > 0;
    }

    struct Values
    {
        JSValue output;
        std::uint32_t index;

        JSValue get(JSContext *ctx) const
        {
            return JS_GetPropertyUint32(ctx, output, index);
        }
        Values &operator ++ ()
        {
            ++index;
            return *this;
        }
        bool operator == (Values const &other) const
        {
            return index == other.index;
        }
    };

    JSValue values(JSContext *ctx) const
    {
        return bridge::Iterator<Values>::make(ctx, value, Values{value, 0}, Values{value, size()});
    }

    JSValue each_1(JSContext *ctx, bridge::Lambda lambda) const
    {
        return each_2(ctx, lambda, bridge::Value{ctx, JS_UNDEFINED});
    }

    JSValue each_2(JSContext *ctx, bridge::Lambda lambda, bridge::Value self) const
    {
        auto const length = size();
        for(std::uint32_t i = 0; i < length; ++i)
        {
            if(bridge::Strong<void> cell{ctx, JS_GetPropertyUint32(ctx, value, i), false}; JS_IsException(cell))
                return cell.release();
            else if(auto result = lambda(self, std::array<JSValue, 3>{cell, JS_NewUint32(ctx, i), value}); JS_IsException(result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            Output source{ctx, ref};
            std::uint32_t const length = source.size();
            for(std::uint32_t i = 0; i < length; ++i)
            {
                if(bridge::Strong<void> cell{ctx, JS_GetPropertyUint32(ctx, ref, i), false}; JS_IsException(cell))
                    return cell.release();
                else if(!Cell::check(ctx, +cell))
                    return JS_ThrowTypeError(ctx, "Expecting Cell at index %u", i);
                else if(bridge::Strong<void> result{ctx, Cell::I{ctx, cell}.print(ctx, output), false}; JS_IsException(result))
                    return result.release();
            }
            return JS_UNDEFINED;
        }
    };

    using each = bridge::Function<&Output::each_1, &Output::each_2>;
    using priv = bridge::Private<bridge::Iterator<Values>>;
    using impl = bridge::Implements<I>;

    static constexpr bool constructible = false;
    static JSClassExoticMethods exoticMethods;
    static JSCFunctionListEntry const funcs[];
};

JSClassExoticMethods Output::exoticMethods = {
    .get_own_property =  &bridge::own_property<Output>,
};

JSCFunctionListEntry const Output::funcs[] = {
    JS_CFUNC_DEF("values", 0, &bridge::Function<&Output::values>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&Output::values>::invoke),
    JS_CFUNC_DEF("forEach", 1, &Output::each::invoke)
};

struct Notebook_
{
    std::string const name;

    JSValue load(JSContext *ctx, bool html = false)
    {
        auto url = facade::URL::parse(("noto:/r/" + name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{
            boost::beast::http::verb::get,
            url->path(),
            11
        };
        if(html) request.set(boost::beast::http::field::accept, "text/html");

        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    static JSValue response(JSContext *ctx, JSValue resp, boost::beast::http::response<boost::beast::http::string_body> const &response)
    {
        if(boost::beast::http::status::ok == response.result())
        {
            if(auto const type = response[boost::beast::http::field::content_type]; "application/json" == type)
            {
                bridge::Strong<void> result{ctx, JS_ParseJSON(ctx, response.body().data(), response.body().size(), "<output>"), false};

                if(JS_IsException(result)) return result.release();
                if(!bridge::Array::check(ctx, +result)) return JS_ThrowTypeError(ctx, "Expecting Array");

                bridge::Strong<void> output{ctx, Output::ctor(ctx), false};
                if(JS_IsException(output)) return output.release();

                bridge::Array cells{ctx, result};

                auto const length = cells.size();
                if(length > 1000) return JS_ThrowRangeError(ctx, "Too many cells for canonical cell IDs");

                for(std::uint32_t i = 0; i < length; ++i)
                {
                    auto cell = cells.at<bridge::String>(i);
                    if(!cell) return JS_ThrowTypeError(ctx, "Expecting String at %ul", i);

                    auto const name = detail::cell_id(i);
                    auto const json = static_cast<std::string_view>(*cell);

                    bridge::Strong<void> data{ctx, JS_ParseJSON(ctx, json.data(), json.size(), name.c_str()), false};
                    if(JS_IsException(data)) return data.release();

                    if(!bridge::Array::check(ctx, +data)) return JS_ThrowTypeError(ctx, "Expecting Array at %ul", i);

                    bridge::Strong<void> value{ctx, Cell::ctor(ctx), false};
                    if(JS_IsException(value)) return value.release();

                    if(JS_DefinePropertyValueStr(ctx, value, "name", JS_NewString(ctx, name.c_str()), JS_PROP_ENUMERABLE) < 0
                        || JS_DefinePropertyValueStr(ctx, value, "data", data.release(), JS_PROP_ENUMERABLE) < 0)
                        return JS_EXCEPTION;

                    if(JS_DefinePropertyValueStr(ctx, output, name.c_str(), value.release(), JS_PROP_C_W_E) < 0) return JS_EXCEPTION;
                }
                if(JS_DefinePropertyValueStr(ctx, output, "length", JS_NewUint32(ctx, length), 0) < 0) return JS_EXCEPTION;
                return output.release();
            }
            else if("text/html" == type)
            {
                return core::facade::html(ctx, response.body(), false);
            }
            return JS_NULL;
        }
        return JS_ThrowInternalError(ctx, "HTTP status code %d", response.result_int());
    }

    static JSValue print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int, JSValue *output)
    {
        return Output::I{ctx, *argv}.print(ctx, bridge::Array{ctx, *output});
    }
};

struct Notebook : bridge::Interface<Notebook, Notebook_>
{
    struct Format : bridge::detail::Reference
    {
        BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
        {
            if(HTML::ctor(ctx, *value)) return true;

            bridge::Strong<bridge::Object> glob(ctx, JS_GetGlobalObject(ctx));
            auto json = glob.get<bridge::Object>("JSON");

            return json
                && JS_VALUE_GET_TAG(*value) == JS_TAG_OBJECT
                && JS_VALUE_GET_PTR(*value) == JS_VALUE_GET_PTR(static_cast<JSValue>(*json));
        }
    };

    struct ExecOptions : bridge::Struct<ExecOptions>
    {
        BRIDGE_DEFINE_STRUCT(ExecOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Either<bridge::Null, Format>>("result"),
            bridge::field<bridge::Value>("input"),
            bridge::field<bridge::Boolean>("update")
        );
    };

    JSValue exec_0(JSContext *ctx)
    {
        auto url = facade::URL::parse(("noto:/r/" + ref().name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::post, url->path(), 11};
        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    JSValue exec_1(JSContext *ctx, ExecOptions opts)
    {
        auto url = facade::URL::parse(("noto:/r/" + ref().name + ".notojs").c_str());
        if(!url) return JS_ThrowInternalError(ctx, "Cannot parse URL");

        auto verb = boost::beast::http::verb::post;
        if(auto u = opts.get<bridge::Boolean>("update"); u && *u)
            verb = boost::beast::http::verb::put;

        boost::beast::http::request<boost::beast::http::string_body> request{verb, url->path(), 11};
        if(auto s = opts.get<bridge::String>("input"))
        {
            request.body() = static_cast<std::string>(*s);
        }
        else if(auto v = opts.get<bridge::Value>("input"))
        {
            request.set(boost::beast::http::field::content_type, "application/json");
            request.body() = static_cast<std::string>(v->json());
        }
        if(opts.get<bridge::Null>("result"))
            request.set(boost::beast::http::field::prefer, "return=minimal");
        else if(auto v = opts.get<bridge::Value>("result"); v && HTML::ctor(ctx, *v))
            request.set(boost::beast::http::field::accept, "text/html");
        return facade::fetch(ctx, std::move(request), std::move(*url), &Notebook_::response);
    }

    using exec = bridge::Function<&Notebook::exec_0, &Notebook::exec_1>;

    struct LoadOptions : bridge::Struct<LoadOptions>
    {
        BRIDGE_DEFINE_STRUCT(LoadOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<Format>("result")
        );
    };

    JSValue load_0(JSContext *ctx)
    {
        return ref().load(ctx);
    }

    JSValue load_1(JSContext *ctx, LoadOptions opts)
    {
        auto result = opts.get<bridge::Value>("result");
        return ref().load(ctx, result && HTML::ctor(ctx, *result));
    }

    using load = bridge::Function<&Notebook::load_0, &Notebook::load_1>;

    struct I : Base::I<I, IPrint>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array output) const
        {
            return bridge::Strong<bridge::Promise>{ctx, ref.load(ctx)}.wrap(
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
    JS_CFUNC_DEF("exec", 0, &Notebook::exec::invoke),
    JS_CFUNC_DEF("load", 0, &Notebook::load::invoke)
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
    JS_CFUNC_DEF("application", 1, application::invoke),
    JS_CFUNC_DEF("notebook", 1, &bridge::Function<&notebook>::invoke),
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
