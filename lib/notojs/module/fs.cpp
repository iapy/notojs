#include "quickjs/quickjs.h"
#include <notojs/module/fs.hpp>
#include <bridge.hpp>
#include <engine.hpp>
#include <global.hpp>
#include <module.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <map>
#include <stdexcept>

namespace notojs {
namespace {

struct Mount
{
    enum Flag
    {
        UNDEFINED = 0,
        RO = 1,
        RW = 2
    };

    std::filesystem::path path;
    Flag flag;

    BOOST_FORCEINLINE static Flag parse(std::string_view const &flag)
    {
        return flag == "rw" ? RW : (flag == "ro" ? RO : UNDEFINED);
    }
};

std::map
<
    std::filesystem::path, Mount,
    std::greater<std::filesystem::path>
> mount;

JSValue mounts(JSContext *ctx)
{
    bridge::Array a{ctx};
    for(auto const &[k, m]: mount)
    {
        bridge::Object o{ctx};
        o.set("path", bridge::String(ctx, k));
        o.set("flag", bridge::String(ctx, std::string_view{m.flag == Mount::RW ? "rw" : "ro"}));
        a.append(o);
    }
    return a;
}

class FileSystemException : public bridge::Exception<FileSystemException>
{
    std::error_code ec;
public:
    FileSystemException(std::error_code &&ec): ec{ec} {}
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        obj.set("code", bridge::Number(ctx, ec.value()));
        obj.set("message", bridge::String(ctx, ec.message()));
    }
};

template<typename Data>
class Read : public notojs::Task
{
public:
    Read(std::ifstream &&stream)
    : stream{std::move(stream)} {}

protected:
    Step step() override
    {
        stream.read(&buffer[0], sizeof(buffer));
        data.insert(data.end(),
            reinterpret_cast<std::uint8_t const *>(&buffer[0]),
            reinterpret_cast<std::uint8_t const *>(&buffer[stream.gcount()]));
        return stream ? Again : (stream.close(), Finish);
    }

    Then then(JSContext *, JSValue &) override;

private:
    std::ifstream stream;
    std::vector<std::uint8_t> data;
};

template<>
class Read<std::pair<std::vector<std::uint8_t>, std::string>> : public notojs::Task
{
public:
    Read(std::ifstream &&stream, std::string &&mime)
    : stream{std::move(stream)}, mime{std::move(mime)} {}

protected:
    Step step() override
    {
        stream.read(&buffer[0], sizeof(buffer));
        data.insert(data.end(),
            reinterpret_cast<std::uint8_t const *>(&buffer[0]),
            reinterpret_cast<std::uint8_t const *>(&buffer[stream.gcount()]));
        return stream ? Again : (stream.close(), Finish);
    }

    Then then(JSContext *ctx, JSValue &v) override
    {
        return (v = facade::Blob::make(ctx, std::move(data), mime)), Resolve;
    }

private:
    std::ifstream stream;
    std::string const mime;
    std::vector<std::uint8_t> data;
};

template<typename Data>
class Write : public notojs::Task
{
public:
    Write(std::ofstream &&stream, Data &&data)
    : stream{std::move(stream)}, data{data} {}

    Step step() override;

    Then then(JSContext *ctx, JSValue &v) override
    {
        v = bridge::Number{ctx, static_cast<std::int64_t>(pos)};
        return (stream.close(), Resolve);
    }

private:
    std::ofstream stream;
    std::size_t pos{0};
    Data const data;
};

template<>
notojs::Task::Then Read<bridge::Object>::then(JSContext *ctx, JSValue &v)
{
    if(data.empty() || data.back()) data.push_back(0);
    v = JS_ParseJSON(ctx, reinterpret_cast<char *>(&data[0]), data.size() - 1, "<json>");
    if(JS_IsException(v)) {
        v = JS_GetException(ctx);
        return Reject;
    }
    return Resolve;
}

template<>
notojs::Task::Then Read<std::vector<std::uint8_t>>::then(JSContext *ctx, JSValue &v)
{
    return (v = facade::Blob::make(ctx, std::move(data))), Resolve;
}

template<>
notojs::Task::Then Read<std::string>::then(JSContext *ctx, JSValue &v)
{
    return (v = bridge::String(ctx, std::string_view{reinterpret_cast<char *>(&data[0]), data.size()})), Resolve;
}

template<>
notojs::Task::Step Write<std::string>::step()
{
    std::size_t const end = pos + sizeof(buffer);
    std::size_t const cnt = std::min(end - pos, data.size() - pos);
    stream.write(data.c_str() + pos, cnt);
    return (pos += cnt) == data.size() ? Finish : Again;
}

template<>
notojs::Task::Step Write<std::shared_ptr<std::vector<std::uint8_t>>>::step()
{
    std::size_t const end = pos + sizeof(buffer);
    std::size_t const cnt = std::min(end - pos, data->size() - pos);
    stream.write(reinterpret_cast<char*>(&(*data)[pos]), cnt);
    return (pos += cnt) == data->size() ? Finish : Again;
}

struct Path : bridge::Interface<Path, std::filesystem::path>
{
    using Base::Base;

