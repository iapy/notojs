#include "quickjs/quickjs.h"
#include <notojs/config.hpp>
#include <notojs/global.hpp>
#include <notojs/folder.hpp>
#include <notojs/logger.hpp>
#include <notojs/module.hpp>
#include <notojs/notojs.hpp>
#include <notojs/server.hpp>
#include <notojs/socket.hpp>

#include <notojs/detail/config.hpp>
#include <notojs/detail/header.hpp>
#include <notojs/detail/jscode.hpp>
#include <notojs/parser/search.hpp>
#include <notojs/script/dollar.hpp>
#include <notojs/script/console.hpp>
#include <bridge.hpp>
#include <engine.hpp>
#include <global.hpp>
#include <notodb.hpp>

#include <rapidjson/rapidjson.h>
#include <boost/asio/ssl/stream.hpp>

#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace notojs {
extern const std::string_view MUSTACHE_JS;

namespace {

struct Blob_
{
    using Data = std::shared_ptr<std::vector<std::uint8_t>>;

    std::string type;
    Data data;

    std::size_t size;
    std::uint8_t const *dptr;

    BOOST_FORCEINLINE static Data from()
    {
        return std::make_shared<Data::element_type>();
    }
    BOOST_FORCEINLINE static Data from(Data::element_type &&data)
    {
        return std::make_shared<Data::element_type>(std::move(data));
    }
};

struct Blob : bridge::Interface<Blob, Blob_>
{
    struct Parts : bridge::Array
    {
        using bridge::Array::Array;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            Parts p(ctx, *value);
            for(std::size_t i = 0; i < p.size(); ++i)
            {
                if(!p.at<bridge::String>(i) && !p.at<bridge::ArrayBuffer>(i))
                {
                    message.append("invalid type [");
                    message.append(std::to_string(i));
                    message.append("]");
                    return false;
                }
            }
            return true;
        }
    };

    struct Options : bridge::Struct<Options>
    {
        BRIDGE_DEFINE_STRUCT(Options);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::String>("type")
        );
    };

    Blob(Parts parts)
    : bridge::Interface<Blob, Blob_>(Blob_{})
    {
        ref().data = Blob_::from();
        for(std::size_t i = 0; i < parts.size(); ++i)
        {
            if(auto s = parts.at<bridge::String>(i); s)
            {
                ref().data->insert(std::end(*ref().data), s->begin(), s->end());
            }
            else if(auto a = parts.at<bridge::ArrayBuffer>(i); a)
            {
                auto const [d, s] = a->data();
                ref().data->insert(std::end(*ref().data), d, d + s);
            }
        }
        ref().dptr = ref().data->empty() ? nullptr : &ref().data->at(0);
        ref().size = ref().data->size();
    }

    Blob(Parts parts, Options options)
    : Base(Blob_{})
    {
        ref().data = Blob_::from();
        for(std::size_t i = 0; i < parts.size(); ++i)
        {
            if(auto s = parts.at<bridge::String>(i); s)
            {
                ref().data->insert(std::end(*ref().data), s->begin(), s->end());
            }
            else if(auto a = parts.at<bridge::ArrayBuffer>(i); a)
            {
                auto const [d, s] = a->data();
                ref().data->insert(std::end(*ref().data), d, d + s);
            }
        }
        if(auto type = options.get<bridge::String>("type"); type)
        {
            ref().type = static_cast<std::string>(*type);
        }
        ref().dptr = ref().data->empty() ? nullptr : &ref().data->at(0);
        ref().size = ref().data->size();
    }

    Blob(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    JSValue get_size(JSContext *ctx) const
    {
        if constexpr (sizeof(std::size_t) == sizeof(std::uint32_t))
            return bridge::Number(ctx, static_cast<std::uint32_t>(ref().size));
        else
            return bridge::Number(ctx, static_cast<std::uint64_t>(ref().size));
    }

    JSValue get_type(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().type);
    }

    void set_type(JSContext *ctx, bridge::String type)
    {
        ref().type = type;
    }

    JSValue arrayBuffer(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(auto &r = ref(); !JS_IsException(promise))
        {
            auto blob = bridge::Strong<bridge::ArrayBuffer>(ctx, bridge::ArrayBuffer(ctx, r.dptr, r.size, self));
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, +blob));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue bytes(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(auto &r = ref(); !JS_IsException(promise))
        {
            auto blob = bridge::Strong<bridge::ArrayBuffer>(ctx, bridge::ArrayBuffer(ctx, r.dptr, r.size, self));
            JSValue args[3] = {blob, JS_NewFloat64(ctx, 0), JS_UNDEFINED};
            auto arr8 = JS_NewTypedArray(ctx, 3, &args[0], JS_TYPED_ARRAY_UINT8);
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &arr8));
            JS_FreeValue(ctx, arr8);
            JS_FreeValue(ctx, args[1]);
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue text(JSContext *ctx) const
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(auto const &r = ref(); !JS_IsException(promise))
        {
            auto text = bridge::Strong<bridge::String>(ctx, bridge::String(ctx, std::string_view{
                reinterpret_cast<const char *>(r.dptr), r.size
            }));
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, +text));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    using ctor = bridge::Constructor
    <
        Blob(Parts),
        Blob(Parts, Options)
    >;

    JSValue slice_0(JSValue self, JSContext *ctx)
    {
        return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = ref().size,
            .dptr = ref().dptr
        }, self);
    }

    JSValue slice_1(JSValue self, JSContext *ctx, bridge::Number ns)
    {
        std::int32_t s = static_cast<std::int32_t>(ns);
        if(s < 0) s += ref().size;

        if(s < 0 || s >= ref().size) return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = 0,
            .dptr = nullptr
        });
        else return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = ref().size - s,
            .dptr = ref().dptr + s
        }, self);
    }

    JSValue slice_2(JSValue self, JSContext *ctx, bridge::Number ns, bridge::Number ne)
    {
        std::int32_t s = static_cast<std::int32_t>(ns);
        if(s < 0) s += ref().size;

        std::int32_t e = static_cast<std::int32_t>(ne);
        if(e < 0) e += ref().size;
        if(e > ref().size) e = ref().size;

        if(s < 0 || s >= e) return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = 0,
            .dptr = nullptr
        });
        else return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = static_cast<std::size_t>(e - s),
            .dptr = ref().dptr + s
        }, self);
    }

    JSValue slice_3(JSValue self, JSContext *ctx, bridge::Number ns, bridge::Number ne, bridge::String tp)
    {
        std::int32_t s = static_cast<std::int32_t>(ns);
        if(s < 0) s += ref().size;

        std::int32_t e = static_cast<std::int32_t>(ne);
        if(e < 0) e += ref().size;
        if(e > ref().size) e = ref().size;

        if(s < 0 || s >= e) return Blob::from(ctx, Blob_{
            .data = nullptr,
            .size = 0,
            .dptr = nullptr
        });
        else return Blob::from(ctx, Blob_{
            .type = static_cast<std::string>(tp),
            .data = nullptr,
            .size = static_cast<std::size_t>(e - s),
            .dptr = ref().dptr + s
        }, self);
    }

    using slice = bridge::Function
    <
        &Blob::slice_0,
        &Blob::slice_1,
        &Blob::slice_2,
        &Blob::slice_3
    >;

    JSValue toJSON(JSContext *ctx) const
    {
        bridge::Object res{ctx};
        res.set("size", bridge::Number(ctx, static_cast<std::int64_t>(ref().size)));
        res.set("type", bridge::String(ctx, ref().type));
        return res;
    }

    struct I : Base::I<I, IBlob>
    {
        using Base::Base;

        std::string type() const override
        {
            return ref.type;
        }

        std::shared_ptr<std::vector<std::uint8_t>> copy() const override
        {
            if(ref.data) return ref.data;
            return std::make_shared<Blob_::Data::element_type>(ref.dptr, ref.dptr + ref.size);
        }

        std::pair<std::uint8_t const *, std::size_t> data() const override
        {
            return {ref.dptr, ref.size};
        }
    };

    using impl = bridge::Implements<I>;
    static JSCFunctionListEntry const funcs[];

    using Base::ref;
    using Base::get;
};

JSCFunctionListEntry const Blob::funcs[] = {
    JS_CGETSET_DEF("size", &bridge::Getter<&Blob::get_size>, NULL),
    JS_CGETSET_DEF("type", &bridge::Getter<&Blob::get_type>, &bridge::Setter<&Blob::set_type>),

    JS_CFUNC_DEF("arrayBuffer", 0, &bridge::Function<&Blob::arrayBuffer>::invoke),
    JS_CFUNC_DEF("bytes", 0, &bridge::Function<&Blob::bytes>::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&Blob::text>::invoke),
    JS_CFUNC_DEF("slice", 0, &Blob::slice::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Blob>::toJSON)
};

struct Headers : bridge::Interface<Headers, boost::beast::http::fields>
{
    Headers() = default;

