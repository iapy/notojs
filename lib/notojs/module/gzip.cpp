#include <notojs/module/gzip.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <filesystem>
#include <fstream>

#include <bridge.hpp>
#include <global.hpp>
#include <engine.hpp>
#include <module.hpp>

namespace notojs {
namespace {

class GZipException : public bridge::Exception<GZipException>
{
    std::error_code ec;
public:
    GZipException(std::error_code &&ec): ec{ec} {}
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
    : stream{std::move(stream)}
    {
        inp.push(boost::iostreams::gzip_decompressor());
        inp.push(this->stream);
    }

protected:
    Step step() override
    {
        inp.read(&buffer[0], sizeof(buffer));
        data.insert(data.end(),
            reinterpret_cast<std::uint8_t const *>(&buffer[0]),
            reinterpret_cast<std::uint8_t const *>(&buffer[inp.gcount()]));
        return inp ? Again : (stream.close(), Finish);
    }

    Then then(JSContext *, JSValue &) override;

private:
    std::ifstream stream;
    std::vector<std::uint8_t> data;

private:
    boost::iostreams::filtering_istream inp;
};

template<>
notojs::Task::Then Read<bridge::Object>::then(JSContext *ctx, JSValue &v)
{
    v = JS_ParseJSON(ctx, reinterpret_cast<char *>(&data[0]), data.size(), "<json>");
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

template<typename Data>
class Write : public notojs::Task
{
public:
    Write(std::ofstream &&stream, Data &&data)
    : stream{std::move(stream)}, data{data}
    {
        out.push(boost::iostreams::gzip_compressor());
        out.push(this->stream);
    }

    Step step() override;

    Then then(JSContext *ctx, JSValue &v) override
    {
        v = bridge::Number{ctx, static_cast<std::int64_t>(pos)};
        return (out.flush(), Resolve);
    }

private:
    std::ofstream stream;
    std::size_t pos{0};
    Data const data;

private:
    boost::iostreams::filtering_ostream out;
};

template<>
notojs::Task::Step Write<std::string>::step()
{
    std::size_t const end = pos + sizeof(buffer);
    std::size_t const cnt = std::min(end - pos, data.size() - pos);
    out.write(data.c_str() + pos, cnt);
    return (pos += cnt) == data.size() ? Finish : Again;
}

template<>
notojs::Task::Step Write<std::shared_ptr<std::vector<std::uint8_t>>>::step()
{
    std::size_t const end = pos + sizeof(buffer);
    std::size_t const cnt = std::min(end - pos, data->size() - pos);
    out.write(reinterpret_cast<char*>(&(*data)[pos]), cnt);
    return (pos += cnt) == data->size() ? Finish : Again;
}

struct GZip : bridge::Interface<GZip, std::pair<std::filesystem::path, bool>>
{
    using Base::Base;

    JSValue blob(JSContext *ctx)
    {
        std::error_code ec;
        if(std::filesystem::is_regular_file(ref().first, ec))
            return std::make_shared<Read<std::vector<std::uint8_t>>>(std::ifstream(ref().first, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return GZipException::throw_(ctx, std::move(ec));
        else
            return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    JSValue json(JSContext *ctx)
    {
        std::error_code ec;
        if(std::filesystem::is_regular_file(ref().first, ec))
            return std::make_shared<Read<bridge::Object>>(std::ifstream(ref().first, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return GZipException::throw_(ctx, std::move(ec));
        else
            return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    JSValue text(JSContext *ctx)
    {
        std::error_code ec;
        if(std::filesystem::is_regular_file(ref().first, ec))
            return std::make_shared<Read<std::string>>(std::ifstream(ref().first, std::ios_base::in | std::ios_base::binary))->run(ctx);
        else if(ec)
            return GZipException::throw_(ctx, std::move(ec));
        else
            return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    template<typename T>
    JSValue writer(JSContext *ctx, std::ofstream &&stream, T &&data);

    template<typename T>
    JSValue write_0(JSContext *ctx, T data)
    {
        if(!ref().second) return GZipException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));

        std::error_code ec;
        if(!std::filesystem::exists(ref().first) || std::filesystem::is_regular_file(ref().first, ec))
            return writer(ctx, std::ofstream(ref().first, std::ios_base::out | std::ios_base::binary), std::move(data));
        else if(ec)
            return GZipException::throw_(ctx, std::move(ec));
        else
            return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    }

    using write = bridge::Function
    <
        &GZip::write_0<bridge::String>,
        &GZip::write_0<notojs::IBlob::Impl>
    >;

    using ctor = bridge::Unconstructable<GZip>;
    static JSCFunctionListEntry const funcs[];
};

template<>
JSValue GZip::writer<bridge::String>(JSContext *ctx, std::ofstream &&stream, bridge::String &&data)
{
    if(!stream.is_open()) return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    return std::make_shared<Write<std::string>>(std::move(stream), std::move(data))->run(ctx);
}

template<>
JSValue GZip::writer<notojs::IBlob::Impl>(JSContext *ctx, std::ofstream &&stream, notojs::IBlob::Impl &&data)
{
    if(!stream.is_open()) return GZipException::throw_(ctx, std::make_error_code(std::errc::no_such_file_or_directory));
    return std::make_shared<Write<std::shared_ptr<std::vector<uint8_t>>>>(std::move(stream), data->copy())->run(ctx);
}

JSCFunctionListEntry const GZip::funcs[] = {
    JS_CFUNC_DEF("blob", 0, &bridge::Function<&GZip::blob>::invoke),
    JS_CFUNC_DEF("json", 0, &bridge::Function<&GZip::json>::invoke),
    JS_CFUNC_DEF("text", 0, &bridge::Function<&GZip::text>::invoke),
    JS_CFUNC_DEF("write", 1, &GZip::write::invoke)
};

JSValue gzip(JSContext *ctx, fs::facade::Path path)
{
    return GZip::from(ctx, path->native());
}

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("gzip", 1, bridge::Function<gzip>::invoke),
};

int init(JSContext *ctx, JSModuleDef *m)
{
    GZip::init(ctx);
    JS_SetModuleExport(ctx, m, "default", JS_NewCFunction(ctx, bridge::Function<gzip>::invoke, "gzip", 1));
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_gzip()
{
    GZip::init();
}

void notojs_init_gzip(JSRuntime *rt)
{
    GZip::init(rt);
}

void notojs_init_gzip(boost::property_tree::ptree const &) {}

JSModuleDef *notojs_init_gzip(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    JS_AddModuleExport(ctx, mod, "default");
    return mod;
}

} // namespace notojs
