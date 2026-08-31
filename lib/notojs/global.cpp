#include "quickjs/quickjs.h"
#include <notojs/config.hpp>
#include <notojs/global.hpp>
#include <notojs/logger.hpp>
#include <notojs/module.hpp>
#include <notojs/notojs.hpp>
#include <notojs/server.hpp>
#include <notojs/socket.hpp>

#include <notojs/detail/module.hpp>
#include <notojs/detail/header.hpp>
#include <notojs/detail/jscode.hpp>

#include <notojs/parser/multipart.hpp>
#include <notojs/parser/search.hpp>

#include <notojs/script/console.hpp>
#include <notojs/script/crypto.hpp>
#include <notojs/script/dollar.hpp>
#include <notojs/script/dom.hpp>
#include <notojs/script/storage.hpp>

#include <bridge.hpp>
#include <engine.hpp>
#include <global.hpp>
#include <notodb.hpp>

#include <rapidjson/rapidjson.h>
#include <boost/asio/ssl/stream.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace notojs {
extern const std::string_view MUSTACHE_JS;

namespace {

JSValue settle_promise(JSContext *ctx, JSValue funcs[2], JSValue value)
{
    int resolver = 0;
    if(JS_IsException(value))
    {
        value = JS_GetException(ctx);
        resolver = 1;
    }

    JSValue result = JS_Call(ctx, funcs[resolver], JS_UNDEFINED, 1, &value);
    JS_FreeValue(ctx, value);
    return result;
}

JSValue finish_promise(JSContext *ctx, JSValue promise, JSValue funcs[2], JSValue value)
{
    JSValue settled = settle_promise(ctx, funcs, value);
    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);

    if(JS_IsException(settled))
    {
        JS_FreeValue(ctx, promise);
        return settled;
    }
    JS_FreeValue(ctx, settled);
    return promise;
}

void settle_async(JSContext *ctx, JSValue funcs[2], JSValue value)
{
    JSValue settled = settle_promise(ctx, funcs, value);
    if(JS_IsException(settled))
    {
        JSValue error = JS_GetException(ctx);
        if(auto *context = Global::Context::ptr(ctx); context && !context->perror)
            context->perror = error;
        else
            JS_FreeValue(ctx, error);
    }
    else
    {
        JS_FreeValue(ctx, settled);
    }
}

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
                if(!p.at<bridge::String>(i) && !p.at<bridge::ArrayBuffer>(i) && !p.is_blob(i))
                {
                    message.append("invalid type [");
                    message.append(std::to_string(i));
                    message.append("]");
                    return false;
                }
            }
            return true;
        }

        bool is_blob(std::uint32_t i) const
        {
            auto value = (*this)[i];
            return IBlob::Impl::check(ctx, +value);
        }

        Blob_::Data blob(std::uint32_t i) const
        {
            auto value = (*this)[i];
            if(!IBlob::Impl::check(ctx, +value)) return nullptr;
            return IBlob::Impl{ctx, value}->copy();
        }
    };

    struct Options : bridge::Struct<Options>
    {
        BRIDGE_DEFINE_STRUCT(Options);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::String>("type")
        );
    };

    static Blob_ make(Parts parts, std::string type = {})
    {
        Blob_ result{
            .type = std::move(type),
            .data = Blob_::from(),
            .size = 0,
            .dptr = nullptr
        };
        for(std::size_t i = 0; i < parts.size(); ++i)
        {
            if(auto s = parts.at<bridge::String>(i); s)
            {
                result.data->insert(std::end(*result.data), s->begin(), s->end());
            }
            else if(auto a = parts.at<bridge::ArrayBuffer>(i); a)
            {
                auto const [data, size] = a->data();
                result.data->insert(std::end(*result.data), data, data + size);
            }
            else if(auto data = parts.blob(i); data)
            {
                result.data->insert(std::end(*result.data), std::begin(*data), std::end(*data));
            }
        }
        result.dptr = result.data->empty() ? nullptr : result.data->data();
        result.size = result.data->size();
        return result;
    }

    static Blob_ make(Parts parts, Options &options)
    {
        if(auto type = options.get<bridge::String>("type"); type)
            return make(parts, static_cast<std::string>(*type));
        return make(parts);
    }

    Blob()
    : Base(Blob_{.data = Blob_::from(), .size = 0, .dptr = nullptr})
    {}

    Blob(Parts parts)
    : Base(make(parts))
    {}

    Blob(Parts parts, Options options)
    : Base(make(parts, options))
    {}

    Blob(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    Blob(std::reference_wrapper<Wrapped> &&rw)
    : Base(std::move(rw)) {}

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


    JSValue arrayBuffer(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        auto &r = ref();
        JSValue blob = bridge::ArrayBuffer(ctx, r.dptr, r.size, self);
        return finish_promise(ctx, promise, funcs, blob);
    }

    JSValue bytes(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        auto &r = ref();
        JSValue blob = bridge::ArrayBuffer(ctx, r.dptr, r.size, self);
        JSValue args[3] = {blob, JS_NewFloat64(ctx, 0), JS_UNDEFINED};
        JSValue arr8 = JS_IsException(blob) || JS_IsException(args[1])
            ? JS_EXCEPTION
            : JS_NewTypedArray(ctx, 3, &args[0], JS_TYPED_ARRAY_UINT8);
        JS_FreeValue(ctx, blob);
        JS_FreeValue(ctx, args[1]);
        return finish_promise(ctx, promise, funcs, arr8);
    }

    JSValue text(JSContext *ctx) const
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        auto const &r = ref();
        JSValue text = bridge::String(ctx, std::string_view{
            reinterpret_cast<const char *>(r.dptr), r.size
        });
        return finish_promise(ctx, promise, funcs, text);
    }

    using ctor = bridge::Constructor
    <
        Blob(),
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
    JS_CGETSET_DEF("type", &bridge::Getter<&Blob::get_type>, NULL),

    JS_CFUNC_DEF("arrayBuffer", 0, &bridge::Function<&Blob::arrayBuffer>::invoke),
    JS_CFUNC_DEF("bytes", 0, &bridge::Function<&Blob::bytes>::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&Blob::text>::invoke),
    JS_CFUNC_DEF("slice", 0, &Blob::slice::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Blob>::toJSON)
};

struct File_ : Blob_
{
    std::string name;
    std::int64_t last_modified;
};