    Headers(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Headers(bridge::Dict<bridge::String> dict)
    {
        dict.each([this](auto &&key, auto value){
            ref().set(key, static_cast<std::string_view const &>(value));
        });
    }

    JSValue get(JSContext *ctx, bridge::String key)
    {
        if(auto const it = ref().find(static_cast<std::string_view const &>(key)); it != std::end(ref()))
            return bridge::String(ctx, static_cast<std::string_view>(it->value()));
        return JS_NULL;
    }

    JSValue has(JSContext *ctx, bridge::String key)
    {
        return ref().find(static_cast<std::string_view const &>(key)) == std::end(ref()) ? JS_FALSE : JS_TRUE;
    }

    JSValue set(JSContext *ctx, bridge::String key, bridge::String val)
    {
        ref().set(static_cast<std::string_view const &>(key), static_cast<std::string_view const &>(val));
        return JS_UNDEFINED;
    }

    struct Keys : Wrapped::const_iterator
    {
        Keys(Wrapped::const_iterator &&base)
        : Wrapped::const_iterator{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::String(ctx, static_cast<std::string_view>((*this)->name_string()));
        }
        Keys &operator ++ ()
        {
            Wrapped::const_iterator::operator ++ ();
            return *this;
        }
    };

    JSValue keys(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Keys>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    struct Values : Wrapped::const_iterator
    {
        Values(Wrapped::const_iterator &&base)
        : Wrapped::const_iterator{base} {}

        JSValue get(JSContext *ctx) const
        {
            return bridge::String(ctx, static_cast<std::string_view>((*this)->value()));
        }
        Values &operator ++ ()
        {
            Wrapped::const_iterator::operator ++ ();
            return *this;
        }
    };

    JSValue values(JSValue self, JSContext *ctx) const
    {
        return bridge::Iterator<Values>::make(ctx, self, std::begin(ref()), std::end(ref()));
    }

    JSValue each_1(JSValue self, JSContext *ctx, bridge::Lambda lambda)
    {
        for(auto const &header : ref())
        {
            bridge::Strong<void> n{ctx, bridge::String(ctx, static_cast<std::string_view>(header.name_string())), false};
            bridge::Strong<void> v{ctx, bridge::String(ctx, static_cast<std::string_view>(header.value())), false};
            if(auto result = lambda(std::array<JSValue, 3>{n, v, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    JSValue each_2(JSValue self, JSContext *ctx, bridge::Lambda lambda, bridge::Value value)
    {
        for(auto const &header : ref())
        {
            bridge::Strong<void> n{ctx, bridge::String(ctx, static_cast<std::string_view>(header.name_string())), false};
            bridge::Strong<void> v{ctx, bridge::String(ctx, static_cast<std::string_view>(header.value())), false};
            if(auto result = lambda(value, std::array<JSValue, 3>{n, v, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    JSValue append(JSContext *ctx, bridge::String key, bridge::String val)
    {
        if(auto const it = ref().find(static_cast<std::string_view const &>(key)); it != std::end(ref()))
            ref().set(it->name_string(), std::string(it->value().data(), it->value().size()) + ", " + static_cast<std::string>(val));
        else
            ref().set(static_cast<std::string_view const &>(key), static_cast<std::string_view const &>(val));
        return JS_UNDEFINED;
    }

    JSValue remove(JSContext *ctx, bridge::String key)
    {
        ref().erase(static_cast<std::string_view const &>(key));
        return JS_UNDEFINED;
    }

    JSValue toJSON(JSContext *ctx) const
    {
        bridge::Object res{ctx};
        for(auto const &header : ref()) {
            std::string name = header.name_string();
            res.set(name.c_str(), bridge::String(ctx, static_cast<std::string_view>(header.value())));
        }
        return res;
    }

    using ctor = bridge::Constructor
    <
        Headers(),
        Headers(bridge::Dict<bridge::String>)
    >;

    using priv = bridge::Private
    <
        bridge::Iterator<Keys>,
        bridge::Iterator<Values>
    >;

    using each = bridge::Function
    <
        &Headers::each_1,
        &Headers::each_2
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Headers::funcs[] = {
    JS_CFUNC_DEF("get", 1, &bridge::Function<&Headers::get>::invoke),
    JS_CFUNC_DEF("has", 1, &bridge::Function<&Headers::has>::invoke),
    JS_CFUNC_DEF("set", 2, &bridge::Function<&Headers::set>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&Headers::keys>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&Headers::values>::invoke),
    JS_CFUNC_DEF("append", 2, &bridge::Function<&Headers::append>::invoke),
    JS_CFUNC_DEF("delete", 1, &bridge::Function<&Headers::remove>::invoke),
    JS_CFUNC_DEF("forEach", 1, &each::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Headers>::toJSON)
};

struct URLSearchParams : bridge::Interface<URLSearchParams, std::vector<std::pair<std::string, std::string>>>
{
    struct Handler
    {
        BOOST_FORCEINLINE void key(std::string &&s)
        {
            result.emplace_back(std::make_pair(std::move(s), std::string{}));
        }
        BOOST_FORCEINLINE void val(std::string &&s)
        {
            result.back().second = std::move(s);
        }
        URLSearchParams::Wrapped result;
    };

    struct EncoderCharset
    {
        bool operator()(char ch) const noexcept
        {
            unsigned char c = static_cast<unsigned char>(ch);
            return std::isalnum(c) ||
                   ch == '-' || ch == '_' || ch == '.' || ch == '!' ||
                   ch == '~' || ch == '*' || ch == '\'' ||
                   ch == '(' || ch == ')';
        }
    };

    JSValue append(JSContext *ctx, bridge::String key, bridge::String val)
    {
        ref().emplace_back(std::make_pair(static_cast<std::string>(key), static_cast<std::string>(val)));
        return JS_UNDEFINED;
    }

    JSValue del_0(JSContext *ctx, bridge::String key)
    {
        auto const ks = static_cast<std::string_view const &>(key);
        for(auto it = std::begin(ref()); it != std::end(ref());)
        {
            if(ks == it->first) it = ref().erase(it);
            else ++it;
        }
        return JS_UNDEFINED;
    }

    JSValue del_1(JSContext *ctx, bridge::String key, bridge::String val)
    {
        auto const ks = static_cast<std::string_view const &>(key);
        auto const vs = static_cast<std::string_view const &>(val);
        for(auto it = std::begin(ref()); it != std::end(ref());)
        {
            if(ks == it->first && vs == it->second) it = ref().erase(it);
            else ++it;
        }
        return JS_UNDEFINED;
    }

    using del = bridge::Function
    <
        &URLSearchParams::del_0,
        &URLSearchParams::del_1
    >;

    JSValue entries(JSContext *ctx)
    {
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &[k, v]: ref())
        {
            bridge::Array entry{ctx};
            entry.append(bridge::String(ctx, k));
            entry.append(bridge::String(ctx, v));
            array.append(entry);
        }
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    JSValue get(JSContext *ctx, bridge::String key)
    {
        auto const ks = static_cast<std::string_view const &>(key);
        for(auto const &[k, v]: ref())
            if(ks == k) return bridge::String{ctx, v};
        return JS_NULL;
    }

    JSValue getAll(JSContext *ctx, bridge::String key)
    {
        bridge::Array res{ctx};
        auto const ks = static_cast<std::string_view const &>(key);
        for(auto const &[k, v]: ref())
            if(ks == k) res.append(bridge::String{ctx, v});
        return res;
    }

    JSValue has(JSContext *ctx, bridge::String key)
    {
        auto const ks = static_cast<std::string_view const &>(key);
        for(auto const &[k, v]: ref())
            if(ks == k) return JS_TRUE;
        return JS_FALSE;
    }

    JSValue keys(JSContext *ctx)
    {
        std::unordered_set<std::string> ks;
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &[v, _]: ref())
        {
            if(ks.count(v)) continue;
            array.append(bridge::String{ctx, v});
            ks.insert(v);
        }
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    JSValue set(JSContext *ctx, bridge::String key, bridge::String val)
    {
        auto const ks = static_cast<std::string_view const &>(key);
        bool replaced = false;
        for(auto it = std::begin(ref()); it != std::end(ref());)
        {
            if(it->first == ks)
            {
                if(std::exchange(replaced, true))
                {
                    it = ref().erase(it);
                    continue;
                }
                else
                {
                    it->second = static_cast<std::string>(val);
                }
            }
            ++it;
        }
        if(!replaced) ref().push_back(std::make_pair(std::string{ks}, static_cast<std::string>(val)));
        return JS_UNDEFINED;
    }

    JSValue get_size(JSContext *ctx) const
    {
        return bridge::Number{ctx, static_cast<std::uint32_t>(ref().size())};
    }

    JSValue sort(JSContext *ctx)
    {
        std::sort(std::begin(ref()), std::end(ref()), [](auto const &lh, auto const &rh){
            return lh.first < rh.first;
        });
        return JS_UNDEFINED;
    }

    JSValue toJSON(JSContext *ctx) const
    {
        bool first = true;
        std::string result;

        boost::urls::encoding_opts opt;
        opt.space_as_plus = false;

        for(auto const &[key, value]: ref())
        {
            if(!std::exchange(first, false)) result.append("&");
            result.append(key);
            result.append("=");
            result.append(boost::urls::encode(value, EncoderCharset{}, opt));
        }
        return bridge::String{ctx, std::move(result)};
    }

    JSValue toString(JSContext *ctx)
    {
        return toJSON(ctx);
    }

    JSValue values(JSContext *ctx)
    {
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &[_, k]: ref())
            array.append(bridge::String{ctx, k});
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const URLSearchParams::funcs[] = {
    JS_CGETSET_DEF("size", &bridge::Getter<&URLSearchParams::get_size>, NULL),

    JS_CFUNC_DEF("append", 2, &bridge::Function<&URLSearchParams::append>::invoke),
    JS_CFUNC_DEF("delete", 1, &URLSearchParams::del::invoke),
    JS_CFUNC_DEF("entries", 0, &bridge::Function<&URLSearchParams::entries>::invoke),
    JS_CFUNC_DEF("get", 1, &bridge::Function<&URLSearchParams::get>::invoke),
    JS_CFUNC_DEF("getAll", 1, &bridge::Function<&URLSearchParams::getAll>::invoke),
    JS_CFUNC_DEF("has", 1, &bridge::Function<&URLSearchParams::has>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&URLSearchParams::keys>::invoke),
    JS_CFUNC_DEF("set", 0, &bridge::Function<&URLSearchParams::set>::invoke),
    JS_CFUNC_DEF("sort", 0, &bridge::Function<&URLSearchParams::sort>::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&URLSearchParams::values>::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<URLSearchParams>::toJSON),
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&URLSearchParams::toString>::invoke),
    JS_CFUNC_DEF("[Symbol.toPrimitive]", 0, &bridge::Function<&URLSearchParams::toString>::invoke)
};

template<typename Base>
struct Content : Base
{
    using Base::Base;

    JSValue arrayBuffer(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if (!JS_IsException(promise))
        {
            bridge::Strong<bridge::ArrayBuffer> body{ctx, bridge::ArrayBuffer{ctx, Base::ref().body(), self}};
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, +body));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue bytes(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(!JS_IsException(promise))
        {
            auto blob = bridge::Strong<bridge::ArrayBuffer>(ctx, bridge::ArrayBuffer(ctx, Base::ref().body(), self));
            JSValue args[3] = {blob, JS_NewFloat64(ctx, 0), JS_UNDEFINED};
            auto arr8 = JS_NewTypedArray(ctx, 3, &args[0], JS_TYPED_ARRAY_UINT8);
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &arr8));
            JS_FreeValue(ctx, arr8);
            JS_FreeValue(ctx, args[1]);
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue blob(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(auto const &r = Base::ref(); !JS_IsException(promise))
        {
            JSValue blob = Blob::from(ctx, Blob_{
                .type = std::invoke([&r]{
                    if(auto it = r.find(boost::beast::http::field::content_type); std::end(r) != it)
                        return std::string{it->value()};
                    return std::string{};
                }),
                .data = nullptr,
                .size = r.body().size(),
                .dptr = reinterpret_cast<std::uint8_t const *>(r.body().data())
            }, self);
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &blob));
            JS_FreeValue(ctx, blob);
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue formData(JSContext *ctx) const
    {
        if(auto it = Base::ref().find(boost::beast::http::field::content_type); std::end(Base::ref()) != it && "multipart/form-data" == it->value())
            return JS_NULL; //TODO: support multipart/form-data

        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(auto const &r = Base::ref(); !JS_IsException(promise))
        {
            JSValue data = URLSearchParams::from(ctx, std::move(parser::Search<URLSearchParams::Handler>{}.parse(Base::ref().body()).result));
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &data));
            JS_FreeValue(ctx, data);
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue json(JSContext *ctx) const
    {
        JSValue parsed = JS_ParseJSON(ctx, Base::ref().body().c_str(), Base::ref().body().size(), "<json>");
        if(JS_IsException(parsed))
        {
            return parsed;
        }

        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if (!JS_IsException(promise))
        {
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &parsed));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        JS_FreeValue(ctx, parsed);
        return promise;
    }

    JSValue text(JSContext *ctx) const
    {
        JSValue funcs[2];
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if (!JS_IsException(promise))
        {
            auto body = bridge::String(ctx, Base::ref().body());
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, +body));
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }
};

struct ServerRequest : bridge::Interface<Content<ServerRequest>, boost::beast::http::request<boost::beast::http::string_body>>
{
    ServerRequest() = default;
    ServerRequest(JSContext *ctx, JSValue val) : Base{ctx, val} {}
    ServerRequest(std::reference_wrapper<Wrapped> &&rw) : Base(std::move(rw)) {}

    JSValue get_path(JSContext *ctx) const
    {
        auto const target = ref().target();
        return bridge::String(ctx, std::string_view(target.data(), target.size()));
    }

    JSValue get_headers(JSContext *ctx, JSValue self)
    {
        return Headers::from(ctx, ref(), self);
    }

    JSValue get_method(JSContext *ctx) const
    {
        auto const method = boost::beast::http::to_string(ref().method());
        return JS_NewStringLen(ctx, method.data(), method.size());
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const ServerRequest::funcs[] = {
    JS_CGETSET_DEF("path", &bridge::Getter<&ServerRequest::get_path>, NULL),
    JS_CGETSET_DEF("method", &bridge::Getter<&ServerRequest::get_method>, NULL),
    JS_CGETSET_DEF("headers", &bridge::Getter<&ServerRequest::get_headers>, NULL),

    JS_CFUNC_DEF("arrayBuffer", 0, &bridge::Function<&Content<ServerRequest>::arrayBuffer>::invoke),
    JS_CFUNC_DEF("blob", 0, &bridge::Function<&Content<ServerRequest>::blob>::invoke),
    JS_CFUNC_DEF("bytes", 0, &bridge::Function<&Content<ServerRequest>::bytes>::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&Content<ServerRequest>::text>::invoke),
    JS_CFUNC_DEF("json", 0, &bridge::Function<&Content<ServerRequest>::json>::invoke),
    JS_CFUNC_DEF("formData", 0, &bridge::Function<&Content<ServerRequest>::formData>::invoke)
};

struct Request_ : boost::beast::http::request<boost::beast::http::string_body>
{
    using Base = boost::beast::http::request<boost::beast::http::string_body>;

    static std::string local;
    static constexpr std::string_view SCHEME{"noto"};

    enum CacheAction : uint16_t
    {
        read  = 0x1, // read cache
        write = 0x2, // write cache (only when allowed)
        check = read  | 0x4, // check cache (conditional request)
        force = write | 0x8,  // force write cache
        fetch = read  | 0x16
    };

    enum class Cache : uint16_t
    {
        default_    = 0,
        force_cache = CacheAction::force | CacheAction::read,
        no_store    = default_,
        no_cache    = CacheAction::fetch | CacheAction::write,
        reload      = CacheAction::write,

        read        = CacheAction::read,
        write       = reload,
        check       = CacheAction::check,
        force       = CacheAction::force,
        read_write  = CacheAction::read  | CacheAction::write,
        check_write = CacheAction::check | CacheAction::write,
        fetch_write = CacheAction::fetch | CacheAction::write,
        read_force  = force_cache,
        check_force = CacheAction::check | CacheAction::force,
        fetch_force = CacheAction::fetch | CacheAction::force
    };

    enum class Redirect : uint16_t
    {
        follow,
        error,
        manual
    };

    Request_() = default;
    Request_(Base &&base, boost::urls::url &&url)
    : Base(std::move(base))
    , url{std::move(url)}
    {
        normalize();
        set(boost::beast::http::field::host, this->url.host());
        if(agent) set(boost::beast::http::field::user_agent, *agent);
    }

    Request_(boost::urls::url &&url)
    : Base{boost::beast::http::verb::get, std::invoke([&url]{
        std::string path = url.path();
        if(auto q = url.encoded_query(); !q.empty())
        {
            path.append("?", 1);
            path.append(q.data(), q.size());
        }
        return path;
    }), 11}
    , url{std::move(url)}
    {
        normalize();
        set(boost::beast::http::field::host, this->url.host());
        if(agent) set(boost::beast::http::field::user_agent, *agent);
    }

    BOOST_FORCEINLINE std::vector<char> key() const
    {
        std::vector<char> k;
        k.reserve(2 + url.buffer().size());
        k.insert(std::end(k), url.buffer().data(), url.buffer().data() + url.buffer().size());
        k.insert(std::end(k), {0, static_cast<char>(method())});
        return k;
    }

    boost::urls::url url;
    std::size_t redirects{0};
    std::uint16_t cache = static_cast<std::uint16_t>(Cache::default_);
    Redirect redirect{Redirect::follow};
    std::chrono::milliseconds timeout{10000};

    static std::optional<std::string> agent;

private:
    void normalize()
    {
        if(boost::urls::scheme::unknown == url.scheme_id() && SCHEME == url.scheme())
        {
            std::string u{"http://"};
            u.append(local).append(url.buffer().substr(url.scheme().size() + 1));
            if(auto p = boost::urls::parse_uri(u); p) url = std::move(*p);
        }
    }
};

std::string Request_::local;
std::optional<std::string> Request_::agent{std::nullopt};

struct URL : bridge::Interface<URL, boost::urls::url>
{
    struct String : bridge::String
    {
        using bridge::String::String;

        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            bridge::String v(ctx, *value);
            return valid(static_cast<std::string_view>(v), message);
        }

        static bool valid(std::string_view &&sv, std::string &message)
        {
            if(auto url = boost::urls::parse_uri(sv); !url)
            {
                message = "invalid url [";
                message.append(std::begin(sv), std::end(sv));
                message.append("]");
                return false;
            }
            return true;
        }

        operator boost::urls::url () const
        {
            return *boost::urls::parse_uri(static_cast<std::string_view>(*this));
        }
    };

    URL() = default;

    URL(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    URL(String url)
    : Base(static_cast<boost::urls::url>(url)) {}

    using ctor = bridge::Constructor
    <
        URL(String)
    >;

    JSValue get_host(JSContext *ctx) const
    {
        if(ref().has_port()) return bridge::String(ctx, std::string(ref().host()) + ":" + std::string(ref().port()));
        return get_hostname(ctx);
    }

    void set_host(JSContext *ctx, bridge::String host)
    {
        std::string_view const &sv = static_cast<std::string_view const &>(host);
        if(sv.empty()) {
            ref().set_host("");
            ref().remove_port();
            return;
        }

        auto pos = sv.rfind(':');
        if (pos != std::string_view::npos) {
            ref().set_host(sv.substr(0, pos));
            ref().set_port(sv.substr(pos + 1));
        } else {
            ref().set_host(sv);
            ref().remove_port();
        }
    }

    JSValue get_hostname(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().host());
    }

    void set_hostname(JSContext *ctx, bridge::String hostname)
    {
        ref().host() = hostname;
    }

    JSValue get_href(JSContext *ctx) const
    {
        return bridge::String(ctx, std::string_view{ref().buffer()});
    }

    void set_href(JSContext *ctx, URL::String url)
    {
        static_cast<boost::urls::url>(url).swap(ref());
    }

    JSValue get_pathname(JSContext *ctx) const
    {
        if(ref().path().empty()) return bridge::String(ctx, std::string_view{"/", 1});
        return bridge::String(ctx, ref().path());
    }

    void set_pathname(JSContext *ctx, bridge::String pathname)
    {
        ref().set_path(static_cast<std::string_view const &>(pathname));
    }

    JSValue get_port(JSContext *ctx) const
    {
        return bridge::String(ctx, std::string_view{ref().port()});
    }

    void set_port(JSContext *ctx, bridge::String port)
    {
        ref().set_port(static_cast<std::string_view const &>(port));
    }

    JSValue get_protocol(JSContext *ctx) const
    {
        return bridge::String(ctx, std::string{ref().scheme()} + ":");
    }

    struct Protocol : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            bridge::String str(ctx, *value);
            if(std::string_view const &sv = static_cast<std::string_view const &>(str); !std::invoke([&]{
                if(sv.empty() || !std::isalpha(sv.front()) || ':' != sv.back())
                    return false;

                for(auto ch = std::begin(sv); ch != std::prev(std::end(sv)); ++ch)
                    if (!(std::isalnum(*ch) || *ch == '+' || *ch == '-' || *ch == '.')) return false;

                return true;
            }))
            {
                message = "invalid protocol [";
                message.append(sv.data(), sv.size());
                message.append("]");
                return false;
            }
            return true;
        }
    };

    void set_protocol(JSContext *ctx, Protocol protocol)
    {
        std::string_view const &sv = static_cast<std::string_view const &>(protocol);
        std::string scheme(sv.substr(0, sv.size() - 1));
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c){ return std::tolower(c); });
        ref().set_scheme(scheme);
    }

    JSValue get_search(JSContext *ctx) const
    {
        if(ref().has_query()) return bridge::String(ctx, "?" + ref().query());
        return bridge::String(ctx, ref().query());
    }

    void set_search(JSContext *ctx, bridge::String search)
    {
        std::string_view const &sv = static_cast<std::string_view const &>(search);
        if(sv.empty()) {
            ref().remove_query();
            return;
        }

        if(sv.front() == '?') ref().set_query(sv.substr(1));
        else ref().set_query(sv);
    }

    JSValue get_username(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().user());
    }

    void set_username(JSContext *ctx, bridge::String user)
    {
        std::string_view const &sv = static_cast<std::string_view const &>(user);
        if(sv.empty()) ref().remove_userinfo();
        else ref().set_user(sv);
    }

    JSValue get_password(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().password());
    }

    void set_password(JSContext *ctx, bridge::String pass)
    {
        std::string_view const &sv = static_cast<std::string_view const &>(pass);
        if(sv.empty()) ref().remove_password();
        else ref().set_password(sv);
    }

    JSValue get_searchParams(JSContext *ctx) const
    {
        return URLSearchParams::from(ctx, std::move(parser::Search<URLSearchParams::Handler>{}.parse(ref().query()).result));
    }

    JSValue toJSON(JSContext *ctx) const
    {
        return get_href(ctx);
    }

    JSValue toString(JSContext *ctx)
    {
        return toJSON(ctx);
    }

    static JSValue createObjectURL(JSContext *ctx, Blob blob)
    {
        std::string result;
        result.append("data:");
        if(blob.ref().type.empty())
            result.append("application/octet-stream");
        else
            result.append(blob.ref().type);
        result.append(";base64,");

        std::string base64;
        base64.resize(boost::beast::detail::base64::encoded_size(blob.ref().size));
        base64.resize(boost::beast::detail::base64::encode(&base64[0], blob.ref().dptr, blob.ref().size));
        result.append(std::move(base64));

        return bridge::String(ctx, std::move(result));
    }

    static JSValue revokeObjectURL(JSContext *ctx)
    {
        return JS_UNDEFINED;
    }

    static JSValue canParse(JSContext *ctx, bridge::String u)
    {
        if(auto url = boost::urls::parse_uri(static_cast<std::string_view const &>(u)))
            return JS_TRUE;
        return JS_FALSE;
    }

    static JSValue parse(JSContext *ctx, bridge::String u)
    {
        if(auto url = boost::urls::parse_uri(static_cast<std::string_view const &>(u)))
            return URL::from(ctx, boost::urls::url{*url});
        return JS_NULL;
    }

    struct I : Base::I<I, IURL>
    {
        using Base::Base;

        std::string href() const override
        {
            return ref.buffer();
        }
    };

    using Base::ref;

    using impl = bridge::Implements<I>;
    static JSCFunctionListEntry const funcs[];
    static JSCFunctionListEntry const sfunc[];
};

JSCFunctionListEntry const URL::funcs[] = {
    JS_CGETSET_DEF("host", &bridge::Getter<&URL::get_host>, &bridge::Setter<&URL::set_host>),
    JS_CGETSET_DEF("hostname", &bridge::Getter<&URL::get_hostname>, &bridge::Setter<&URL::set_hostname>),
    JS_CGETSET_DEF("href", &bridge::Getter<&URL::get_href>, &bridge::Setter<&URL::set_href>),
    JS_CGETSET_DEF("pathname", &bridge::Getter<&URL::get_pathname>, &bridge::Setter<&URL::set_pathname>),
    JS_CGETSET_DEF("port", &bridge::Getter<&URL::get_port>, &bridge::Setter<&URL::set_port>),
    JS_CGETSET_DEF("protocol", &bridge::Getter<&URL::get_protocol>, &bridge::Setter<&URL::set_protocol>),
    JS_CGETSET_DEF("search", &bridge::Getter<&URL::get_search>, &bridge::Setter<&URL::set_search>),
    JS_CGETSET_DEF("searchParams", &bridge::Getter<&URL::get_searchParams>, NULL),
    JS_CGETSET_DEF("username", &bridge::Getter<&URL::get_username>, &bridge::Setter<&URL::set_username>),
    JS_CGETSET_DEF("password", &bridge::Getter<&URL::get_password>, &bridge::Setter<&URL::set_password>),
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&URL::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<URL>::toJSON)
};

JSCFunctionListEntry const URL::sfunc[] = {
    JS_CFUNC_DEF("createObjectURL", 1, &bridge::Function<&URL::createObjectURL>::invoke),
    JS_CFUNC_DEF("canParse", 1, &bridge::Function<&URL::canParse>::invoke),
    JS_CFUNC_DEF("parse", 1, &bridge::Function<&URL::parse>::invoke),
    JS_CFUNC_DEF("revokeObjectURL", 1, &bridge::Function<&URL::revokeObjectURL>::invoke)
};

struct Request : bridge::Interface<Request, Request_, ServerRequest>
{
    struct HTTPString : bridge::String
    {
        using bridge::String::String;

        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            bridge::String v(ctx, *value);
            if(auto sv = static_cast<std::string_view>(v); (prefix && !sv.empty() && sv[0] == '/'))
            {
                std::string u = *prefix;
                u.append(std::begin(sv), std::end(sv));
                return valid(std::string_view(u.c_str(), u.size()), message);
            }
            else
            {
                return valid(std::move(sv), message);
            }
        }

        operator boost::urls::url () const
        {
            if(auto sv = static_cast<std::string_view>(*this); (prefix && !sv.empty() && sv[0] == '/'))
            {
                std::string u = *prefix;
                u.append(std::begin(sv), std::end(sv));
                return *boost::urls::parse_uri(u);
            }
            else
            {
                return *boost::urls::parse_uri(sv);
            }
        }

        static std::optional<std::string> prefix;

    private:
        static bool valid(std::string_view &&sv, std::string &message)
        {
            if(auto url = boost::urls::parse_uri(sv); !url)
            {
                message = "invalid url [";
                message.append(std::begin(sv), std::end(sv));
                message.append("]");
                return false;
            }
            else if(boost::urls::scheme::http != url->scheme_id() && boost::urls::scheme::https != url->scheme_id() && Request_::SCHEME != url->scheme())
            {
                message = "unsupported scheme [";
                message.append(std::begin(url->scheme()), std::end(url->scheme()));
                message.append("]");
                return false;
            }
            return true;
        }
    };

    struct HTTPURL : URL
    {
        using URL::URL;

        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            if(auto const &url = URL{ctx, *value}.ref(); boost::urls::scheme::http != url.scheme_id() && boost::urls::scheme::https != url.scheme_id())
            {
                message = "unsupported scheme [";
                message.append(std::begin(url.scheme()), std::end(url.scheme()));
                message.append("]");
                return false;
            }
            return true;
        }
    };

    struct Cache : bridge::String
    {
        using bridge::String::String;

        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            Cache c(ctx, *value);
            if(auto const &cv = static_cast<std::string_view const &>(c); options.find(cv) == std::end(options))
            {
                message.append("invalid cache directive [");
                message.append(std::begin(cv), std::end(cv));
                message.append("]");
                return false;
            }
            return true;
        }

        BOOST_FORCEINLINE operator Request_::Cache () const
        {
            if(auto it = options.find(static_cast<std::string_view const &>(*this)); it != options.end())
                return it->second;
            return Request_::Cache::default_;
        }

    private:
        static std::unordered_map<std::string_view, Request_::Cache> const options;
    };

    struct Method : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            if(Method m(ctx, *value); boost::beast::http::verb::unknown == static_cast<boost::beast::http::verb>(m))
            {
                message.append("invalid method [");
                message.append(std::begin(m), std::end(m));
                message.append("]");
                return false;
            }
            return true;
        }

        BOOST_FORCEINLINE operator boost::beast::http::verb () const
        {
            return boost::beast::http::string_to_verb(static_cast<std::string_view>(*this));
        }
    };

