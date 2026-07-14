#include <notojs/module/zip.hpp>
#include <bridge.hpp>
#include <global.hpp>
#include <engine.hpp>
#include <module.hpp>
#include <zip.h>

#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace notojs {
namespace {

class ZipException : public bridge::Exception<ZipException>
{
    std::variant<std::error_code, std::string> ec;
public:
    ZipException(std::string &&ec): ec{ec} {}
    ZipException(std::error_code &&ec): ec{ec} {}
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        if(std::holds_alternative<std::error_code>(ec))
        {
            obj.set("code", bridge::Number(ctx, std::get<std::error_code>(ec).value()));
            obj.set("message", bridge::String(ctx, std::get<std::error_code>(ec).message()));
        }
        else
        {
            obj.set("message", bridge::String(ctx, std::get<std::string>(ec)));
        }
    }
};

class Read : public notojs::Task
{
public:
    Read(std::filesystem::path const &p, std::unordered_set<std::string> &&extract)
    : handle{::zip_open(p.string().c_str(), ZIP_RDONLY, NULL)}
    , count{::zip_get_num_entries(handle, 0)}
    , extract{std::move(extract)}
    {}

    Step step() override
    {
        if(!handle || current == count) return Finish;
        if(const char* name = ::zip_get_name(handle, current, 0); name && (extract.empty() || extract.count(name)))
        {
            ::zip_file* zf = ::zip_fopen(handle, name, 0);
            if(zf)
            {
                ::zip_int64_t len;
                while ((len = ::zip_fread(zf, buffer, sizeof(buffer))) > 0) {
                    auto &blob = entries[name];
                    blob.insert(blob.end(),
                        reinterpret_cast<std::uint8_t const *>(&buffer[0]),
                        reinterpret_cast<std::uint8_t const *>(&buffer[len]));
                }
                ::zip_fclose(zf);
            }
        }
        return (count == ++current) ? Finish : Again;
    }

    Then then(JSContext *ctx, JSValue &v) override
    {
        if(!handle)
        {
            v = ZipException::reject_(ctx, std::make_error_code(std::errc::bad_file_descriptor));
            return Reject;
        }
        else
        {
            ::zip_close(handle);
            bridge::Object obj{ctx};
            for(auto &[k, v]: entries)
                obj.set(k.c_str(), facade::Blob::make(ctx, std::move(v)));
            v = obj;
            return Resolve;
        }
    }

private:
    ::zip_t *handle;
    ::zip_int64_t current{0};
    ::zip_int64_t const count;
    std::unordered_set<std::string> extract;
    std::unordered_map<std::string, std::vector<std::uint8_t>> entries;
};

class Remove : public notojs::Task
{
public:
    Remove(std::filesystem::path const &p, std::unordered_set<std::string> &&entries)
    : handle{::zip_open(p.string().c_str(), 0, NULL)}
    , entries{std::move(entries)}
    {}

    Step step() override
    {
        if(!handle) return Finish;
        for(auto const &name : entries)
        {
            ::zip_int64_t idx = ::zip_name_locate(handle, name.c_str(), 0);
            if(idx >= 0) ::zip_delete(handle, idx);
        }
        return Finish;
    }

    Then then(JSContext *ctx, JSValue &v) override
    {
        if(!handle)
        {
            v = ZipException::reject_(ctx, std::make_error_code(std::errc::bad_file_descriptor));
            return Reject;
        }
        else
        {
            ::zip_close(handle);
            return Resolve;
        }
    }

private:
    ::zip_t *handle;
    std::unordered_set<std::string> entries;
};

class Write : public notojs::Task
{
public:
    using Entries = std::unordered_map<std::string, std::variant<
        std::shared_ptr<std::vector<std::uint8_t>>,
        std::filesystem::path,
        std::string
    >>;
    using Entry = Entries::mapped_type;

    Write(std::filesystem::path const &p, int flags, Entries &&entries)
    : handle{::zip_open(p.string().c_str(), ZIP_CREATE | flags, NULL)}
    , entries{std::move(entries)}
    {
        it = std::begin(this->entries);
    }