struct File : bridge::Interface<File, File_, Blob>
{
    struct Options : bridge::Struct<Options>
    {
        BRIDGE_DEFINE_STRUCT(Options);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::String>("type"),
            bridge::field<bridge::Number>("lastModified")
        );
    };

    static std::int64_t now()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    static File_ make(Blob::Parts parts, bridge::String filename, std::string type, std::int64_t last_modified)
    {
        File_ result;
        static_cast<Blob_ &>(result) = Blob::make(parts, std::move(type));
        result.name = static_cast<std::string>(filename);
        result.last_modified = last_modified;
        return result;
    }

    static File_ make(Blob::Parts parts, bridge::String filename, Options &options)
    {
        std::string type;
        if(auto value = options.get<bridge::String>("type"); value)
            type = static_cast<std::string>(*value);

        std::int64_t last_modified = now();
        if(auto value = options.get<bridge::Number>("lastModified"); value)
            last_modified = static_cast<std::int64_t>(*value);

        return make(parts, filename, std::move(type), last_modified);
    }

    File(Blob::Parts parts, bridge::String filename)
    : Base(make(parts, filename, {}, now()))
    {}

    File(Blob::Parts parts, bridge::String filename, Options options)
    : Base(make(parts, filename, options))
    {}

    File(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    JSValue get_name(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().name);
    }

    JSValue get_lastModified(JSContext *ctx) const
    {
        return bridge::Number(ctx, ref().last_modified);
    }

    JSValue get_webkitRelativePath(JSContext *ctx) const
    {
        return bridge::String(ctx, std::string{});
    }

    using ctor = bridge::Constructor
    <
        File(Blob::Parts, bridge::String),
        File(Blob::Parts, bridge::String, Options)
    >;

    struct I : Base::I<I, IFile>
    {
        using Base::Base;

        std::string name() const override
        {
            return ref.name;
        }

        std::string type() const override
        {
            return ref.type;
        }

        std::int64_t last_modified() const override
        {
            return ref.last_modified;
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

JSCFunctionListEntry const File::funcs[] = {
    JS_CGETSET_DEF("name", &bridge::Getter<&File::get_name>, NULL),
    JS_CGETSET_DEF("lastModified", &bridge::Getter<&File::get_lastModified>, NULL),
    JS_CGETSET_DEF("webkitRelativePath", &bridge::Getter<&File::get_webkitRelativePath>, NULL)
};

struct FormData_
{
    struct BlobValue
    {
        Blob_::Data data;
        std::string type;
        std::string filename;
        std::int64_t last_modified;
    };

    struct Entry
    {
        std::string name;
        std::variant<std::string, BlobValue> value;
    };

    std::vector<Entry> entries;
};

struct FormData : bridge::Interface<FormData, FormData_>
{
    FormData() : Base(FormData_{}) {}

    FormData(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    static FormData_::BlobValue blob(IBlob::Impl const &value, std::string filename)
    {
        return FormData_::BlobValue{
            .data = value->copy(),
            .type = value->type(),
            .filename = std::move(filename),
            .last_modified = File::now()
        };
    }

    static FormData_::BlobValue file(File const &value)
    {
        auto const &source = value.ref();
        return FormData_::BlobValue{
            .data = source.data ? source.data : std::make_shared<Blob_::Data::element_type>(source.dptr, source.dptr + source.size),
            .type = source.type,
            .filename = source.name,
            .last_modified = source.last_modified
        };
    }

    static JSValue value(JSContext *ctx, FormData_::Entry const &entry)
    {
        if(auto text = std::get_if<std::string>(&entry.value))
            return bridge::String(ctx, *text);

        auto const &value = std::get<FormData_::BlobValue>(entry.value);
        File_ file;
        file.type = value.type;
        file.data = value.data;
        file.size = value.data->size();
        file.dptr = value.data->empty() ? nullptr : value.data->data();
        file.name = value.filename;
        file.last_modified = value.last_modified;
        return File::from(ctx, std::move(file));
    }

    JSValue append_0(JSContext *ctx, bridge::String name, bridge::String value)
    {
        ref().entries.push_back(FormData_::Entry{
            static_cast<std::string>(name), static_cast<std::string>(value)
        });
        return JS_UNDEFINED;
    }

    JSValue append_1(JSContext *ctx, bridge::String name, File value)
    {
        ref().entries.push_back(FormData_::Entry{
            static_cast<std::string>(name), file(value)
        });
        return JS_UNDEFINED;
    }

    JSValue append_2(JSContext *ctx, bridge::String name, File value, bridge::String filename)
    {
        auto entry = file(value);
        entry.filename = static_cast<std::string>(filename);
        ref().entries.push_back(FormData_::Entry{static_cast<std::string>(name), std::move(entry)});
        return JS_UNDEFINED;
    }

    JSValue append_3(JSContext *ctx, bridge::String name, IBlob::Impl value)
    {
        ref().entries.push_back(FormData_::Entry{
            static_cast<std::string>(name), blob(value, "blob")
        });
        return JS_UNDEFINED;
    }

    JSValue append_4(JSContext *ctx, bridge::String name, IBlob::Impl value, bridge::String filename)
    {
        ref().entries.push_back(FormData_::Entry{
            static_cast<std::string>(name), blob(value, static_cast<std::string>(filename))
        });
        return JS_UNDEFINED;
    }

    using append = bridge::Function
    <
        &FormData::append_0,
        &FormData::append_1,
        &FormData::append_2,
        &FormData::append_3,
        &FormData::append_4
    >;

    JSValue remove(JSContext *ctx, bridge::String name)
    {
        auto const key = static_cast<std::string_view const &>(name);
        for(auto it = std::begin(ref().entries); it != std::end(ref().entries);)
        {
            if(it->name == key) it = ref().entries.erase(it);
            else ++it;
        }
        return JS_UNDEFINED;
    }

    JSValue entries(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &item: ref().entries)
        {
            bridge::Array entry{ctx};
            entry.append(bridge::String(ctx, item.name));
            entry.append(value(ctx, item));
            array.append(entry);
        }
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    JSValue get(JSContext *ctx, bridge::String name) const
    {
        auto const key = static_cast<std::string_view const &>(name);
        for(auto const &entry: ref().entries)
            if(entry.name == key) return value(ctx, entry);
        return JS_NULL;
    }

    JSValue getAll(JSContext *ctx, bridge::String name) const
    {
        bridge::Array result{ctx};
        auto const key = static_cast<std::string_view const &>(name);
        for(auto const &entry: ref().entries)
            if(entry.name == key) result.append(value(ctx, entry));
        return result;
    }

    JSValue has(JSContext *ctx, bridge::String name) const
    {
        auto const key = static_cast<std::string_view const &>(name);
        for(auto const &entry: ref().entries)
            if(entry.name == key) return JS_TRUE;
        return JS_FALSE;
    }

    JSValue keys(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &entry: ref().entries)
            array.append(bridge::String(ctx, entry.name));
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    void set(std::string name, std::variant<std::string, FormData_::BlobValue> value)
    {
        bool replaced = false;
        for(auto it = std::begin(ref().entries); it != std::end(ref().entries);)
        {
            if(it->name == name)
            {
                if(std::exchange(replaced, true))
                {
                    it = ref().entries.erase(it);
                    continue;
                }
                it->value = value;
            }
            ++it;
        }
        if(!replaced) ref().entries.push_back(FormData_::Entry{std::move(name), std::move(value)});
    }

    JSValue set_0(JSContext *ctx, bridge::String name, bridge::String value)
    {
        set(static_cast<std::string>(name), static_cast<std::string>(value));
        return JS_UNDEFINED;
    }

    JSValue set_1(JSContext *ctx, bridge::String name, File value)
    {
        set(static_cast<std::string>(name), file(value));
        return JS_UNDEFINED;
    }

    JSValue set_2(JSContext *ctx, bridge::String name, File value, bridge::String filename)
    {
        auto entry = file(value);
        entry.filename = static_cast<std::string>(filename);
        set(static_cast<std::string>(name), std::move(entry));
        return JS_UNDEFINED;
    }

    JSValue set_3(JSContext *ctx, bridge::String name, IBlob::Impl value)
    {
        set(static_cast<std::string>(name), blob(value, "blob"));
        return JS_UNDEFINED;
    }

    JSValue set_4(JSContext *ctx, bridge::String name, IBlob::Impl value, bridge::String filename)
    {
        set(static_cast<std::string>(name), blob(value, static_cast<std::string>(filename)));
        return JS_UNDEFINED;
    }

    using set_value = bridge::Function
    <
        &FormData::set_0,
        &FormData::set_1,
        &FormData::set_2,
        &FormData::set_3,
        &FormData::set_4
    >;

    JSValue values(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> array(ctx, bridge::Array{ctx});
        for(auto const &entry: ref().entries)
            array.append(value(ctx, entry));
        bridge::Strong<bridge::Lambda> values(ctx, JS_GetPropertyStr(ctx, array, "values"));
        return values(array).release();
    }

    static std::string escape(std::string_view value)
    {
        std::string result;
        result.reserve(value.size());
        for(char ch: value)
        {
            switch(ch)
            {
            case '\r': result.append("%0D"); break;
            case '\n': result.append("%0A"); break;
            case '"': result.append("%22"); break;
            default: result.push_back(ch); break;
            }
        }
        return result;
    }

    static std::string encode(FormData_ const &form, std::string_view boundary)
    {
        std::string result;
        for(auto const &entry: form.entries)
        {
            result.append("--");
            result.append(boundary);
            result.append("\r\nContent-Disposition: form-data; name=\"");
            result.append(escape(entry.name));
            result.append("\"");

            if(auto blob = std::get_if<FormData_::BlobValue>(&entry.value))
            {
                result.append("; filename=\"");
                result.append(escape(blob->filename));
                result.append("\"\r\nContent-Type: ");
                result.append(blob->type.empty() ? "application/octet-stream" : blob->type);
                result.append("\r\n\r\n");
                if(!blob->data->empty()) result.append(
                    reinterpret_cast<char const *>(blob->data->data()), blob->data->size()
                );
            }
            else
            {
                result.append("\r\n\r\n");
                result.append(std::get<std::string>(entry.value));
            }
            result.append("\r\n");
        }
        result.append("--");
        result.append(boundary);
        result.append("--\r\n");
        return result;
    }

    using ctor = bridge::Constructor<FormData()>;
    static JSCFunctionListEntry const funcs[];

    using Base::ref;
};

JSCFunctionListEntry const FormData::funcs[] = {
    JS_CFUNC_DEF("append", 2, &FormData::append::invoke),
    JS_CFUNC_DEF("delete", 1, &bridge::Function<&FormData::remove>::invoke),
    JS_CFUNC_DEF("entries", 0, &bridge::Function<&FormData::entries>::invoke),
    JS_CFUNC_DEF("get", 1, &bridge::Function<&FormData::get>::invoke),
    JS_CFUNC_DEF("getAll", 1, &bridge::Function<&FormData::getAll>::invoke),
    JS_CFUNC_DEF("has", 1, &bridge::Function<&FormData::has>::invoke),
    JS_CFUNC_DEF("keys", 0, &bridge::Function<&FormData::keys>::invoke),
    JS_CFUNC_DEF("set", 2, &FormData::set_value::invoke),
    JS_CFUNC_DEF("values", 0, &bridge::Function<&FormData::values>::invoke),
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&FormData::entries>::invoke)
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
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        JSValue body = bridge::ArrayBuffer{ctx, Base::ref().body(), self};
        return finish_promise(ctx, promise, funcs, body);
    }

    JSValue bytes(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        JSValue blob = bridge::ArrayBuffer(ctx, Base::ref().body(), self);
        JSValue args[3] = {blob, JS_NewFloat64(ctx, 0), JS_UNDEFINED};
        JSValue arr8 = JS_IsException(blob)
            ? JS_EXCEPTION
            : JS_NewTypedArray(ctx, 3, &args[0], JS_TYPED_ARRAY_UINT8);
        JS_FreeValue(ctx, blob);
        JS_FreeValue(ctx, args[1]);
        return finish_promise(ctx, promise, funcs, arr8);
    }

    JSValue blob(JSValue self, JSContext *ctx)
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        auto const &r = Base::ref();
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
        return finish_promise(ctx, promise, funcs, blob);
    }

    JSValue formData(JSContext *ctx) const
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        auto reject = [ctx, &funcs](std::string_view message)
        {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue constructor = JS_GetPropertyStr(ctx, global, "TypeError");
            JSValue text = JS_NewStringLen(ctx, message.data(), message.size());
            JSValue error = JS_CallConstructor(ctx, constructor, 1, &text);
            JS_FreeValue(ctx, text);
            JS_FreeValue(ctx, constructor);
            JS_FreeValue(ctx, global);
            if(JS_IsException(error))
            {
                settle_async(ctx, funcs, error);
            }
            else
            {
                JSValue rejected = JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, &error);
                JS_FreeValue(ctx, error);
                if(JS_IsException(rejected))
                {
                    JSValue exception = JS_GetException(ctx);
                    if(auto *context = Global::Context::ptr(ctx); context && !context->perror)
                        context->perror = exception;
                    else
                        JS_FreeValue(ctx, exception);
                }
                else
                {
                    JS_FreeValue(ctx, rejected);
                }
            }
        };

        auto resolve = [ctx, &funcs](JSValue data)
        {
            settle_async(ctx, funcs, data);
        };

        auto is_type = [](std::string_view value, std::string_view expected)
        {
            value = value.substr(0, value.find(';'));
            while(!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
            while(!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
            return value.size() == expected.size() && std::equal(
                std::begin(value), std::end(value), std::begin(expected),
                [](unsigned char left, unsigned char right){ return std::tolower(left) == std::tolower(right); }
            );
        };

        if(auto const &r = Base::ref(); !JS_IsException(promise))
        {
            auto content_type = r.find(boost::beast::http::field::content_type);
            std::string_view type = content_type == std::end(r)
                ? std::string_view{}
                : std::string_view{content_type->value().data(), content_type->value().size()};

            if(parser::Multipart::is(type))
            {
                auto parsed = parser::Multipart::parse(type, r.body());
                if(!parsed)
                {
                    reject(parsed.error);
                }
                else
                {
                    FormData_ form;
                    form.entries.reserve(parsed.parts.size());
                    for(auto &part: parsed.parts)
                    {
                        if(part.filename)
                        {
                            std::vector<std::uint8_t> bytes(std::begin(part.body), std::end(part.body));
                            form.entries.push_back(FormData_::Entry{
                                std::move(part.name),
                                FormData_::BlobValue{
                                    .data = Blob_::from(std::move(bytes)),
                                    .type = std::string(part.type),
                                    .filename = std::move(*part.filename),
                                    .last_modified = File::now()
                                }
                            });
                        }
                        else
                        {
                            form.entries.push_back(FormData_::Entry{
                                std::move(part.name), std::string(part.body)
                            });
                        }
                    }

                    resolve(FormData::from(ctx, std::move(form)));
                }
            }
            else if(is_type(type, "application/x-www-form-urlencoded"))
            {
                auto decoded = std::move(parser::Search<URLSearchParams::Handler>{}.parse(r.body()).result);
                FormData_ form;
                form.entries.reserve(decoded.size());
                for(auto &[name, value]: decoded)
                    form.entries.push_back(FormData_::Entry{std::move(name), std::move(value)});
                resolve(FormData::from(ctx, std::move(form)));
            }
            else if(type.empty())
            {
                reject("Missing Content-Type");
            }
            else
            {
                std::string message{"Unsupported form data Content-Type: "};
                message.append(type);
                reject(message);
            }
        }

        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    JSValue json(JSContext *ctx) const
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);
        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        JSValue parsed = JS_ParseJSON(ctx, Base::ref().body().c_str(), Base::ref().body().size(), "<json>");
        return finish_promise(ctx, promise, funcs, parsed);
    }

    JSValue text(JSContext *ctx) const
    {
        JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
        JSValue promise = JS_NewPromiseCapability(ctx, funcs);

        if(JS_IsException(promise))
        {
            JS_FreeValue(ctx, funcs[0]);
            JS_FreeValue(ctx, funcs[1]);
            return promise;
        }

        JSValue body = bridge::String(ctx, Base::ref().body());
        return finish_promise(ctx, promise, funcs, body);
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
        maybenoto(this->url);
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
        maybenoto(this->url);
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

public: // static
    static std::optional<std::string> agent;

    BOOST_FORCEINLINE static void maybenoto(boost::urls::url &url)
    {
        if(boost::urls::scheme::unknown == url.scheme_id() && url.is_path_absolute() && SCHEME == url.scheme())
        {
            std::string u{"http://"};
            u.append(local).append(url.buffer().substr(url.scheme().size() + 1));
            if(auto p = boost::urls::parse_uri(u)) url = std::move(*p);
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
            bridge::field<bridge::Either<bridge::String, bridge::ArrayBuffer, FormData, bridge::Object>>("body"),
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
        else if(auto form = config.get<FormData>("body"); form)
        {
            static thread_local boost::uuids::random_generator generator;
            std::string boundary = "----notojs-" + boost::uuids::to_string(generator());
            ref().body() = FormData::encode(form->ref(), boundary);
            ref().set(
                boost::beast::http::field::content_type,
                "multipart/form-data; boundary=" + boundary
            );
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
    JS_CFUNC_DEF("formData", 0, &bridge::Function<&Content<ServerResponse>::formData>::invoke),

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
            settle_async(ctx, funcs, JS_ThrowInternalError(ctx, "%s", std::get<1>(result).c_str()));
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
            settle_async(ctx, funcs, response);
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
    auto execute_jobs = [](JSRuntime *runtime) {
        for(;;)
        {
            JSContext *job_ctx{nullptr};
            int const status = JS_ExecutePendingJob(runtime, &job_ctx);
            if(status > 0) continue;
            if(status == 0) break;

            JSValue error = JS_GetException(job_ctx);
            if(auto *context = Global::Context::ptr(job_ctx); context && !context->perror)
                context->perror = error;
            else
                JS_FreeValue(job_ctx, error);
        }
    };

    execute_jobs(JS_GetRuntime(ctx));

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
        execute_jobs(JS_GetRuntime(ctx));
        guard.lock();
    };
}

JSValue fetch_(JSContext *ctx, Request &&request)
{
    JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);
    if(JS_IsException(promise))
    {
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

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

JSValue fetch_6(JSContext *ctx, Request::HTTPString url, bridge::Undefined)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{url})};
    return fetch_(ctx, Request(ctx, req));
}