    struct Redirect : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            Redirect r(ctx, *value);
            if(auto rv = static_cast<std::string_view>(r);
                (rv != "error" && rv != "follow" && rv != "manual"))
            {
                message.append("invalid redirect method [");
                message.append(std::begin(rv), std::end(rv));
                message.append("]");
                return false;
            }
            return true;
        }

        BOOST_FORCEINLINE operator Request_::Redirect () const
        {
            if(auto sv = static_cast<std::string_view>(*this); sv == "error")
            {
                return Request_::Redirect::error;
            }
            else if(sv == "manual")
            {
                return Request_::Redirect::manual;
            }
            return Request_::Redirect::follow;
        }
    };

    struct Config : bridge::Struct<Config>
    {
        BRIDGE_DEFINE_STRUCT(Config);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Either<bridge::String, bridge::ArrayBuffer, bridge::Object>>("body"),
            bridge::field<bridge::Either<Headers, bridge::Dict<bridge::String>>>("headers"),
            bridge::field<bridge::Number>("timeout"),
            bridge::field<Redirect>("redirect"),
            bridge::field<Method>("method"),
            bridge::field<Cache>("cache")
        );
    };

    Request() = default;
    Request(Request &&) = default;

    Request(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Request(HTTPString url)
    : Base(Request_(url))
    {}

    Request(HTTPURL url)
    : Base(Request_(boost::urls::url{url.ref()}))
    {}

    Request(HTTPString url, Config config)
    : Base(Request_(url))
    {
        (void)set_config(config);
    }

    Request(HTTPURL url, Config config)
    : Base(Request_(boost::urls::url{url.ref()}))
    {
        (void)set_config(config);
    }

    using ctor = bridge::Constructor
    <
        Request(),
        Request(HTTPString),
        Request(HTTPURL),
        Request(HTTPString, Config),
        Request(HTTPURL, Config)
    >;

    Request &set_config(Config &config)
    {
        if(auto method = config.get<Method>("method"); method)
        {
            ref().method(static_cast<boost::beast::http::verb>(*method));
        }
        if(auto redirect = config.get<Redirect>("redirect"); redirect)
        {
            ref().redirect = static_cast<Request_::Redirect>(*redirect);
        }
        if(auto cache = config.get<Cache>("cache"); cache)
        {
            ref().cache = static_cast<std::underlying_type_t<Request_::Cache>>(static_cast<Request_::Cache>(*cache));
        }
        if(auto body = config.get<bridge::String>("body"); body)
        {
            ref().body() = static_cast<std::string>(*body);
        }
        else if(auto abuf = config.get<bridge::ArrayBuffer>("body"); abuf)
        {
            auto [data, size] = abuf->data();
            ref().body().assign(reinterpret_cast<char const *>(data), size);
        }
        else if(auto json = config.get<bridge::Object>("body"); json)
        {
            ref().body() = static_cast<std::string>(json->json());
            ref().set(boost::beast::http::field::content_type, "application/json");
        }
        if(auto headers = config.get<Headers>("headers"); headers)
        {
            for(auto const &head : headers->ref())
                ref().set(head.name_string(), head.value());
        }
        else if(auto dict = config.get<bridge::Dict<bridge::String>>("headers"); dict)
        {
            dict->each([this](auto &&key, auto value){
                ref().set(key, static_cast<std::string_view const &>(value));
            });
        }
        else if(auto timeout = config.get<bridge::Number>("timeout"); timeout)
        {
            std::int64_t n = static_cast<std::int64_t>(*timeout);
            if(n > 0) ref().timeout = std::chrono::milliseconds(n);
        }
        return *this;
    }

    JSValue get_url(JSContext *ctx) const
    {
        return bridge::String(ctx, static_cast<std::string_view>(ref().url.buffer()));
    }

    JSValue get_redirect(JSContext *ctx) const
    {
        switch(ref().redirect)
        {
        case Request_::Redirect::error:
            return JS_NewString(ctx, "error");
        case Request_::Redirect::follow:
            return JS_NewString(ctx, "follow");
        case Request_::Redirect::manual:
            return JS_NewString(ctx, "manual");
        }
        return JS_UNDEFINED;
    }

    static JSCFunctionListEntry const funcs[];

    using Base::ref;
};