    Path(JSContext *ctx, JSValue val)
    : Base{ctx, val} {}

    struct Absolute : bridge::String
    {
        using bridge::String::String;
        static bool valid(JSContext *ctx, JSValue *value, std::string &message)
        {
            auto p = std::filesystem::path{Absolute{ctx, *value}}.lexically_normal();
            if(!p.is_absolute()) return (message = "Expecting absolute path", false);

            if(normalize<bool>(p)) return true;
            return (message = "Not mounted", false);
        }

        template<typename T>
        static T normalize(std::filesystem::path const &p)
        {
            for(auto const &[q, c]: mount)
            {
                auto const r = p.lexically_relative(q);
                if(auto s = r.u8string(); '.' != s[0] || 1 == s.size())
                {
                    if constexpr (std::is_same_v<T, bool>) return true;
                    else return {(c.path / r).lexically_normal(), '.' == s[0] ? Mount::RO : c.flag};
                }
            }
            if constexpr (std::is_same_v<T, bool>) return false;
            else return {std::filesystem::path{}, Mount::UNDEFINED};
        }

        BOOST_FORCEINLINE operator std::filesystem::path() const
        {
            return static_cast<std::string_view const &>(*this);
        }
    };

    struct Iterator : std::filesystem::directory_iterator
    {
        Iterator() = default;

        Iterator(std::filesystem::directory_iterator &&base, std::filesystem::path const &path)
        : std::filesystem::directory_iterator(std::move(base)), path{path} {}

        BOOST_FORCEINLINE JSValue get(JSContext *ctx) const
        {
            return Path::from(ctx, path / (*this)->path().filename());
        }

        BOOST_FORCEINLINE Iterator &operator ++ ()
        {
            std::filesystem::directory_iterator::operator ++ ();
            return *this;
        }
    private:
        std::filesystem::path path;
    };

    JSValue append(JSContext *ctx, bridge::String p)
    {
        auto path = (ref() / static_cast<std::string_view const &>(p)).lexically_normal();
        if(!Absolute::normalize<bool>(path)) return JS_ThrowTypeError(ctx, "Invalid path %s", path.u8string().c_str());
        return Path::from(ctx, std::move(path));
    }