    Step step() override
    {
        if(it == std::end(entries)) return Finish;
        std::visit(boost::hana::overload_linearly(
            [this, &name=it->first](std::shared_ptr<std::vector<std::uint8_t>> const &data) {
                const void *buf = &((*data)[0]);
                if(::zip_source_t *source = ::zip_source_buffer(handle, buf, data->size(), 0); !source)
                {
                    errors[name] = zip_strerror(handle);
                }
                else if(int idx = ::zip_file_add(handle, name.c_str(), source, ZIP_FL_OVERWRITE); idx < 0)
                {
                    ::zip_source_free(source);
                    errors[name] = zip_strerror(handle);
                }
                else if(::zip_set_file_compression(handle, idx, ZIP_CM_DEFLATE, 0) < 0)
                {
                    errors[name] = zip_strerror(handle);
                }
            },
            [this, &name=it->first](auto const &data) {
                if constexpr (std::is_same_v<std::decay_t<decltype(data)>, std::filesystem::path>)
                {
                    if(::zip_source_t *source = ::zip_source_file(handle, data.string().c_str(), 0, 0); !source)
                    {
                        errors[name] = zip_strerror(handle);
                    }
                    else if(int idx = ::zip_file_add(handle, name.c_str(), source, ZIP_FL_OVERWRITE); idx < 0)
                    {
                        ::zip_source_free(source);
                        errors[name] = zip_strerror(handle);
                    }
                    else if(::zip_set_file_compression(handle, idx, ZIP_CM_DEFLATE, 0) < 0)
                    {
                        errors[name] = zip_strerror(handle);
                    }
                }
                else
                {
                    const void *buf = data.c_str();
                    if(::zip_source_t *source = ::zip_source_buffer(handle, buf, data.size(), 0); !source)
                    {
                        errors[name] = zip_strerror(handle);
                    }
                    else if(int idx = ::zip_file_add(handle, name.c_str(), source, ZIP_FL_OVERWRITE); idx < 0)
                    {
                        ::zip_source_free(source);
                        errors[name] = zip_strerror(handle);
                    }
                    else if(::zip_set_file_compression(handle, idx, ZIP_CM_DEFLATE, 0) < 0)
                    {
                        errors[name] = zip_strerror(handle);
                    }
                }
            }
        ), it->second);
        return (std::end(entries) == ++it) ? Finish : Again;
    }

    Then then(JSContext *ctx, JSValue &v) override
    {
        if(!handle)
        {
            v = ZipException::reject_(ctx, std::make_error_code(std::errc::bad_file_descriptor));
            return Reject;
        }
        else
        {
            ::zip_close(handle);
            if(!errors.empty())
            {
                bridge::Object obj{ctx};
                for(auto const &[k, v]: errors)
                    obj.set(k.c_str(), bridge::String(ctx, v));
                v = obj;
            }
            return Resolve;
        }
    }
private:
    zip_t *handle;
    Entries entries;
    Entries::iterator it;
    std::unordered_map<std::string, std::string> errors;
};

struct Zip : bridge::Interface<Zip, std::pair<std::filesystem::path, bool>>
{
    using Base::Base;

    JSValue read(JSContext *ctx, bridge::Tail<0, bridge::String> names)
    {
        if(!std::filesystem::exists(ref().first)) return ZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
        std::unordered_set<std::string> entries;
        for(std::size_t i = 0; i < names.size(); ++i)
        {
            entries.insert(names[i]);
        }
        return std::make_shared<Read>(ref().first, std::move(entries))->run(ctx);
    }

    JSValue remove(JSContext *ctx, bridge::Tail<1, bridge::String> names)
    {
        if(!std::filesystem::exists(ref().first)) return ZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
        if(!ref().second) return ZipException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        std::unordered_set<std::string> entries;
        for(std::size_t i = 0; i < names.size(); ++i)
        {
            entries.insert(names[i]);
        }
        return std::make_shared<Remove>(ref().first, std::move(entries))->run(ctx);
    }

    class Iterator
    {
    public:
        Iterator() = default;
        Iterator(Iterator &&other)
        : handle{other.handle}
        , count{other.count}
        , current{other.current}
        {
            other.handle = nullptr;
        }

        Iterator(Iterator const &) = delete;

        Iterator(std::filesystem::path const &p)
        : handle{::zip_open(p.string().c_str(), ZIP_RDONLY, NULL)}
        , count{handle ? ::zip_get_num_entries(handle, 0) : std::numeric_limits<zip_int64_t>::max()}
        , current{handle ? 0 : std::numeric_limits<zip_int64_t>::max()}
        {}

        Iterator& operator ++ ()
        {
            if(!handle || ++current == count)
            {
                handle = nullptr;
                current = std::numeric_limits<zip_int64_t>::max();
            }
            return *this;
        }

        BOOST_FORCEINLINE JSValue get(JSContext *ctx) const
        {
            ::zip_stat_index(handle, current, 0, &stat);
            bridge::Array arr{ctx};
            arr.set(0, bridge::String{ctx, std::string_view{stat.name}});
            arr.set(1, bridge::Number{ctx, static_cast<std::int64_t>(stat.size)});
            return arr;
        }