JSCFunctionListEntry const Request::funcs[] = {
    JS_CGETSET_DEF("url", &bridge::Getter<&Request::get_url>, NULL),
    JS_CGETSET_DEF("redirect", &bridge::Getter<&Request::get_redirect>, NULL)
};

std::unordered_map<std::string_view, Request_::Cache> const Request::Cache::options = {
    {"default",     Request_::Cache::default_},
    {"force-cache", Request_::Cache::force_cache},
    {"no-cache",    Request_::Cache::no_cache},
    {"no-store",    Request_::Cache::no_store},
    {"reload",      Request_::Cache::reload},
    {"read",        Request_::Cache::read},
    {"write",       Request_::Cache::write},
    {"check",       Request_::Cache::check},
    {"force",       Request_::Cache::force},
    {"read-write",  Request_::Cache::read_write},
    {"check-write", Request_::Cache::check_write},
    {"fetch-write", Request_::Cache::fetch_write},
    {"read-force",  Request_::Cache::read_force},
    {"check-force", Request_::Cache::check_force},
    {"fetch-force", Request_::Cache::fetch_force}
};

std::optional<std::string> Request::HTTPString::prefix{std::nullopt};

struct ServerResponse : bridge::Interface<Content<ServerResponse>, boost::beast::http::response<boost::beast::http::string_body>>
{
    ServerResponse() = default;
    ServerResponse(JSContext *ctx, JSValue val) : Base{ctx, val} {}
    ServerResponse(std::reference_wrapper<Wrapped> &&rw) : Base(std::move(rw)) {}

    JSValue get_ok(JSContext *ctx) const
    {
        return (ref().result_int() / 100) == 2 ? JS_TRUE : JS_FALSE;
    }

    JSValue get_status(JSContext *ctx) const
    {
        return bridge::Number(ctx, ref().result_int());
    }

    void set_status(JSContext *ctx, bridge::Number s)
    {
        ref().result(static_cast<std::int32_t>(s));
    }

    void set_body_0(JSContext *ctx, bridge::String s)
    {
        ref().body() = static_cast<std::string>(s);
    }

    void set_body_1(JSContext *ctx, Blob blob)
    {
        ref().body().assign(reinterpret_cast<char const *>(blob.ref().dptr), blob.ref().size);
        ref().set(boost::beast::http::field::content_type, blob.ref().type);
    }

    void set_body_2(JSContext *ctx, HTML html)
    {
        if(auto data = html.get<bridge::String>("data"); data)
        {
            ref().body().assign(static_cast<std::string_view const &>(*data));
            ref().set(boost::beast::http::field::content_type, "text/html");
        }
    }

    void set_body_3(JSContext *ctx, SVG svg)
    {
        if(auto data = svg.get<bridge::String>("data"); data)
        {
            ref().body().assign(static_cast<std::string_view const &>(*data));
            ref().set(boost::beast::http::field::content_type, "image/svg+xml");
        }
    }

    void set_body_4(JSContext *ctx, XML xml)
    {
        if(auto data = xml.get<bridge::String>("data"); data)
        {
            ref().body().assign(static_cast<std::string_view const &>(*data));
            ref().set(boost::beast::http::field::content_type, "text/xml");
        }
    }

    void set_body_5(JSContext *ctx, bridge::Object json)
    {
        ref().body() = static_cast<std::string>(json.json());
        ref().set(boost::beast::http::field::content_type, "application/json");
    }

    using set_body = bridge::Setters
    <
        &ServerResponse::set_body_0,
        &ServerResponse::set_body_1,
        &ServerResponse::set_body_2,
        &ServerResponse::set_body_3,
        &ServerResponse::set_body_4,
        &ServerResponse::set_body_5
    >;

    JSValue get_status_text(JSContext *ctx) const
    {
        auto const reason = boost::beast::http::obsolete_reason(ref().result());
        return bridge::String(ctx, std::string_view(reason.data(), reason.size()));
    }

    JSValue get_headers(JSContext *ctx, JSValue self)
    {
        return Headers::from(ctx, ref(), self);
    }