JSValue fetch_7(JSContext *ctx, Request::HTTPURL url, bridge::Undefined)
{
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, Request_{boost::urls::url{url.ref()}})};
    return fetch_(ctx, Request(ctx, req));
}

using fetch = bridge::Function<fetch_1, fetch_3, fetch_2, fetch_5, fetch_4, fetch_6, fetch_7>;

JSValue print(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv)
{
    if(argc == 0) return JS_UNDEFINED;

    bool const g = JS_IsString(self);
    auto &output = Global::Context::ptr(ctx)->output;

    if(argc == 1 && !g && IPrint::Impl::check(ctx, argv))
    {
        return IPrint::Impl{ctx, *argv}->print(ctx, bridge::Array{ctx, output});
    }
    else
    {
        JSValue length = JS_GetPropertyStr(ctx, output, "length");
        uint32_t len = 0;
        if(JS_IsNumber(length))
            JS_ToUint32(ctx, &len, length);
        JS_FreeValue(ctx, length);

        JSValue out = JS_NewArray(ctx);
        int i = 0;
        if(g)
        {
            bridge::Object obj{ctx};
            using namespace std::string_view_literals;
            obj.set("type", bridge::String{ctx, "notojs.Grid"sv});
            obj.set("data", self);
            JS_SetPropertyUint32(ctx, out, i++, obj);
        }
        for(int j = 0; j < argc; ++j)
        {
            if(IPrint::Impl::check(ctx, argv + j))
            {
                JS_FreeValue(ctx, out);
                return JS_ThrowRangeError(ctx, "Printable object must be only argument");
            }
            JS_SetPropertyUint32(ctx, out, i++, bridge::Lambda::check(ctx, argv + j) ? JS_NewString(ctx, "function") : JS_DupValue(ctx, *(argv + j)));
        }
        JS_SetPropertyUint32(ctx, output, len, out);
        return JS_UNDEFINED;
    }
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

    using ctor = bridge::Constructor
    <
        Storage(Namespace)
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Storage::funcs[] = {
    JS_CFUNC_DEF("key", 1, &bridge::Function<&Storage::key>::invoke),
    JS_CFUNC_DEF("clear", 0, &bridge::Function<&Storage::clear>::invoke),
    JS_CFUNC_DEF("getItem", 1, &bridge::Function<&Storage::getItem>::invoke),
    JS_CFUNC_DEF("setItem", 2, &Storage::setItem::invoke),
    JS_CFUNC_DEF("removeItem", 1, &bridge::Function<&Storage::removeItem>::invoke),

    JS_CGETSET_DEF("length", &bridge::Getter<&Storage::get_length>, NULL)
};

struct TextEncoder : bridge::Interface<TextEncoder>
{
    static bool encode_(JSContext *ctx, JSValue source, std::vector<std::uint8_t> &output,
                       std::size_t capacity, std::size_t &read)
    {
        std::size_t size;
        char const *text = JS_ToCStringLen2(ctx, &size, source, true);
        if(!text) return false;

        auto const *data = reinterpret_cast<std::uint8_t const *>(text);
        auto unit = [data, size](std::size_t offset, std::uint32_t &value, std::size_t &length) {
            if(offset >= size) return false;
            std::uint8_t const c = data[offset];
            if(c < 0x80)
            {
                value = c;
                length = 1;
                return true;
            }
            if(c >= 0xC2 && c <= 0xDF && offset + 1 < size && (data[offset + 1] & 0xC0) == 0x80)
            {
                value = ((c & 0x1F) << 6) | (data[offset + 1] & 0x3F);
                length = 2;
                return true;
            }
            if(c >= 0xE0 && c <= 0xEF && offset + 2 < size &&
               (data[offset + 1] & 0xC0) == 0x80 && (data[offset + 2] & 0xC0) == 0x80)
            {
                value = ((c & 0x0F) << 12) | ((data[offset + 1] & 0x3F) << 6) | (data[offset + 2] & 0x3F);
                length = 3;
                return true;
            }
            return false;
        };

        read = 0;
        std::size_t offset = 0;
        while(offset < size)
        {
            std::uint32_t codepoint;
            std::size_t consumed;
            if(!unit(offset, codepoint, consumed))
            {
                codepoint = 0xFFFD;
                consumed = 1;
            }

            std::size_t units = 1;
            if(codepoint >= 0xD800 && codepoint <= 0xDBFF)
            {
                std::uint32_t low;
                std::size_t low_size;
                if(unit(offset + consumed, low, low_size) && low >= 0xDC00 && low <= 0xDFFF)
                {
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    consumed += low_size;
                    units = 2;
                }
                else
                {
                    codepoint = 0xFFFD;
                }
            }
            else if(codepoint >= 0xDC00 && codepoint <= 0xDFFF)
            {
                codepoint = 0xFFFD;
            }

            std::size_t const required = codepoint < 0x80 ? 1 : codepoint < 0x800 ? 2 : codepoint < 0x10000 ? 3 : 4;
            if(output.size() + required > capacity) break;

            if(required == 1)
                output.push_back(static_cast<std::uint8_t>(codepoint));
            else if(required == 2)
            {
                output.push_back(static_cast<std::uint8_t>(0xC0 | (codepoint >> 6)));
                output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else if(required == 3)
            {
                output.push_back(static_cast<std::uint8_t>(0xE0 | (codepoint >> 12)));
                output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else
            {
                output.push_back(static_cast<std::uint8_t>(0xF0 | (codepoint >> 18)));
                output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
                output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            offset += consumed;
            read += units;
        }

        JS_FreeCString(ctx, text);
        return true;
    }

    static JSValue array(JSContext *ctx, std::vector<std::uint8_t> const &data)
    {
        JSValue args[3] = {
            JS_NewArrayBufferCopy(ctx, data.data(), data.size()),
            JS_NewInt32(ctx, 0),
            JS_UNDEFINED
        };
        if(JS_IsException(args[0])) return args[0];

        JSValue result = JS_NewTypedArray(ctx, 3, args, JS_TYPED_ARRAY_UINT8);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        return result;
    }

    JSValue get_encoding(JSContext *ctx) const
    {
        return JS_NewString(ctx, "utf-8");
    }

    JSValue encode_0(JSContext *ctx)
    {
        std::vector<std::uint8_t> output;
        return array(ctx, output);
    }

    JSValue encode_1(JSContext *ctx, bridge::Value source)
    {
        if(JS_IsUndefined(source)) return encode_0(ctx);

        std::vector<std::uint8_t> output;
        std::size_t read;
        if(!encode_(ctx, source, output, std::numeric_limits<std::size_t>::max(), read))
            return JS_EXCEPTION;
        return array(ctx, output);
    }

    JSValue encodeInto(JSContext *ctx, bridge::Value source, bridge::Value destination)
    {
        JSValue glob = JS_GetGlobalObject(ctx);
        JSValue ctor = JS_GetPropertyStr(ctx, glob, "Uint8Array");
        int const is_uint8 = JS_IsInstanceOf(ctx, destination, ctor);
        JS_FreeValue(ctx, ctor);
        JS_FreeValue(ctx, glob);
        if(is_uint8 < 0) return JS_EXCEPTION;
        if(!is_uint8) return JS_ThrowTypeError(ctx, "encodeInto destination must be a Uint8Array");

        std::size_t offset;
        std::size_t length;
        JSValue buffer = JS_GetTypedArrayBuffer(ctx, destination, &offset, &length, nullptr);
        if(JS_IsException(buffer)) return buffer;

        std::vector<std::uint8_t> output;
        output.reserve(length);
        std::size_t read;
        if(!encode_(ctx, source, output, length, read))
        {
            JS_FreeValue(ctx, buffer);
            return JS_EXCEPTION;
        }

        std::size_t buffer_size;
        std::uint8_t *bytes = JS_GetArrayBuffer(ctx, &buffer_size, buffer);
        if(!bytes)
        {
            JS_FreeValue(ctx, buffer);
            return JS_EXCEPTION;
        }
        if(offset > buffer_size || output.size() > buffer_size - offset)
        {
            JS_FreeValue(ctx, buffer);
            return JS_ThrowRangeError(ctx, "Uint8Array range exceeds its ArrayBuffer");
        }
        std::copy(output.begin(), output.end(), bytes + offset);
        JS_FreeValue(ctx, buffer);

        JSValue result = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, result, "read", JS_NewInt64(ctx, read));
        JS_SetPropertyStr(ctx, result, "written", JS_NewInt64(ctx, output.size()));
        return result;
    }

    using encode = bridge::Function<&TextEncoder::encode_0, &TextEncoder::encode_1>;
    using ctor = bridge::Constructor<TextEncoder()>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const TextEncoder::funcs[] = {
    JS_CGETSET_DEF("encoding", &bridge::Getter<&TextEncoder::get_encoding>, nullptr),
    JS_CFUNC_DEF("encode", 1, &TextEncoder::encode::invoke),
    JS_CFUNC_DEF("encodeInto", 2, &bridge::Function<&TextEncoder::encodeInto>::invoke)
};

struct TextDecoder_
{
    bool fatal{false};
    bool ignore_bom{false};
    bool bom_seen{false};
    std::vector<std::uint8_t> pending;
};

struct TextDecoder : bridge::Interface<TextDecoder, TextDecoder_>
{
    struct Encoding : bridge::Value
    {
        using bridge::Value::Value;
        static constexpr bool check(JSContext *, JSValue *) { return true; }
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            if(JS_IsUndefined(*value)) return true;

            bridge::Strong<bridge::String> encoding{ctx, JS_ToString(ctx, *value)};
            if(JS_IsException(encoding)) return false;
            std::string label = static_cast<std::string>(encoding);
            auto first = label.find_first_not_of("\t\n\f\r ");
            auto last = label.find_last_not_of("\t\n\f\r ");
            if(first == std::string::npos) label.clear();
            else label = label.substr(first, last - first + 1);
            std::transform(label.begin(), label.end(), label.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if(label == "utf-8" || label == "utf8" || label == "unicode-1-1-utf-8") return true;
            message = "unsupported encoding [" + label + "]";
            return false;
        }
    };

    struct Options : bridge::Struct<Options>
    {
        BRIDGE_DEFINE_STRUCT(Options);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("fatal"),
            bridge::field<bridge::Boolean>("ignoreBOM")
        );
    };

    struct DecodeOptions : bridge::Struct<DecodeOptions>
    {
        BRIDGE_DEFINE_STRUCT(DecodeOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("stream")
        );
    };

    TextDecoder() : Base{TextDecoder_{}} {}
    TextDecoder(Encoding) : Base{TextDecoder_{}} {}
    TextDecoder(Encoding, Options options) : Base{TextDecoder_{}}
    {
        if(auto fatal = options.get<bridge::Boolean>("fatal"); fatal) ref().fatal = *fatal;
        if(auto ignore = options.get<bridge::Boolean>("ignoreBOM"); ignore) ref().ignore_bom = *ignore;
    }

    JSValue get_encoding(JSContext *ctx) const { return JS_NewString(ctx, "utf-8"); }
    JSValue get_fatal(JSContext *ctx) const { return JS_NewBool(ctx, ref().fatal); }
    JSValue get_ignoreBOM(JSContext *ctx) const { return JS_NewBool(ctx, ref().ignore_bom); }

    static void append(std::vector<std::uint8_t> &output, std::uint32_t codepoint)
    {
        if(codepoint < 0x80)
            output.push_back(static_cast<std::uint8_t>(codepoint));
        else if(codepoint < 0x800)
        {
            output.push_back(static_cast<std::uint8_t>(0xC0 | (codepoint >> 6)));
            output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
        }
        else if(codepoint < 0x10000)
        {
            output.push_back(static_cast<std::uint8_t>(0xE0 | (codepoint >> 12)));
            output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
            output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
        }
        else
        {
            output.push_back(static_cast<std::uint8_t>(0xF0 | (codepoint >> 18)));
            output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
            output.push_back(static_cast<std::uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
            output.push_back(static_cast<std::uint8_t>(0x80 | (codepoint & 0x3F)));
        }
    }

    bool decode_utf8(std::vector<std::uint8_t> const &input, bool stream, std::vector<std::uint8_t> &output)
    {
        auto &state = ref();
        std::vector<std::uint8_t> data;
        data.reserve(state.pending.size() + input.size());
        data.insert(data.end(), state.pending.begin(), state.pending.end());
        data.insert(data.end(), input.begin(), input.end());
        state.pending.clear();

        auto emit = [&state, &output](std::uint32_t codepoint) {
            if(!state.bom_seen)
            {
                state.bom_seen = true;
                if(codepoint == 0xFEFF && !state.ignore_bom) return;
            }
            append(output, codepoint);
        };

        auto invalid = [&state, &emit]() {
            if(state.fatal) return false;
            emit(0xFFFD);
            return true;
        };

        std::size_t i = 0;
        while(i < data.size())
        {
            std::uint8_t const c = data[i];
            if(c < 0x80)
            {
                emit(c);
                ++i;
                continue;
            }

            std::size_t length = 0;
            std::uint32_t codepoint = 0;
            std::uint32_t minimum = 0;
            if(c >= 0xC2 && c <= 0xDF) { length = 2; codepoint = c & 0x1F; minimum = 0x80; }
            else if(c >= 0xE0 && c <= 0xEF) { length = 3; codepoint = c & 0x0F; minimum = 0x800; }
            else if(c >= 0xF0 && c <= 0xF4) { length = 4; codepoint = c & 0x07; minimum = 0x10000; }
            else
            {
                if(!invalid()) return false;
                ++i;
                continue;
            }

            if(data.size() - i < length)
            {
                bool prefix = true;
                for(std::size_t j = i + 1; j < data.size(); ++j)
                    prefix = prefix && (data[j] & 0xC0) == 0x80;
                if(stream && prefix)
                {
                    state.pending.assign(data.begin() + i, data.end());
                    break;
                }
                if(!invalid()) return false;
                i = prefix ? data.size() : i + 1;
                continue;
            }

            bool continuation = true;
            for(std::size_t j = 1; j < length; ++j)
            {
                continuation = continuation && (data[i + j] & 0xC0) == 0x80;
                codepoint = (codepoint << 6) | (data[i + j] & 0x3F);
            }
            if(!continuation || codepoint < minimum || codepoint > 0x10FFFF ||
               (codepoint >= 0xD800 && codepoint <= 0xDFFF))
            {
                if(!invalid()) return false;
                ++i;
                continue;
            }

            emit(codepoint);
            i += length;
        }

        if(!stream)
        {
            if(!state.pending.empty())
            {
                state.pending.clear();
                if(!invalid()) return false;
            }
            state.bom_seen = false;
        }
        return true;
    }

    JSValue decode_0(JSContext *ctx)
    {
        std::vector<std::uint8_t> input;
        std::vector<std::uint8_t> output;
        if(!decode_utf8(input, false, output))
            return JS_ThrowTypeError(ctx, "The encoded data was not valid UTF-8");
        return JS_NewStringLen(ctx, reinterpret_cast<char const *>(output.data()), output.size());
    }

    JSValue decode_impl(JSContext *ctx, bridge::Value input, bool stream)
    {
        if(JS_IsUndefined(input))
        {
            std::vector<std::uint8_t> bytes;
            std::vector<std::uint8_t> output;
            if(!decode_utf8(bytes, stream, output))
            {
                ref().pending.clear();
                ref().bom_seen = false;
                return JS_ThrowTypeError(ctx, "The encoded data was not valid UTF-8");
            }
            return JS_NewStringLen(ctx, reinterpret_cast<char const *>(output.data()), output.size());
        }

        std::size_t offset = 0;
        std::size_t length = 0;
        JSValue buffer = JS_GetTypedArrayBuffer(ctx, input, &offset, &length, nullptr);
        if(JS_IsException(buffer))
        {
            JS_FreeValue(ctx, JS_GetException(ctx));
            std::size_t direct_size;
            if(JS_GetArrayBuffer(ctx, &direct_size, input))
            {
                offset = 0;
                length = direct_size;
                buffer = JS_DupValue(ctx, input);
            }
            else
            {
                JS_FreeValue(ctx, JS_GetException(ctx));
                JSValue glob = JS_GetGlobalObject(ctx);
                JSValue ctor = JS_GetPropertyStr(ctx, glob, "DataView");
                int const is_dataview = JS_IsInstanceOf(ctx, input, ctor);
                JS_FreeValue(ctx, ctor);
                JS_FreeValue(ctx, glob);
                if(is_dataview < 0) return JS_EXCEPTION;
                if(!is_dataview) return JS_ThrowTypeError(ctx, "decode input must be a BufferSource");

                buffer = JS_GetPropertyStr(ctx, input, "buffer");
                JSValue offset_value = JS_GetPropertyStr(ctx, input, "byteOffset");
                JSValue length_value = JS_GetPropertyStr(ctx, input, "byteLength");
                std::uint64_t view_offset;
                std::uint64_t view_length;
                bool const failed = JS_IsException(buffer) || JS_IsException(offset_value) ||
                    JS_IsException(length_value) || JS_ToIndex(ctx, &view_offset, offset_value) ||
                    JS_ToIndex(ctx, &view_length, length_value);
                JS_FreeValue(ctx, offset_value);
                JS_FreeValue(ctx, length_value);
                if(failed)
                {
                    JS_FreeValue(ctx, buffer);
                    return JS_EXCEPTION;
                }
                if(view_offset > std::numeric_limits<std::size_t>::max() ||
                   view_length > std::numeric_limits<std::size_t>::max())
                {
                    JS_FreeValue(ctx, buffer);
                    return JS_ThrowRangeError(ctx, "DataView range is too large");
                }
                offset = static_cast<std::size_t>(view_offset);
                length = static_cast<std::size_t>(view_length);
            }
        }

        std::size_t size;
        std::uint8_t *data = JS_GetArrayBuffer(ctx, &size, buffer);
        if(!data)
        {
            JS_FreeValue(ctx, buffer);
            return JS_EXCEPTION;
        }
        if(offset > size || length > size - offset)
        {
            JS_FreeValue(ctx, buffer);
            return JS_ThrowRangeError(ctx, "BufferSource range exceeds its ArrayBuffer");
        }
        std::vector<std::uint8_t> bytes(data + offset, data + offset + length);
        JS_FreeValue(ctx, buffer);

        std::vector<std::uint8_t> output;
        if(!decode_utf8(bytes, stream, output))
        {
            ref().pending.clear();
            ref().bom_seen = false;
            return JS_ThrowTypeError(ctx, "The encoded data was not valid UTF-8");
        }
        return JS_NewStringLen(ctx, reinterpret_cast<char const *>(output.data()), output.size());
    }

    JSValue decode_1(JSContext *ctx, bridge::Value input)
    {
        return decode_impl(ctx, input, false);
    }

    JSValue decode_2(JSContext *ctx, bridge::Value input, DecodeOptions options)
    {
        bool stream = false;
        if(auto value = options.get<bridge::Boolean>("stream"); value) stream = *value;
        return decode_impl(ctx, input, stream);
    }

    using decode = bridge::Function<&TextDecoder::decode_0, &TextDecoder::decode_1, &TextDecoder::decode_2>;
    using ctor = bridge::Constructor<TextDecoder(), TextDecoder(Encoding), TextDecoder(Encoding, Options)>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const TextDecoder::funcs[] = {
    JS_CGETSET_DEF("encoding", &bridge::Getter<&TextDecoder::get_encoding>, nullptr),
    JS_CGETSET_DEF("fatal", &bridge::Getter<&TextDecoder::get_fatal>, nullptr),
    JS_CGETSET_DEF("ignoreBOM", &bridge::Getter<&TextDecoder::get_ignoreBOM>, nullptr),
    JS_CFUNC_DEF("decode", 1, &TextDecoder::decode::invoke)
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

JSValue require_1(JSContext *ctx, bridge::String name)
{
    Global *glob = Global::ptr(ctx);
    auto const n = static_cast<std::string>(name);

    static std::unordered_map<std::string, JSValue(*)(JSContext *)> const scripts = {
#define SCRIPT(name) {#name, &notojs_init_##name}
        SCRIPT(console),
        SCRIPT(crypto),
        SCRIPT(dollar),
        SCRIPT(dom),
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
        if(lmdb::val v; db.get(tx, k, v) && detail::Module::is_script(v))
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

JSValue require_2(JSContext *ctx, bridge::String name, ScriptConfig config)
{
    auto const n = static_cast<std::string>(name);

    static std::unordered_map<std::string, JSValue(*)(JSContext *, ScriptConfig)> const scripts = {
#define SCRIPT(name) {#name, &notojs_init_##name}
        SCRIPT(console),
            SCRIPT(crypto),
        SCRIPT(dollar),
        SCRIPT(dom),
        SCRIPT(storage)
#undef SCRIPT
    };

    if(auto it = scripts.find(n); it != std::end(scripts))
        return it->second(ctx, config);

    return JS_ThrowReferenceError(ctx, "Module %s not found", n.data());
}

using require = bridge::Function<&require_1, &require_2>;

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
    File::init();
    FormData::init();
    Headers::init();
    ServerRequest::init();
    Request::init();
    ServerResponse::init();
    Response::init();
    Storage::init();
    TextDecoder::init();
    TextEncoder::init();
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
    File::init(rt);
    FormData::init(rt);
    Headers::init(rt);
    ServerRequest::init(rt);
    Request::init(rt);
    ServerResponse::init(rt);
    Response::init(rt);
    Storage::init(rt);
    TextDecoder::init(rt);
    TextEncoder::init(rt);
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

        JSValue r = JS_NewCFunction(ctx, &require::invoke, "require", 1);
        JS_SetPropertyStr(ctx, r, "script", JS_NewCFunction(ctx, &script::invoke, "script", 1));
        JS_SetPropertyStr(ctx, glob, "require", r);

        JS_SetPropertyStr(ctx, d, "__renderer", JS_NewCFunction(ctx, &bridge::Function<renderer>::invoke, NULL, 0));

        if(!jsapp)
        {
            ServerRequest::init(ctx);
            ServerResponse::init(ctx, glob);
        }

        Blob::init(ctx, glob);
        File::init(ctx, glob);
        FormData::init(ctx, glob);
        Headers::init(ctx, glob);
        Request::init(ctx, glob);
        Response::init(ctx, glob);
        Storage::init(ctx, glob);
        TextDecoder::init(ctx, glob);
        TextEncoder::init(ctx, glob);
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

JSValue Global::Context::load(JSContext *ctx, char const *name)
{
    std::string base{"<require:"};
    base.append(name);
    base.append(">");

    JSValue mod = JS_LoadModule(ctx, base.c_str(), name);
    if(JS_IsException(mod)) return mod;

    wait(ctx);
    auto const state = JS_PromiseState(ctx, mod);
    if(JS_PROMISE_REJECTED == state)
    {
        JSValue error = JS_PromiseResult(ctx, mod);
        if(auto it = std::find_if(std::begin(rejections), std::end(rejections), [ctx, mod](auto const &entry) {
            return JS_StrictEq(ctx, entry.promise, mod);
        }); it != std::end(rejections))
        {
            JS_FreeValue(ctx, it->promise);
            JS_FreeValue(ctx, it->reason);
            rejections.erase(it);
        }
        JS_FreeValue(ctx, mod);
        return JS_Throw(ctx, error);
    }
    if(JS_PROMISE_FULFILLED != state)
    {
        JS_FreeValue(ctx, mod);
        return JS_ThrowInternalError(ctx, JS_PROMISE_PENDING == state
            ? "Module %s did not finish loading"
            : "Module %s did not return a promise", name);
    }

    JSValue res = JS_PromiseResult(ctx, mod);
    JS_FreeValue(ctx, mod);
    return res;
}

void Global::Context::free(JSContext *ctx)
{
    JS_FreeValue(ctx, output);
    JSValue glob = JS_GetGlobalObject(ctx);
    for(auto const &name : cleanup)
    {
        if("$." == name.substr(0, 2))
        {
            JSAtom atom = JS_NewAtom(ctx, name.c_str() + 2);
            JSValue dollar = JS_GetPropertyStr(ctx, glob, "$");
            JS_DeleteProperty(ctx, dollar, atom, 0);
            JS_FreeValue(ctx, dollar);
            JS_FreeAtom(ctx, atom);
        }
        else
        {
            JSAtom atom = JS_NewAtom(ctx, name.c_str());
            JS_DeleteProperty(ctx, glob, atom, 0);
            JS_FreeAtom(ctx, atom);
        }

    }
    JS_FreeValue(ctx, glob);
    if(perror)
    {
        JS_FreeValue(ctx, *perror);
        perror.reset();
    }
    for(auto const &entry : rejections)
    {
        JS_FreeValue(ctx, entry.promise);
        JS_FreeValue(ctx, entry.reason);
    }
    rejections.clear();
}

void Global::set_agent(std::string &&agent) const
{
    Request_::agent = agent;
}

void Global::set_prefix(std::string &&prefix) const
{
    Request::HTTPString::prefix = std::move(prefix);
}

void Global::configure(detail::Config const &cfg)
{
    if(auto agent = cfg.get_optional<std::string>("global.agent"); agent)
        Request_::agent = *agent;

    if(auto prefix = cfg.get_optional<std::string>("global.prefix"); prefix)
        Request::HTTPString::prefix = *prefix;
    else
        Request::HTTPString::prefix = "http://" + cfg.get<std::string>("server.bind");

    Request_::local = cfg.get<std::string>("server.bind");
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
    JSValue value = JS_UNDEFINED;
    auto const action = then(ctx, value);
    if(action || JS_IsException(value))
    {
        settle_async(ctx, funcs, value);
    }
    else
    {
        JSValue settled = JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, &value);
        JS_FreeValue(ctx, value);
        if(JS_IsException(settled))
        {
            JSValue error = JS_GetException(ctx);
            if(auto *context = Global::Context::ptr(ctx); context && !context->perror)
                context->perror = error;
            else
                JS_FreeValue(ctx, error);
        }
        else
        {
            JS_FreeValue(ctx, settled);
        }
    }
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
        std::string u{*notojs::Request::HTTPString::prefix};
        u.append(uri);
        if(auto uv = boost::urls::parse_uri(u))
            return std::move(*uv);
    }
    else if(auto uv = boost::urls::parse_uri(uri))
    {
        boost::urls::url url{std::move(*uv)};
        Request_::maybenoto(url);
        return std::move(url);
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

JSValue IFile::Static::make(JSContext *ctx, std::vector<std::uint8_t> &&data, std::string const &name, std::int64_t time, std::string const &type)
{
    auto const size = data.size();
    JSValue blob = File::from(ctx, File_{
        Blob_{
            .type = type,
            .data = Blob_::from(std::move(data)),
            .size = size
        },
        .name = name,
        .last_modified = time
    });
    File::get(blob).dptr = size
        ? &Blob::get(blob).data->at(0)
        : nullptr;
    return blob;
}

JSValue IFile::Static::make(JSContext *ctx, std::uint8_t const *data, std::size_t size, std::string const &name, std::int64_t time, std::string const &type)
{
    JSValue blob = File::from(ctx, File_{
        Blob_{
            .type = type,
            .data = Blob_::from(std::vector<std::uint8_t>(data, data + size)),
            .size = size
        },
        .name = name,
        .last_modified = time
    });
    File::get(blob).dptr = size
        ? &File::get(blob).data->at(0)
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
        boost::beast::http::response<boost::beast::http::string_body> const &),
    std::chrono::milliseconds timeout)
{
    using Callback = decltype(callback);
    JSValue cb = JS_NewObjectClass(ctx, callback_id);
    JS_SetOpaque(cb, (void*)callback);

    Request_::maybenoto(url);
    Request_ request_{std::move(request), std::move(url)};
    request_.timeout = timeout;
    auto req = bridge::Strong<bridge::Object>{ctx, Request::from(ctx, std::move(request_))};
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

JSValue facade::import(JSContext *ctx, char const *name)
{
    return Global::Context::ptr(ctx)->load(ctx, name);
}

} // namespace notojs