        ~Iterator()
        {
            if(handle) ::zip_close(handle);
        }

        bool operator == (Iterator const &other) const
        {
            return handle == other.handle && current == other.current;
        }

    private:
        zip_t *handle{nullptr};
        zip_int64_t count;
        mutable zip_stat_t stat;
        zip_int64_t current{std::numeric_limits<zip_int64_t>::max()};
    };

    JSValue sym(JSValue self, JSContext *ctx)
    {
        if(!std::filesystem::exists(ref().first)) return ZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
        return bridge::Iterator<Iterator>::make(ctx, self, Iterator(ref().first), Iterator());
    }

    struct WriteOptions : bridge::Struct<WriteOptions>
    {
        BRIDGE_DEFINE_STRUCT(WriteOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("append")
        );
    };

    template<int flags>
    JSValue write_0(JSContext *ctx, bridge::Object data)
    {
        if(!ref().second) return ZipException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        Write::Entries entries;
        for(auto it = std::begin(data); it; ++it)
        {
            if(fs::IPath::Impl::check(ctx, *it))
            {
                auto [path, _] = fs::IPath::Impl{ctx, **it}->native();
                if(!std::filesystem::exists(path)) return ZipException::throw_(ctx, "File doesn't exist at key " + std::string{it.key()});
                entries.insert(std::make_pair(it.key(), std::move(path)));
            }
            else if(IBlob::Impl::check(ctx, *it))
            {
                auto copy = IBlob::Impl{ctx, **it}->copy();
                entries.insert(std::make_pair(it.key(), std::move(copy)));
            }
            else if(bridge::String::check(ctx, *it))
            {
                auto data = bridge::String{ctx, **it};
                entries.insert(std::make_pair(it.key(), static_cast<std::string>(data)));
            }
            else return ZipException::throw_(ctx, "Unsuported entry type at key " + std::string{it.key()});
        }

        return std::make_shared<Write>(ref().first, flags, std::move(entries))->run(ctx);
    }

    JSValue write_1(JSContext *ctx, bridge::Object data, WriteOptions opts)
    {
        if(auto a = opts.get<bridge::Boolean>("append"); a && *a)
            return write_0<0>(ctx, std::move(data));
        return write_0<ZIP_TRUNCATE>(ctx, std::move(data));
    }

    JSValue toJSON(JSContext *ctx) const
    {
        zip_t *handle = ::zip_open(ref().first.string().c_str(), ZIP_RDONLY, NULL);
        if(!handle) return ZipException::throw_(ctx, std::make_error_code(std::errc::bad_file_descriptor));

        bridge::Object obj{ctx};

        zip_stat_t stat;
        zip_int64_t const s = ::zip_get_num_entries(handle, 0);
        for(zip_int64_t i = 0; i < s; ++i)
        {
            ::zip_stat_index(handle, i, 0, &stat);
            obj.set(stat.name, bridge::Number{ctx, static_cast<std::int64_t>(stat.size)});
        }

        ::zip_close(handle);
        return obj;
    }

    using write = bridge::Function
    <
        &Zip::write_0<ZIP_TRUNCATE>,
        &Zip::write_1
    >;

    using ctor = bridge::Unconstructable<Zip>;
    using priv = bridge::Private
    <
        bridge::Iterator<Iterator>
    >;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Zip::funcs[] = {
    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&Zip::sym>::invoke),
    JS_CFUNC_DEF("read", 0, &bridge::Function<&Zip::read>::invoke),
    JS_CFUNC_DEF("remove", 1, &bridge::Function<&Zip::remove>::invoke),
    JS_CFUNC_DEF("write", 1, &Zip::write::invoke),

    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Zip>::toJSON)
};

JSValue zip(JSContext *ctx, fs::facade::Path path)
{
    return Zip::from(ctx, path->native());
}

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("zip", 1, bridge::Function<zip>::invoke),
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Zip::init(ctx);
    JS_SetModuleExport(ctx, m, "default", JS_NewCFunction(ctx, bridge::Function<zip>::invoke, "zip", 1));
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_zip()
{
    Zip::init();
}

void notojs_init_zip(JSRuntime *rt)
{
    Zip::init(rt);
}

void notojs_init_zip(boost::property_tree::ptree const &) {}

JSModuleDef *notojs_init_zip(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    JS_AddModuleExport(ctx, mod, "default");
    return mod;
}

} // namespace notojs