    JSValue toJSON(JSContext *ctx) const
    {
        bridge::Object res{ctx};
        res.set("status", bridge::Number(ctx, ref().result_int()));

        bridge::Object headers{ctx};
        for(auto const &header : ref()) {
            std::string name = header.name_string();
            headers.set(name.c_str(), bridge::String(ctx, static_cast<std::string_view>(header.value())));
        }

        res.set("headers", headers);
        return res;
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const ServerResponse::funcs[] = {
    JS_CGETSET_DEF("ok", &bridge::Getter<&ServerResponse::get_ok>, NULL),
    JS_CGETSET_DEF("body", NULL, &ServerResponse::set_body::invoke),
    JS_CGETSET_DEF("status", &bridge::Getter<&ServerResponse::get_status>, &bridge::Setter<&ServerResponse::set_status>),
    JS_CGETSET_DEF("headers", &bridge::Getter<&ServerResponse::get_headers>, NULL),
    JS_CGETSET_DEF("status_text", &bridge::Getter<&ServerResponse::get_status_text>, NULL),

    JS_CFUNC_DEF("arrayBuffer", 0, &bridge::Function<&Content<ServerResponse>::arrayBuffer>::invoke),
    JS_CFUNC_DEF("blob", 0, &bridge::Function<&Content<ServerResponse>::blob>::invoke),
    JS_CFUNC_DEF("bytes", 0, &bridge::Function<&Content<ServerResponse>::bytes>::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&Content<ServerResponse>::text>::invoke),
    JS_CFUNC_DEF("json", 0, &bridge::Function<&Content<ServerResponse>::json>::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<ServerResponse>::toJSON)
};

struct Response_ : boost::beast::http::response<boost::beast::http::string_body>
{
    Response_() = default;
    Response_(boost::beast::http::response<boost::beast::http::string_body> &&body, bool redirect, boost::urls::url url)
    : boost::beast::http::response<boost::beast::http::string_body>(std::move(body)), redirect{redirect}, url{std::move(url)} {}

    bool redirect{false};
    boost::urls::url url;
};

struct Response : bridge::Interface<Response, Response_, ServerResponse>
{
    Response() = default;
    Response(Response &&) = default;
    Response(JSContext *ctx, JSValue val) : Base{ctx, val} {}

    JSValue get_url(JSContext *ctx) const
    {
        return bridge::String(ctx, static_cast<std::string_view>(ref().url.buffer()));
    }

    JSValue get_redirected(JSContext *ctx) const
    {
        return ref().redirect ? JS_TRUE : JS_FALSE;
    }

    static JSCFunctionListEntry const funcs[];
    using Base::get;
};

JSCFunctionListEntry const Response::funcs[] = {
    JS_CGETSET_DEF("url", &bridge::Getter<&Response::get_url>, NULL),
    JS_CGETSET_DEF("body", NULL, NULL),
    JS_CGETSET_DEF("status", &bridge::Getter<&ServerResponse::get_status>, NULL),
    JS_CGETSET_DEF("redirected", &bridge::Getter<&Response::get_redirected>, NULL)
};

class Worker
{
    Worker() = default;
    Worker(Worker &&) = delete;
    Worker(Worker const &) = delete;

    std::mutex mu;
    std::condition_variable cv;

    class Response : public boost::beast::http::response_parser<boost::beast::http::string_body>
    {
        bool redirect{false};
        boost::urls::url url;
        friend class Worker;

    public:
        Response() = default;

        BOOST_FORCEINLINE operator Response_ ()
        {
            return Response_{std::move(get()), redirect, std::move(url)};
        }

        BOOST_FORCEINLINE std::string str() const
        {
            std::ostringstream ss;
            ss << get();
            return ss.str();
        }
    };

public:
    class Connection : Request
    {
        JSValue funcs[2];
        DB lmdb;
    public:
        Connection(JSContext *ctx, Request &&request, JSValue funcs[2])
        : Request(std::move(request))
        , funcs{JS_DupValue(ctx, funcs[0]), JS_DupValue(ctx, funcs[1])}
        , lmdb{ctx} {}

        using Request::ref;
        std::chrono::system_clock::time_point clock;
        std::unique_ptr<boost::asio::streambuf> buffer;
        std::unique_ptr<boost::asio::steady_timer> timer;

        std::variant<std::monostate,
            std::string, std::unique_ptr<Response>> result;

        std::variant<std::monostate,
            boost::beast::tcp_stream,
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> stream;

        BOOST_FORCEINLINE void failed(JSContext *ctx)
        {
            JSValue error = JS_ThrowInternalError(ctx, "%s", std::get<1>(result).c_str());
            JSValue exception = JS_GetException(ctx);
            JS_FreeValue(ctx, JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, &exception));
            JS_FreeValue(ctx, exception);
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
        }

        BOOST_FORCEINLINE void success(JSContext *ctx)
        {
            auto &raw = std::get<2>(result);
            if(boost::beast::http::status::not_modified == raw->get().result() && (Request_::CacheAction::check & ref().cache))
            {
                std::unique_ptr<Response> old;
                raw.swap(old);
                if(!get_cache())
                {
                    auto &response = result.emplace<2>();
                    old.swap(response);
                }
                else
                {
                    raw->get().set(detail::CACHE_USE, "not-expired");
                }
            }
            else if((Request_::CacheAction::write & ref().cache) && (Request_::CacheAction::force == (Request_::CacheAction::force & ref().cache) || std::invoke([&]{
                if(raw->get().find(boost::beast::http::field::etag) != raw->get().end()) return true;
                if(raw->get().find(boost::beast::http::field::expires) != raw->get().end()) return true;
                if(auto c = raw->get().find(boost::beast::http::field::cache_control); c != raw->get().end())
                    return c->value().find("max-age=") != std::string::npos;
                return false;
            })) && 2 == (raw->get().result_int() / 100) && timer) try
            {
                auto [tx, db] = lmdb.http(DB::RW);
                auto const ck = ref().key();
                auto const cv = raw->str();

                lmdb::val k{ck.data(), ck.size()};
                lmdb::val v{cv.c_str(), cv.size()};
                db.put(tx, k, v);
                tx.commit();

                raw->get().set(detail::CACHE_USE, "updated");
            } catch(std::runtime_error const &) {}
            JSValue response = notojs::Response::from(ctx, std::move(*raw));
            JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &response));
            JS_FreeValue(ctx, response);
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
        }

        BOOST_FORCEINLINE bool get_cache()
        {
            result.emplace<0>();
            auto const ck = ref().key();
            lmdb::val k{ck.data(), ck.size()};
            try {
                auto [tx, db] = lmdb.http();
                if(lmdb::val v; db.get(tx, k, v))
                {
                    boost::system::error_code ec;
                    auto resp = std::make_unique<Response>();
                    for(std::size_t i = 0; i < v.size() && !ec;)
                        i += resp->put(boost::asio::buffer(v.data() + i, v.size() - i), ec);
                    if(!ec) result.emplace<2>().swap(resp);
                }
                tx.abort();
            } catch(std::runtime_error const &) {}
            return 2 == result.index();
        }

        bool success();
        void timeout();
    };

    BOOST_FORCEINLINE void enqueue(Connection &&request, Server &server)
    {
        send(server, queue.emplace_back(std::move(request)));
    }

    BOOST_FORCEINLINE void notify(Connection &con)
    {
        std::unique_lock<std::mutex> guard(mu);
        con.buffer.reset();
        cv.notify_one();
    }

    BOOST_FORCEINLINE void notify(Connection &con, std::string &&error)
    {
        std::unique_lock<std::mutex> guard(mu);
        con.result = std::move(error);
        cv.notify_one();
    }
    void wait(JSContext *ctx);

private:
    std::list<Connection> queue;
    std::list<std::shared_ptr<Task>> tasks;

    void http(Server &, Connection &, boost::asio::ip::tcp::resolver::results_type const &);
    void https(Server &, Connection &, boost::asio::ip::tcp::resolver::results_type const &);

    void send(Server &, Connection &, bool cache);
    void send(Server &, Connection &);

    template<typename Stream>
    void read(Server &, Connection &, Stream &);

    template<typename Stream>
    void write(Server &, Connection &, Stream &);

    template<typename Stream>
    void body(Connection &, Stream &stream);

public:
    static Worker &get();
    friend class notojs::Task;
};

Worker &Worker::get()
{
    thread_local Worker instance;
    return instance;
}

bool Worker::Connection::success()
{
    auto const time = std::chrono::system_clock::now();
    auto const diff = std::chrono::duration_cast<std::chrono::milliseconds>(time - clock);
    if(Request::ref().timeout < diff)
    {
        timer->cancel();
        return false;
    }
    else
    {
        Request::ref().timeout -= diff;
        clock = time;
        return true;
    }
}

void Worker::Connection::timeout()
{
    clock = std::chrono::system_clock::now();
    timer->expires_after(Request::ref().timeout);
    timer->async_wait([this, clock=this->clock](auto ec) {
        if(!ec && clock == this->clock) {
            std::visit(boost::hana::overload_linearly(
                [](std::monostate const &) {},
                [](boost::beast::tcp_stream &s) {s.close(); },
                [&ec](boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &s) { s.shutdown(ec); }
            ), stream);
            Request::ref().timeout = std::chrono::milliseconds(0);
        }
    });
}

void Worker::send(Server &server, Connection &con)
{
    if(auto const m = con.ref().method();
           m != boost::beast::http::verb::get
        && m != boost::beast::http::verb::head
        && m != boost::beast::http::verb::options
    ) con.ref().cache = static_cast<std::underlying_type_t<Request_::Cache>>(Request_::Cache::default_);

    send(server, con, con.ref().cache && std::invoke([&request=con.ref()]{
        if(request.find(boost::beast::http::field::authorization) != std::end(request)) return false;
        if(request.find(boost::beast::http::field::cookie) != std::end(request)) return false;
        return true;
    }));
}

void Worker::send(Server &server, Connection &con, bool cache)
{
    static constexpr auto parse_http_date = [](std::string const &s) {
        std::tm tm{};
        std::istringstream ss(s);
        ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
        if (ss.fail()) return std::time_t{0};
        tm.tm_isdst = 0;
#ifdef _WIN32
        return _mkgmtime(&tm);
#else
        return timegm(&tm);
#endif
    };

    static constexpr auto parse_max_age = [](std::string_view const &s) {
        static constexpr std::string_view key{"max-age="};

        size_t pos = s.find(key);
        if (pos == std::string::npos)
            return 0;

        pos += key.size();
        return std::atoi(&s[pos]);
    };

    if(cache && (Request_::CacheAction::read & con.ref().cache))
    {
        boost::asio::post(*server.sync, [this, &server, &con]{
            if(con.get_cache())
            {
                bool stale = false;
                bool const fetch = Request_::CacheAction::fetch == (Request_::CacheAction::fetch & con.ref().cache);
                if(fetch || Request_::CacheAction::check == (Request_::CacheAction::check & con.ref().cache))
                {
                    auto const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

                    std::time_t date{0}, expd{0};
                    auto const &response = *std::get<2>(con.result);

                    if(auto d = response.get()[boost::beast::http::field::date]; !d.empty())
                    {
                        date = parse_http_date(d);
                        if(auto d = response.get()[boost::beast::http::field::expires]; !d.empty())
                            expd = parse_http_date(d);
                        if(auto c = response.get()[boost::beast::http::field::cache_control]; !c.empty())
                            if(auto a = parse_max_age(c); a) expd = date + a;
                        if((stale = (expd < now || fetch)))
                        {
                            con.ref().set(boost::beast::http::field::if_modified_since, d);
                        }
                    }
                    else if(auto t = response.get()[boost::beast::http::field::etag]; !t.empty())
                    {
                        stale = true;
                        con.ref().set(boost::beast::http::field::if_none_match, t);
                    }
                }
                if(!stale)
                {
                    if(con.timer)
                    {
                        con.timer->cancel();
                        con.timer.reset();
                    }
                    std::get<2>(con.result)->get().set(detail::CACHE_USE, "direct");
                    boost::asio::post(server.resolver.get_executor(), [this, &con]() {
                        notify(con);
                    });
                    return;
                }
                con.result.emplace<0>();
            }
            send(server, con, false);
        });
    }
    else
    {
        std::string port = con.ref().url.port();
        if(port.empty())
        {
            port = std::to_string(boost::urls::scheme::http == con.ref().url.scheme_id() ? 80 : 443);
        }

        con.timer = std::make_unique<boost::asio::steady_timer>(server.resolver.get_executor());
        con.timeout();

        server.resolver.async_resolve(con.ref().url.host(), port,
            [this, &con, &server](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type addr)
            {
                if(!con.success())
                {
                    notify(con, "Request timed out");
                }
                else if(ec)
                {
                    notify(con, ec.message());
                }
                else if(boost::urls::scheme::http == con.ref().url.scheme_id())
                {
                    http(server, con, addr);
                }
                else
                {
                    https(server, con, addr);
                }
            });
    }
}