    JSValue blob_0(JSContext *ctx)
    {
        auto [path, _] = Absolute::normalize<Mount>(ref());

        std::error_code ec;
        if(std::filesystem::is_regular_file(path, ec))
            return std::make_shared<Read<std::vector<std::uint8_t>>>(std::ifstream(path, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    JSValue blob_1(JSContext *ctx, bridge::String mime)
    {
        auto [path, _] = Absolute::normalize<Mount>(ref());

        std::error_code ec;
        if(std::filesystem::is_regular_file(path, ec))
            return std::make_shared<Read<std::pair<std::vector<std::uint8_t>, std::string>>>(std::ifstream(path, std::ios_base::in | std::ios_base::binary), mime)->run(ctx);
        else if(ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    using blob = bridge::Function<&Path::blob_0, &Path::blob_1>;

    using Check = bool(std::filesystem::path const &, std::error_code&) noexcept;

    template<Check* fn>
    JSValue check_(JSContext *ctx) const
    {
        std::error_code ec;
        auto [path, _] = Absolute::normalize<Mount>(ref());
        if (bool b = fn(path, ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return b ? JS_TRUE : JS_FALSE;
    }

    struct CopyOptions : bridge::Struct<CopyOptions>
    {
        BRIDGE_DEFINE_STRUCT(CopyOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("overwrite")
        );
    };

    template<std::filesystem::copy_options opts>
    JSValue copy_0(JSContext *ctx, Path other) const
    {
        std::error_code ec;
        auto [path, _] = Absolute::normalize<Mount>(ref());
        auto [that, g] = Absolute::normalize<Mount>(other.ref());

        if (!(g & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));
        if (std::filesystem::copy(path, that, opts, ec); ec) return FileSystemException::throw_(ctx, std::move(ec));

        return JS_UNDEFINED;
    }

    JSValue copy_1(JSContext *ctx, Path other, CopyOptions opts) const
    {
        if(auto o = opts.get<bridge::Boolean>("overwrite"); o && *o)
            return copy_0<std::filesystem::copy_options::overwrite_existing>(ctx, std::move(other));
        return copy_0<std::filesystem::copy_options::none>(ctx, std::move(other));
    }

    using copy = bridge::Function
    <
        &Path::copy_0<std::filesystem::copy_options::none>,
        &Path::copy_1
    >;

    JSValue extension(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().extension().u8string());
    }

    JSValue filename(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().filename().u8string());
    }

    JSValue json(JSContext *ctx)
    {
        auto [path, _] = Absolute::normalize<Mount>(ref());

        std::error_code ec;
        if(std::filesystem::is_regular_file(path, ec))
            return std::make_shared<Read<bridge::Object>>(std::ifstream(path, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    JSValue move(JSContext *ctx, Path other) const
    {
        std::error_code ec;
        auto [path, f] = Absolute::normalize<Mount>(ref());
        auto [that, g] = Absolute::normalize<Mount>(other.ref());

        if (!(g & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));
        if (!(f & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));
        if (std::filesystem::rename(path, that, ec); ec) return FileSystemException::throw_(ctx, std::move(ec));

        return JS_UNDEFINED;
    }

    struct DirectoryOptions : bridge::Struct<DirectoryOptions>
    {
        BRIDGE_DEFINE_STRUCT(DirectoryOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("recursive")
        );
    };

    JSValue mkdir_0(JSContext *ctx) const
    {
        std::error_code ec;
        auto [path, flag] = Absolute::normalize<Mount>(ref());
        if (!(flag & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        if (auto result = std::filesystem::create_directory(path, ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return result ? JS_TRUE : JS_FALSE;
    }

    JSValue mkdir_1(JSContext *ctx, DirectoryOptions opts) const
    {
        if(auto o = opts.get<bridge::Boolean>("recursive"); o && *o)
        {
            std::error_code ec;
            auto [path, flag] = Absolute::normalize<Mount>(ref());
            if (!(flag & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

            if (auto result = std::filesystem::create_directories(path, ec); ec)
                return FileSystemException::throw_(ctx, std::move(ec));
            else return result ? JS_TRUE : JS_FALSE;
        }
        return mkdir_0(ctx);
    }

    using mkdir = bridge::Function
    <
        &Path::mkdir_0,
        &Path::mkdir_1
    >;

    JSValue native(JSContext *ctx) const
    {
        auto [path, _] = Absolute::normalize<Mount>(ref());
        return bridge::String(ctx, path.u8string());
    }

    JSValue parent(JSContext *ctx) const
    {
        auto path = ref().parent_path();
        if(!Absolute::normalize<bool>(path)) return JS_ThrowTypeError(ctx, "Invalid path %s", path.u8string().c_str());
        return Path::from(ctx, std::move(path));
    }

    JSValue text(JSContext *ctx)
    {
        auto [path, _] = Absolute::normalize<Mount>(ref());

        std::error_code ec;
        if(std::filesystem::is_regular_file(path, ec))
            return std::make_shared<Read<std::string>>(std::ifstream(path, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    JSValue relative(JSContext *ctx, Path path)
    {
        std::error_code ec;
        if(auto const p = std::filesystem::relative(ref(), path.ref(), ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return bridge::String(ctx, p.u8string());
    }

    JSValue remove_0(JSContext *ctx) const
    {
        std::error_code ec;
        auto [path, flag] = Absolute::normalize<Mount>(ref());

        if (!(flag & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        if (std::filesystem::remove(path, ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return JS_UNDEFINED;
    }

    JSValue remove_1(JSContext *ctx, DirectoryOptions opts) const
    {
        if(auto o = opts.get<bridge::Boolean>("recursive"); o && *o)
        {
            std::error_code ec;
            auto [path, flag] = Absolute::normalize<Mount>(ref());

            if (!(flag & Mount::RW)) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

            if (auto count = std::filesystem::remove_all(path, ec); ec)
                return FileSystemException::throw_(ctx, std::move(ec));
            else return bridge::Number{ctx, static_cast<std::int64_t>(count)};
        }
        return remove_0(ctx);
    }

    using remove = bridge::Function
    <
        &Path::remove_0,
        &Path::remove_1
    >;

    JSValue size(JSContext *ctx) const
    {
        std::error_code ec;
        auto [path, _] = Absolute::normalize<Mount>(ref());
        if (auto s = std::filesystem::file_size(path, ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return bridge::Number(ctx, static_cast<std::int64_t>(s));
    }

    JSValue sym(JSValue self, JSContext *ctx)
    {
        std::error_code ec;
        auto [path, _] = Absolute::normalize<Mount>(ref());
        if(auto it = std::filesystem::directory_iterator(path, ec); ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else return bridge::Iterator<Iterator>::make(ctx, self, Iterator(std::move(it), ref()), Iterator());
    }

    JSValue toJSON(JSContext *ctx) const
    {
        return bridge::String(ctx, ref().u8string());
    }

    template<typename T>
    JSValue writer(JSContext *ctx, std::ofstream &&stream, T &&data);

    template<typename T, std::ios_base::openmode mode>
    JSValue write_0(JSContext *ctx, T data)
    {
        std::error_code ec;
        auto [path, flag] = Absolute::normalize<Mount>(ref());

        if(!(flag & Mount::RW))
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        if(!std::filesystem::exists(path) || std::filesystem::is_regular_file(path, ec))
            return writer(ctx, std::ofstream(path, mode), std::move(data));
        else if(ec)
            return FileSystemException::throw_(ctx, std::move(ec));
        else
            return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    struct WriteOptions : bridge::Struct<WriteOptions>
    {
        BRIDGE_DEFINE_STRUCT(WriteOptions);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("append")
        );
    };

    template<typename T, std::ios_base::openmode mode>
    JSValue write_1(JSContext *ctx, T data, WriteOptions opts)
    {
        if(auto a = opts.get<bridge::Boolean>("append"); a && *a)
            return write_0<T, mode | std::ios_base::app>(ctx, std::move(data));
        return write_0<T, mode>(ctx, std::move(data));
    }

    using write = bridge::Function
    <
        &Path::write_0<bridge::String, std::ios_base::out | std::ios_base::binary>,
        &Path::write_0<facade::Blob,   std::ios_base::out | std::ios_base::binary>,
        &Path::write_1<bridge::String, std::ios_base::out | std::ios_base::binary>,
        &Path::write_1<facade::Blob,   std::ios_base::out | std::ios_base::binary>
    >;

    struct I : Base::I<I, fs::IPath>
    {
        std::pair<std::filesystem::path, bool> native() const override
        {
            auto [path, flag] = Absolute::normalize<Mount>(ref);
            return {std::move(path), flag & Mount::RW};
        }
        using Base::Base;
    };

    using ctor = bridge::Unconstructable<Path>;
    using impl = bridge::Implements<I>;
    using priv = bridge::Private
    <
        bridge::Iterator<Iterator>
    >;
    static JSCFunctionListEntry const funcs[];
};

template<>
JSValue Path::writer<bridge::String>(JSContext *ctx, std::ofstream &&stream, bridge::String &&data)
{
    if(!stream.is_open()) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    return std::make_shared<Write<std::string>>(std::move(stream), std::move(data))->run(ctx);
}

template<>
JSValue Path::writer<notojs::IBlob::Impl>(JSContext *ctx, std::ofstream &&stream, notojs::IBlob::Impl &&data)
{
    if(!stream.is_open()) return FileSystemException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    return std::make_shared<Write<std::shared_ptr<std::vector<uint8_t>>>>(std::move(stream), data->copy())->run(ctx);
}

JSCFunctionListEntry const Path::funcs[] = {
#define FILESYSTEM_CHECK(n) JS_CGETSET_DEF(#n, &bridge::Getter<&Path::check_<std::filesystem::n>>, NULL),
    FILESYSTEM_CHECK(exists)
    FILESYSTEM_CHECK(is_directory)
    FILESYSTEM_CHECK(is_fifo)
    FILESYSTEM_CHECK(is_regular_file)
    FILESYSTEM_CHECK(is_socket)
    FILESYSTEM_CHECK(is_symlink)
#undef FILESYSTEM_CHECK
    JS_CGETSET_DEF("extension", &bridge::Getter<&Path::extension>, NULL),
    JS_CGETSET_DEF("filename", &bridge::Getter<&Path::filename>, NULL),
    JS_CGETSET_DEF("native", &bridge::Getter<&Path::native>, NULL),
    JS_CGETSET_DEF("parent", &bridge::Getter<&Path::parent>, NULL),
    JS_CGETSET_DEF("size", &bridge::Getter<&Path::size>, NULL),

    JS_CFUNC_DEF("[Symbol.iterator]", 0, &bridge::Function<&Path::sym>::invoke),
    JS_CFUNC_DEF("append", 1, &bridge::Function<&Path::append>::invoke),
    JS_CFUNC_DEF("blob", 0, &Path::blob::invoke),
    JS_CFUNC_DEF("copy", 0, &Path::copy::invoke),
    JS_CFUNC_DEF("json", 0, &bridge::Function<&Path::json>::invoke),
    JS_CFUNC_DEF("mkdir", 1, &Path::mkdir::invoke),
    JS_CFUNC_DEF("move", 1, &bridge::Function<&Path::move>::invoke),
    JS_CFUNC_DEF("relative", 1, &bridge::Function<&Path::relative>::invoke),
    JS_CFUNC_DEF("remove", 0, &Path::remove::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&Path::text>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<Path>::toJSON),
    JS_CFUNC_DEF("write", 1, &Path::write::invoke)
};

JSValue path(JSContext *ctx, Path::Absolute p)
{
    return Path::from(ctx, std::filesystem::path(p));
}

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("mounts", 1, bridge::Function<mounts>::invoke),
    JS_CFUNC_DEF("path", 0, bridge::Function<path>::invoke),
};

int init(JSContext *ctx, JSModuleDef *m)
{
    fs::init(ctx);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_fs()
{
    Path::init();
}

void notojs_init_fs(JSRuntime *rt)
{
    Path::init(rt);
}

void notojs_init_fs(boost::property_tree::ptree const &pt)
{
    if(auto config = pt.get_child_optional("module:fs"))
    {
        for(auto const &[k, v]: *config) if(!k.empty() && k[0] == '/')
        {
            auto const &s = v.get_value<std::string>();
            auto const p = s.rfind(',');

            auto const f = p == std::string::npos ? Mount::RO : Mount::parse(s.substr(p + 1));
            if(f == Mount::UNDEFINED) throw std::invalid_argument("Unexpected mount mode: " + s.substr(p + 1));

            mount.insert({k, Mount{
                .path = s.substr(0, p),
                .flag = f
            }});
        }
    }
}

JSModuleDef *notojs_init_fs(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

void fs::init(JSContext *ctx)
{
    auto p = JS_GetClassProto(ctx, Path::cid);
    if(JS_IsNull(p)) Path::init(ctx);
    JS_FreeValue(ctx, p);
}

JSValue fs::IPath::Static::make(JSContext *ctx, std::filesystem::path &&p)
{
    if(!p.is_absolute())
        return JS_ThrowTypeError(ctx, "Expecting absolute path");
    if(auto [path, mode] = notojs::Path::Absolute::normalize<notojs::Mount>(p); notojs::Mount::UNDEFINED != mode)
        return notojs::Path::from(ctx, std::move(p));
    return JS_ThrowTypeError(ctx, "Not mounted");
}

std::filesystem::path fs::IPath::Static::v2n(std::filesystem::path const &p)
{
    if(!p.is_absolute())
        throw std::runtime_error("Not absolute: " + p.u8string());

    auto [path, mode] = Path::Absolute::normalize<Mount>(p.lexically_normal());
    if(Mount::UNDEFINED == mode)
        throw std::runtime_error("Not mounted: " + p.u8string());
    return path;
}

std::filesystem::path fs::IPath::Static::n2v(std::filesystem::path const &p)
{
    if(!p.is_absolute())
        throw std::runtime_error("Not absolute: " + p.u8string());

    auto const path = p.lexically_normal();
    for(auto const &[v, m]: mount)
    {
        auto const root = m.path.lexically_normal();
        auto const r = path.lexically_relative(root);
        if(auto s = r.u8string(); '.' != s[0] || 1 == s.size())
            return (v / r).lexically_normal();
    }

    throw std::runtime_error("Not mounted: " + p.u8string());
}

} // namespace notojs