void Worker::http(Server &server, Connection &con, boost::asio::ip::tcp::resolver::results_type const &addr)
{
    con.timeout();
    con.stream.emplace<1>(server.resolver.get_executor()).async_connect(addr,
        [this, &con, &server](boost::system::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type)
        {
            if(!con.success())
            {
                notify(con, "Request timed out");
            }
            else if(ec)
            {
                notify(con, ec.message());
            }
            else
            {
                write(server, con, std::get<1>(con.stream));
            }
        });
}

void Worker::https(Server &server, Connection &con, boost::asio::ip::tcp::resolver::results_type const &addr)
{
    auto &socket = con.stream.emplace<2>(server.resolver.get_executor(), server.sslcontext);
    if(!SSL_set_tlsext_host_name(socket.native_handle(), con.ref().url.host().data()))
    {
        auto err = ERR_get_error();
        char buf[256];

        ERR_error_string_n(err, buf, sizeof(buf));
        notify(con, std::string(&buf[0]));
    }
    else
    {
        con.timeout();
        boost::asio::async_connect(socket.next_layer(), addr.begin(), addr.end(),
            [this, &con, &server, &socket](boost::system::error_code ec, auto)
            {
                if(!con.success())
                {
                    notify(con, "Request timed out");
                }
                else if(ec)
                {
                    notify(con, ec.message());
                }
                else
                {
                    socket.async_handshake(boost::asio::ssl::stream_base::client, [this, &con, &server, &socket](boost::system::error_code ec){
                        if(ec)
                        {
                            notify(con, ec.message());
                        }
                        else
                        {
                            write(server, con, socket);
                        }
                    });
                }
            });
    }
}

template<typename Stream>
void Worker::read(Server &server, Connection &con, Stream &stream)
{
    con.timeout();
    con.buffer = std::make_unique<boost::asio::streambuf>();
    boost::beast::http::async_read_header(stream, *con.buffer, *(con.result.emplace<2>() = std::make_unique<Response>()),
        [this, &con, &server, &stream](boost::beast::error_code ec, std::size_t)
        {
            if(!con.success())
            {
                notify(con, "Request timed out");
            }
            else if(ec)
            {
                notify(con, ec.message());
            }
            else
            {
                auto &resp = std::get<2>(con.result)->get();
                if(auto code = resp.result(); boost::beast::http::status::not_modified == code)
                {
                    body(con, stream);
                }
                else if(3 == (static_cast<int>(code) / 100))
                {
                    switch(con.ref().redirect)
                    {
                    case Request_::Redirect::follow:
                        if(auto loc = resp[boost::beast::http::field::location]; loc.empty())
                        {
                            notify(con, "Redirect without location");
                        }
                        else if(!std::invoke([&](){
                            if('/' == loc[0])
                            {
                                con.ref().target(loc);
                                con.ref().url.set_path(loc);
                                return true;
                            }
                            else if(auto url = boost::urls::parse_uri(loc); url)
                            {
                                con.ref().target(url->path());
                                con.ref().set(boost::beast::http::field::host, url->host());
                                con.ref().url = std::move(*url);
                                return true;
                            }
                            return false;
                        }))
                        {
                            notify(con, "Bad redirect location");
                        }
                        else
                        {
                            if(303 == static_cast<int>(code))
                            {
                                con.ref().method(boost::beast::http::verb::get);
                                con.ref().body().clear();
                            }
                            ++con.ref().redirects;
                            this->send(server, con);
                        }
                        break;
                    case Request_::Redirect::error:
                        notify(con, "Redirect was not allowed");
                        break;
                    case Request_::Redirect::manual:
                        body(con, stream);
                        break;
                    }
                }
                else
                {
                    std::get<2>(con.result)->redirect = con.ref().redirects;
                    std::get<2>(con.result)->url = con.ref().url;
                    body(con, stream);
                }
            }
        });
}

template<typename Stream>
void Worker::body(Connection &con, Stream &stream)
{
    con.timeout();
    boost::beast::http::async_read(stream, *con.buffer, *std::get<2>(con.result),
        [this, &con, &stream](boost::beast::error_code ec, std::size_t)
        {
            if(!con.success())
            {
                notify(con, "Request timed out");
            }
            else if(ec)
            {
                notify(con, ec.message());
            }
            else
            {
                notify(con);
            }
        });
}

template<typename Stream>
void Worker::write(Server &server, Connection &con, Stream &stream)
{
    con.timeout();
    con.ref().prepare_payload();
    boost::beast::http::async_write(stream, con.ref(),
        [this, &con, &server, &stream](boost::beast::error_code ec, std::size_t)
        {
            if(!con.success())
            {
                notify(con, "Request timed out");
            }
            else if(ec)
            {
                notify(con, ec.message());
            }
            else
            {
                read(server, con, stream);
            }
        });
}

void Worker::wait(JSContext *ctx)
{
    JSContext *ctx1;
    while(JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1) > 0);

    std::unique_lock<std::mutex> guard(mu);
    while(!queue.empty() || !tasks.empty())
    {
        std::list<Connection>::iterator it = std::end(queue);
        std::list<std::shared_ptr<Task>>::iterator jt = std::end(tasks);

        while(
            (it = std::find_if(std::begin(queue), std::end(queue), [](auto const &con){
                return 1 == con.result.index() || (2 == con.result.index() && !con.buffer);
            })) == std::end(queue)
        &&
            (jt = std::find_if(std::begin(tasks), std::end(tasks), [](auto const &task){
                return static_cast<bool>(*task);
            })) == std::end(tasks)
        ) cv.wait(guard);

        while(it != std::end(queue))
        {
            switch(it->result.index())
            {
            case 1:
                it->failed(ctx);
                it = queue.erase(it);
                break;
            case 2:
                if(!it->buffer)
                {
                    it->success(ctx);
                    it = queue.erase(it);
                    break;
                }
            default:
                ++it;
            }
        }
        while(jt != std::end(tasks))
        {
            if(static_cast<bool>(**jt))
            {
                jt->get()->end(ctx);
                jt = tasks.erase(jt);
            }
            else ++jt;
        }

        guard.unlock();
        while(JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1) > 0);
        guard.lock();
    };
}

JSValue fetch_(JSContext *ctx, Request &&request)
{
    JSValue funcs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);

    Worker::get().enqueue(
        Worker::Connection{ctx, std::move(request), funcs}, Global::ptr(ctx)->get<Server>());

    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);
    return promise;
}

JSValue fetch_1(JSContext *ctx, Request request)
{
    return fetch_(ctx, std::move(request));
}

JSValue fetch_2(JSContext *ctx, Request::HTTPString url)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{url})};
    return fetch_(ctx, Request(ctx, req));
}

JSValue fetch_3(JSContext *ctx, Request::HTTPString url, Request::Config config)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{url})};
    return fetch_(ctx, std::move(Request(ctx, req).set_config(config)));
}

JSValue fetch_4(JSContext *ctx, Request::HTTPURL url)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{boost::urls::url{url.ref()}})};
    return fetch_(ctx, Request(ctx, req));
}

JSValue fetch_5(JSContext *ctx, Request::HTTPURL url, Request::Config config)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{boost::urls::url{url.ref()}})};
    return fetch_(ctx, std::move(Request(ctx, req).set_config(config)));
}

using fetch = bridge::Function<fetch_1, fetch_3, fetch_2, fetch_5, fetch_4>;

JSValue print(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if(argc == 0) return JS_UNDEFINED;

    auto &array = Global::Context::ptr(ctx)->output;
    JSValue length = JS_GetPropertyStr(ctx, array, "length");
    uint32_t len = 0;
    if(JS_IsNumber(length))
        JS_ToUint32(ctx, &len, length);
    JS_FreeValue(ctx, length);

    JSValue out = JS_NewArray(ctx);
    int i = 0;
    if(JS_IsString(self))
    {
        bridge::Object obj{ctx};
        using namespace std::string_view_literals;
        obj.set("type", bridge::String{ctx, "notojs.Grid"sv});
        obj.set("data", self);
        JS_SetPropertyUint32(ctx, out, i++, obj);
    }
    for(int j = 0; j < argc; ++j)
    {
        JS_SetPropertyUint32(ctx, out, i++, JS_DupValue(ctx, *(argv + j)));
    }
    JS_SetPropertyUint32(ctx, array, len, out);
    return JS_UNDEFINED;
}

JSValue renderer(JSContext *ctx, bridge::String name)
{
    Global::Context::ptr(ctx)->renderers.insert(name);
    return JS_DupValue(ctx, name);
}

JSValue proxy(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    bridge::String layout{ctx, argv[1]};
    if(auto sv = static_cast<std::string_view const &>(layout); !std::invoke([&sv]{
        static constexpr auto percent = [](const char *b, const char *e) {
            char const *p = b;
            for(;p != e && *p >= '0' && *p <= '9'; ++p);
            if(*p != '%') return false;
            if((p + 1) == e) return true;
            if(++p; *p != '/') return false;
            for(++p; p != e && *p >= '0' && *p <= '9'; ++p);
            return p == e;
        };

        size_t b = 0;
        if(sv.size() != 0 && sv[b] == ':') ++b;
        std::size_t e = sv.find_first_of(' ', b);
        while(e != std::string::npos)
        {
            if(!percent(&sv[b], &sv[e])) return false;
            e = sv.find_first_of(' ', b = e + 1);
        }

        return percent(&sv[b], &sv[sv.size()]) || (sv.size() == 1 && sv[0] == ':');
    })) return JS_ThrowTypeError(ctx, "Invalid layout: [%s]", sv.data());

    (void)JS_DupValue(ctx, argv[1]);
    return JS_NewCFunctionData(ctx, [](JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv, int, JSValueConst *array){
        return JS_Call(ctx, array[0], array[1], argc, argv);
    }, 0, 0xDEADBEEF, 2, argv);
}

struct Storage : bridge::Interface<Storage, DB::Storage>
{
    struct Namespace : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            if(Namespace n(ctx, *value); static_cast<std::string_view>(n).empty()
                || std::string::npos != static_cast<std::string_view>(n).find('/')
                || std::string::npos != static_cast<std::string_view>(n).find(' '))
            {
                message.append("invalid namespace [");
                message.append(std::begin(n), std::end(n));
                message.append("]");
                return false;
            }
            return true;
        }

        BOOST_FORCEINLINE operator JSContext * () const
        {
            return ctx;
        }
    };

    Storage(Namespace ns)
    : Base{DB::Storage{ns, "storage:" + static_cast<std::string>(ns)}}
    {}

    JSValue key(JSContext *ctx, bridge::Number m)
    {
        try {
            return ref().key(m);
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue clear(JSContext *ctx)
    {
        try {
            return ref().remove(), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue getItem(JSContext *ctx, bridge::String k)
    {
        try {
            return ref().get(k);
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue setItem_0(JSContext *ctx, bridge::String k, bridge::String v)
    {
        try {
            return ref().set(k, v), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue setItem_1(JSContext *ctx, bridge::String k, bridge::Value v)
    {
        try {
            return ref().set(k, v), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    using setItem = bridge::Function
    <
        &Storage::setItem_0,
        &Storage::setItem_1
    >;

    JSValue removeItem(JSContext *ctx, bridge::String k)
    {
        try {
            return ref().remove(k), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue get_length(JSContext *ctx) const
    {
        try {
            return bridge::Number(ctx, ref().count());
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    struct Config : bridge::Struct<Config>
    {
        BRIDGE_DEFINE_STRUCT(Config);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("notebook")
        );
    };

    JSValue attach_0(JSValue self, JSContext *ctx)
    {
        JSValue glob = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, glob, "localStorage", JS_DupValue(ctx, self));
        JS_FreeValue(ctx, glob);
        Global::Context::ptr(ctx)->cleanup.insert("localStorage");
        return JS_DupValue(ctx, self);
    }

    JSValue attach_1(JSValue self, JSContext *ctx, Config config)
    {
        JSValue glob = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, glob, "localStorage", JS_DupValue(ctx, self));
        JS_FreeValue(ctx, glob);
        if(auto notebook = config.get<bridge::Boolean>("notebook"); !notebook || !*notebook)
            Global::Context::ptr(ctx)->cleanup.insert("localStorage");
        return JS_DupValue(ctx, self);
    }

    using attach = bridge::Function
    <
        &Storage::attach_0,
        &Storage::attach_1
    >;

    using ctor = bridge::Constructor
    <
        Storage(Namespace)
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Storage::funcs[] = {
    JS_CFUNC_DEF("attach", 0, &Storage::attach::invoke),
    JS_CFUNC_DEF("key", 1, &bridge::Function<&Storage::key>::invoke),
    JS_CFUNC_DEF("clear", 0, &bridge::Function<&Storage::clear>::invoke),
    JS_CFUNC_DEF("getItem", 1, &bridge::Function<&Storage::getItem>::invoke),
    JS_CFUNC_DEF("setItem", 2, &Storage::setItem::invoke),
    JS_CFUNC_DEF("removeItem", 1, &bridge::Function<&Storage::removeItem>::invoke),

    JS_CGETSET_DEF("length", &bridge::Getter<&Storage::get_length>, NULL)
};

JSValue atob(JSContext *ctx, bridge::String base64)
{
    auto sv = static_cast<std::string_view const &>(base64);

    std::string data;
    data.resize(boost::beast::detail::base64::decoded_size(sv.size()));
    data.resize(boost::beast::detail::base64::decode(&data[0], sv.data(), sv.size()).first);

    return bridge::String(ctx, std::move(data));
}

JSValue btoa(JSContext *ctx, bridge::String data)
{
    auto sv = static_cast<std::string_view const &>(data);

    std::string base64;
    base64.resize(boost::beast::detail::base64::encoded_size(sv.size()));
    base64.resize(boost::beast::detail::base64::encode(&base64[0], sv.data(), sv.size()));

    return bridge::String(ctx, std::move(base64));
}

JSValue notojs_init_mustache(JSContext *ctx)
{
    thread_local std::optional<detail::Bytecode> bytecode;
    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};
    if(auto msth = glob.get<bridge::Object>("Mustache"); msth)
    {
        return msth->release();
    }

    if(!bytecode)
    {
        auto const ts = std::chrono::system_clock::now();
        JSValue fn = JS_Eval(ctx, MUSTACHE_JS.data(), MUSTACHE_JS.size(), "mustache", JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
        if(JS_IsError(ctx, fn)) return fn;

        std::size_t size;
        if(std::uint8_t *code = JS_WriteObject(ctx, &size, fn, JS_WRITE_OBJ_BYTECODE); code)
        {
            bytecode.emplace(code, code + size);
            js_free(ctx, code);
        }

        JS_FreeValue(ctx, fn);
        NOTOJS_LOG(Global::ptr(ctx), "Module compiled",
            ("mustache")
            (std::this_thread::get_id())
            (std::chrono::system_clock::now() - ts)
            (size));
    }

    JSValue f = JS_ReadObject(ctx, bytecode->data(), bytecode->size(), JS_READ_OBJ_BYTECODE);
    if(JS_IsException(f) || JS_IsError(ctx, f))
    {
        return f;
    }
    else
    {
        JS_FreeValue(ctx, JS_EvalFunction(ctx, f));
    }

    if(auto msth = glob.get<bridge::Object>("Mustache"); msth)
    {
        return msth->release();
    }
    return JS_UNDEFINED;
}

JSValue pipe_(JSContext *ctx, bridge::Lambda lambda)
{
    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};
    if(auto r = lambda(std::array<JSValue, 1>{glob["input"]}); bridge::Error::check(ctx, r))
        return r.release();
    else if(bridge::Promise::check(ctx, r))
        bridge::Promise(ctx, r).wrap(
            [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) -> JSValue {
                if(argc && !JS_IsUndefined(argv[0])) {
                    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};
                    glob.set("input", argv[0]);
                }
                return JS_UNDEFINED;
            },
            [](JSContext *ctx, JSValueConst, int, JSValueConst *argv) -> JSValue {
                return JS_DupValue(ctx, argv[0]);
            }
        );
    else if(!JS_IsUndefined(r))
        glob.set("input", r.release());
    return JS_UNDEFINED;
}

template<typename T>
JSValue dollar_(JSContext *ctx, T t, bridge::Object c)
{
    JSValue must = notojs_init_mustache(ctx);
    if(JS_IsObject(must))
    {
        JSValue rend = JS_GetPropertyStr(ctx, must, "render");
        if(JS_IsFunction(ctx, rend))
        {
            JSValue p;
            if constexpr (std::is_same_v<T, bridge::String>)
            {
                JSValue args[2] = {t, c};
                p = JS_Call(ctx, rend, must, 2, &args[0]);
            }
            else if(auto data = t.template get<bridge::String>("data"); data)
            {
                JSValue args[2] = {*data, c};
                p = JS_Call(ctx, rend, must, 2, &args[0]);
            }
            else
            {
                p = JS_Call(ctx, rend, must, 1, +c);
            }

            JS_FreeValue(ctx, must);
            if(JS_IsString(p))
            {
                if constexpr (std::is_same_v<T, bridge::String>)
                {
                    must = p;
                }
                else
                {
                    must = T::data(ctx, p);
                }
            }
            else
            {
                must = p;
            }
        }
        JS_FreeValue(ctx, rend);
    }
    return must;
}

using dollar = bridge::Function<
    &pipe_,
    &dollar_<HTML>,
    &dollar_<__Markdown>,
    &dollar_<bridge::String>
>;

JSValue require(JSContext *ctx, bridge::String name)
{
    Global *glob = Global::ptr(ctx);
    auto const n = static_cast<std::string>(name);

    static std::unordered_map<std::string, JSValue(*)(JSContext *)> const scripts = {
#define SCRIPT(name) {#name, &notojs_init_##name}
        SCRIPT(console),
        SCRIPT(dollar),
        SCRIPT(mustache)
#undef SCRIPT
    };

    if(auto it = scripts.find(n); it != std::end(scripts))
        return it->second(ctx);

    if(auto url = facade::URL::parse(n.data()); url)
        return glob->get<Module>().load(ctx, Module::SCRIPT, std::move(*url), n.data());

    try {
        std::optional<boost::urls::url> url;
        lmdb::val k{n.c_str(), n.size()};

        auto [tx, db] = DB(ctx).pkgs();
        if(lmdb::val v; db.get(tx, k, v) && detail::INI::is_script(v))
        {
            url = facade::URL::parse(v.data());
        }
        tx.abort();

        if(url)
        {
            if(auto mdef = glob->get<Module>().load(ctx, Module::SCRIPT, std::move(*url), n.data(), true);
                !JS_IsException(mdef)) return mdef;
        }
    } catch(std::runtime_error const &e) {
        return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
    }

    return JS_ThrowReferenceError(ctx, "Module %s not found", n.data());
}

JSValue config_0(JSContext *ctx)
{
    if(std::string data; boost::beast::http::status::ok != Global::ptr(ctx)->get<Folder>().get_packages(data))
        return JS_ThrowInternalError(ctx, "Could not load packages config");
    else
        return bridge::String(ctx, std::move(data));
}

JSValue config_1(JSContext *ctx, bridge::String config)
{
    if(std::string data = config; boost::beast::http::status::ok != Global::ptr(ctx)->get<Folder>().set_packages(data))
        return JS_ThrowInternalError(ctx, "%s", data.c_str());
    else
        return JS_UNDEFINED;
}

using config = bridge::Function<&config_0, &config_1>;

template<typename Context>
JSValue script_handler_(JSContext *ctx, JSValue *argv, JSValue *data)
{
    if(bridge::String name(ctx, argv[1]); "__context__" == static_cast<std::string_view const &>(name))
    {
        return JS_DupValue(ctx, argv[0]);
    }
    JSAtom atom = JS_ValueToAtom(ctx, argv[1]);
    if(1 == JS_HasProperty(ctx, argv[0], atom))
    {
        JSValue value = JS_GetProperty(ctx, argv[0], atom);
        JS_FreeAtom(ctx, atom);
        return value;
    }

    if constexpr (!std::is_same_v<Context, void>)
    {
        if(1 == JS_HasProperty(ctx, *data, atom))
        {
            JSValue value = JS_GetProperty(ctx, *data, atom);
            JS_FreeAtom(ctx, atom);
            return value;
        }
    }

    JSValue glob = JS_GetGlobalObject(ctx);
    if(1 == JS_HasProperty(ctx, glob, atom))
    {
        JSValue value = JS_GetProperty(ctx, glob, atom);
        JS_FreeValue(ctx, glob);
        JS_FreeAtom(ctx, atom);
        return value;
    }

    JS_FreeValue(ctx, glob);
    JS_FreeAtom(ctx, atom);
    return JS_UNDEFINED;
}

template<typename Capture, typename Context = void>
JSValue script_(JSContext *ctx, JSValueConst self, int, JSValue *resp, int size, JSValue *data)
{
    auto &response = Response::get(*resp);
    if(boost::beast::http::status::ok != response.result())
        return JS_ThrowInternalError(ctx, "Bad HTTP status code: %d", response.result());

    std::string script{"((handler) => {const context = {}; with(new Proxy(context, handler)) {"};
    script.append(response.body());
    if constexpr (std::is_same_v<Capture, bridge::String>)
    {
        bridge::String str{ctx, *data};
        script.append("__context__.");
        script.append(static_cast<std::string_view const &>(str));
        script.append("=");
        script.append(static_cast<std::string_view const &>(str));
        script.append(";");
    }
    else
    {
        bridge::Array arr{ctx, *data};
        for(std::uint32_t i = 0; i < arr.size(); ++i)
        {
            if(auto c = arr.at<bridge::String>(i); c)
            {
                script.append("__context__.");
                script.append(static_cast<std::string_view const &>(*c));
                script.append("=");
                script.append(static_cast<std::string_view const &>(*c));
                script.append(";");
            }
        }
    }
    script.append("} return context; })");

    bridge::Strong<bridge::Object> handler{ctx, JS_NewObject(ctx)};
    handler.set("has", JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv){
        return JS_TRUE;
    }, "has", 1));
    if constexpr (std::is_same_v<Context, void>)
    {
        handler.set("get", JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv){
            return script_handler_<Context>(ctx, argv, nullptr);
        }, "get", 2));
    }
    else
    {
        handler.set("get", JS_NewCFunctionData(ctx, [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int, JSValue *data){
            return script_handler_<Context>(ctx, argv, data);
        }, 2, bridge::Promise::MAGIC | 1, 1, data + 1));
    }
    handler.set("set", JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv){
        bridge::Object obj{ctx, argv[0]};
        bridge::String key{ctx, argv[1]};
        obj.set(static_cast<std::string const &>(key).data(), JS_DupValue(ctx, argv[2]));
        return JS_TRUE;
    }, "set", 3));

    JSValue fn = JS_Eval(ctx, script.c_str(), script.size(), "<eval>", JS_EVAL_TYPE_GLOBAL);
    if(!bridge::Lambda::check(ctx, &fn)) return fn;
    return bridge::Strong<bridge::Lambda>{ctx, fn}(std::array<JSValue, 1>{handler}).release();
}

template<typename ...Ts>
JSValue script_0(JSContext *ctx, JSValue *argv, Request::HTTPString url, Ts...)
{
    return bridge::Strong<bridge::Promise>{ctx, fetch_2(ctx, std::move(url))}.wrap(
        &script_<Ts...>,
        [](JSContext *ctx, JSValueConst, int, JSValueConst *argv)
        {
            bridge::Error error{ctx, argv[0]};
            if(auto message = error.message(); message)
                return JS_ThrowReferenceError(ctx, "%s", message->c_str());
            return JS_Throw(ctx, argv[0]);
        }, sizeof ...(Ts), argv + 1
    ).release();
}

template<typename ...Ts>
JSValue script_1(JSContext *ctx, JSValue *argv, Request req, Ts...)
{
    return bridge::Strong<bridge::Promise>{ctx, fetch_1(ctx, std::move(req))}.wrap(
        &script_<Ts...>,
        [](JSContext *ctx, JSValueConst, int, JSValueConst *argv)
        {
            bridge::Error error{ctx, argv[0]};
            if(auto message = error.message(); message)
                return JS_ThrowReferenceError(ctx, "%s", message->c_str());
            return JS_Throw(ctx, argv[0]);
        }, sizeof ...(Ts), argv + 1
    ).release();
}

template<typename ...Ts>
JSValue script_2(JSContext *ctx, JSValue *argv, Request::HTTPURL req, Ts...)
{
    return bridge::Strong<bridge::Promise>{ctx, fetch_4(ctx, std::move(req))}.wrap(
        &script_<Ts...>,
        [](JSContext *ctx, JSValueConst, int, JSValueConst *argv)
        {
            bridge::Error error{ctx, argv[0]};
            if(auto message = error.message(); message)
                return JS_ThrowReferenceError(ctx, "%s", message->c_str());
            return JS_Throw(ctx, argv[0]);
        }, sizeof ...(Ts), argv + 1
    ).release();
}

using script = bridge::Function
<
    &script_0<bridge::Array>,
    &script_0<bridge::String>,
    &script_0<bridge::Array, bridge::Object>,
    &script_0<bridge::String, bridge::Object>,
    &script_1<bridge::Array>,
    &script_1<bridge::String>,
    &script_1<bridge::Array, bridge::Object>,
    &script_1<bridge::String, bridge::Object>,
    &script_2<bridge::Array>,
    &script_2<bridge::String>,
    &script_2<bridge::Array, bridge::Object>,
    &script_2<bridge::String, bridge::Object>
>;

JSClassID callback_id;

} // namespace

Global::Global()
{
    Blob::init();
    Headers::init();
    ServerRequest::init();
    Request::init();
    ServerResponse::init();
    Response::init();
    Storage::init();
    HTML::init();
    Image::init();
    SVG::init();
    XML::init();
    URL::init();
    URLSearchParams::init();
    __Markdown::init();
    JS_NewClassID(&callback_id);
}

void Global::init(JSRuntime *rt) const
{
    Blob::init(rt);
    Headers::init(rt);
    ServerRequest::init(rt);
    Request::init(rt);
    ServerResponse::init(rt);
    Response::init(rt);
    Storage::init(rt);
    HTML::init(rt);
    Image::init(rt);
    SVG::init(rt);
    XML::init(rt);
    URL::init(rt);
    URLSearchParams::init(rt);
    __Markdown::init(rt);
}

std::unique_ptr<Global::Context> Global::make(JSContext *ctx, JSValue glob) const
{
    auto context = std::make_unique<Global::Context>(JS_NewArray(ctx));
    bool const jsapp = (ctx == JS_GetContextOpaque(ctx));
    bool const fresh = jsapp || !JS_GetContextOpaque(ctx);
    JS_SetContextOpaque(ctx, context.get());

    JSValue g = JS_NewObject(ctx);
    JSValue p = JS_NewCFunction(ctx, &print, NULL, 0);
    JS_SetPropertyStr(ctx, g, "get", JS_NewCFunction(ctx, &proxy, NULL, 3));

    JSValue args[2] = {p, g};

    JSValue proxy = JS_GetPropertyStr(ctx, glob, "Proxy");
    JS_SetPropertyStr(ctx, glob, "print", JS_CallConstructor(ctx, proxy, 2, args));
    JS_FreeValue(ctx, proxy);

    JS_FreeValue(ctx, p);
    JS_FreeValue(ctx, g);

    if(fresh)
    {
        JSValue d = JS_NewCFunction(ctx, &dollar::invoke, "$", 1);
        JS_SetPropertyStr(ctx, glob, "$", d);
        JS_SetPropertyStr(ctx, glob, "atob", JS_NewCFunction(ctx, &bridge::Function<atob>::invoke, "atob", 1));
        JS_SetPropertyStr(ctx, glob, "btoa", JS_NewCFunction(ctx, &bridge::Function<btoa>::invoke, "btoa", 1));
        JS_SetPropertyStr(ctx, glob, "fetch", JS_NewCFunction(ctx, &fetch::invoke, "fetch", 1));

        JSValue r = JS_NewCFunction(ctx, &bridge::Function<&require>::invoke, "require", 1);
        JS_SetPropertyStr(ctx, r, "script", JS_NewCFunction(ctx, &script::invoke, "script", 1));
        JS_SetPropertyStr(ctx, r, "config", JS_NewCFunction(ctx, &config::invoke, "config", 1));
        JS_SetPropertyStr(ctx, glob, "require", r);

        JS_SetPropertyStr(ctx, d, "__renderer", JS_NewCFunction(ctx, &bridge::Function<renderer>::invoke, NULL, 0));

        if(!jsapp)
        {
            ServerRequest::init(ctx);
            ServerResponse::init(ctx, glob);
        }

        Blob::init(ctx, glob);
        Headers::init(ctx, glob);
        Request::init(ctx, glob);
        Response::init(ctx, glob);
        Storage::init(ctx, glob);
        HTML::init(ctx);
        Image::init(ctx);
        SVG::init(ctx);
        XML::init(ctx);
        URL::init(ctx, glob);
        URLSearchParams::init(ctx, glob);
        __Markdown::init(ctx, d);
    }
    return context;
}

void Global::set_handle(JSContext *ctx, SocketBase &socket)
{
    if(!JS_GetContextOpaque(ctx))
    {
        ServerRequest::init(ctx);
        JSValue glob = JS_GetGlobalObject(ctx);
        ServerResponse::init(ctx, glob);
        JS_FreeValue(ctx, glob);
        JS_SetContextOpaque(ctx, ctx);
    }

    JSValue glob = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, glob, "request", ServerRequest::from(ctx, socket.parser.get(), bridge::unsafe));
    JS_SetPropertyStr(ctx, glob, "response", ServerResponse::from(ctx, socket.response, bridge::unsafe));
    JS_FreeValue(ctx, glob);
}

void Global::Context::wait(JSContext *ctx)
{
    Worker::get().wait(ctx);
}

void Global::Context::free(JSContext *ctx)
{
    JS_FreeValue(ctx, output);
    JSValue glob = JS_GetGlobalObject(ctx);
    for(auto const &name : cleanup)
    {
        JSAtom atom = JS_NewAtom(ctx, name.c_str());
        JS_DeleteProperty(ctx, glob, atom, 0);
        JS_FreeAtom(ctx, atom);
    }
    JS_FreeValue(ctx, glob);
    if(perror)
    {
        JS_FreeValue(ctx, *perror);
        perror.reset();
    }
}

void Global::set_agent(std::string &&agent) const
{
    Request_::agent = agent;
}

void Global::set_prefix(std::string &&prefix) const
{
    Request::HTTPString::prefix = std::move(prefix);
}

void Global::configure(boost::property_tree::ptree const &pt)
{
    if(auto agent = pt.get_optional<std::string>("global.agent"); agent)
        Request_::agent = *agent;

    if(auto prefix = pt.get_optional<std::string>("global.prefix"); prefix)
        Request::HTTPString::prefix = *prefix;
    else
        Request::HTTPString::prefix = "http://" + pt.get<std::string>("server.bind");

    Request_::local = pt.get<std::string>("server.bind");
}

thread_local char Task::buffer[16384];

void Task::run()
{
    boost::asio::post(*global->get<Server>().sync, [self=shared_from_this()]{
        if(self->step()) {
            Worker *network = static_cast<Worker*>(std::exchange(self->thread, nullptr));
            std::unique_lock<std::mutex> guard(network->mu);
            network->cv.notify_one();
        } else {
            self->run();
        }
    });
}

void Task::end(JSContext *ctx)
{
    JSValue result = JS_UNDEFINED;
    if(then(ctx, result))
        JS_FreeValue(ctx, JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, &result));
    else
        JS_FreeValue(ctx, JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, &result));
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);
}

JSValue Task::run(JSContext *ctx)
{
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);
    if(!JS_IsException(promise))
    {
        global = Global::ptr(ctx);
        thread = static_cast<void*>(&Worker::get());

        std::unique_lock<std::mutex> guard(Worker::get().mu);
        Worker::get().tasks.push_back(shared_from_this());

        guard.unlock();
        run();
    }
    return promise;
}

JSValue IURL::Static::make(JSContext *ctx, boost::urls::url &&url)
{
    return URL::from(ctx, std::move(url));
}

std::optional<boost::urls::url> IURL::Static::parse(char const *uri)
{
    if(notojs::Request::HTTPString::prefix && strlen(uri) && *uri == '/')
    {
        std::string u = *notojs::Request::HTTPString::prefix;
        u.append(uri);
        if(auto uv = boost::urls::parse_uri(u); uv)
            return std::move(*uv);
    }
    else if(auto uv = boost::urls::parse_uri(uri); uv)
    {
        return std::move(*uv);
    }
    return std::nullopt;
}

JSValue IBlob::Static::make(JSContext *ctx, std::vector<std::uint8_t> &&data, std::string const &type)
{
    auto const size = data.size();
    JSValue blob = Blob::from(ctx, Blob_{
        .type = type,
        .data = Blob_::from(std::move(data)),
        .size = size
    });
    Blob::get(blob).dptr = size
        ? &Blob::get(blob).data->at(0)
        : nullptr;
    return blob;
}

JSValue IBlob::Static::make(JSContext *ctx, std::uint8_t const *data, std::size_t size, std::string const &type)
{
    JSValue blob = Blob::from(ctx, Blob_{
        .type = type,
        .data = Blob_::from(std::vector<std::uint8_t>(data, data + size)),
        .size = size
    });
    Blob::get(blob).dptr = size
        ? &Blob::get(blob).data->at(0)
        : nullptr;
    return blob;
}

JSValue facade::clog(JSContext *ctx, int argc, JSValueConst *argv)
{
    if(argc == 0 || !bridge::String::check(ctx, argv))
    {
        return JS_ThrowTypeError(ctx, "No matching function overload found");
    }

    std::string msg = bridge::String(ctx, argv[0]);
    if(argc > 1)
    {
        std::size_t len;
        std::string raw;

        raw.append("[", 1);
        for(int i = 1; i < argc; ++i)
        {
            if(i > 1) raw.append(",", 1);

            JSValue json = JS_JSONStringify(ctx, argv[i], JS_UNDEFINED, JS_UNDEFINED);
            const char *cstr = JS_ToCStringLen(ctx, &len, json);

            raw.append(cstr, len);

            JS_FreeCString(ctx, cstr);
            JS_FreeValue(ctx, json);
        }
        raw.append("]", 1);

        NOTOJS_LOG_RAW(notojs::Global::ptr(ctx), std::move(msg), std::move(raw));
    }
    else
    {
        NOTOJS_LOG_MSG(notojs::Global::ptr(ctx), std::move(msg));
    }
    return JS_UNDEFINED;
}

JSValue facade::fetch(JSContext *ctx,
    boost::beast::http::request<boost::beast::http::string_body> &&request,
    boost::urls::url &&url,
    JSValue(*callback)(
        JSContext *, JSValue,
        boost::beast::http::response<boost::beast::http::string_body> const &))
{
    using Callback = decltype(callback);
    JSValue cb = JS_NewObjectClass(ctx, callback_id);
    JS_SetOpaque(cb, (void*)callback);

    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{std::move(request), std::move(url)})};
    auto fut = bridge::Strong<bridge::Promise>{ctx, fetch_(ctx, Request{ctx, req})}.wrap(
        [](JSContext *ctx, JSValueConst, int, JSValue *resp, int size, JSValue *data)
        {
            Callback cb = reinterpret_cast<Callback>(JS_GetOpaque(*data, callback_id));
            return cb(ctx, *resp, Response::get(*resp));
        },
        [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            return JS_DupValue(ctx, argv[0]);
        }, 1, &cb
    ).release();
    JS_FreeValue(ctx, cb);
    return fut;
}

JSValue facade::print(JSContext *ctx, int argc, JSValueConst *argv)
{
    return notojs::print(ctx, JS_UNDEFINED, argc, argv);
}

} // namespace notojs
