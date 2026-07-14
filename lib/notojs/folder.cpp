#include <notojs/config.hpp>
#include <notojs/folder.hpp>
#include <notojs/engine.hpp>
#include <notojs/logger.hpp>
#include <notojs/plugin.hpp>
#include <notojs/timers.hpp>
#include <notojs/writer.hpp>

#include <notojs/detail/config.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/url.hpp>

#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/memorystream.h>

#include <lmdbxx/lmdb++.h>
#include <atomic>
#include <regex>
#include <map>

#include <pwd.h>
#include <stdexcept>
#include <unistd.h>

#define BOUNDARY "--notojs-313960ee-e195-4a1e-a796-e881cd5553c2"

namespace notojs {
namespace {

std::atomic<std::uint16_t> tmpid{0};

namespace x3 = boost::spirit::x3;
using Iterator = boost::spirit::istream_iterator;

enum class Mode
{
    Full,
    Code,
    Data,
    Json
};

template<Mode M>
struct Base
{
    BOOST_FORCEINLINE void body(std::string const &) {}
};

template<>
struct Base<Mode::Json>
{
    Base(Base &&) = delete;
    Base(Base const &) = delete;

    BOOST_FORCEINLINE Base(std::unordered_set<std::string> &rs): rs{rs} {}
    BOOST_FORCEINLINE void body(std::string const &body)
    {
        alevel = 0;
        olevel = 0;
        render = false;
        rarray.clear();
        rapidjson::Reader reader;
        rapidjson::MemoryStream ms(body.c_str(), body.size());
        reader.Parse(ms, *this);
    }

    BOOST_FORCEINLINE bool Null()           { return true; }
    BOOST_FORCEINLINE bool Bool(bool)       { return true; }
    BOOST_FORCEINLINE bool Int(int)         { return true; }
    BOOST_FORCEINLINE bool Uint(unsigned)   { return true; }
    BOOST_FORCEINLINE bool Int64(int64_t)   { return true; }
    BOOST_FORCEINLINE bool Uint64(uint64_t) { return true; }
    BOOST_FORCEINLINE bool Double(double)   { return true; }
    BOOST_FORCEINLINE bool RawNumber(const char* str, rapidjson::SizeType len, bool) { return true; }

    BOOST_FORCEINLINE bool Key(const char* str, rapidjson::SizeType len, bool)
    {
        if(alevel == 1 && olevel == 1) key.assign(str, len);
        else key.clear();
        return true;
    }

    BOOST_FORCEINLINE bool String(const char* str, rapidjson::SizeType len, bool)
    {
        if(alevel == 2 && olevel == 1 && "data" == key)
        {
            if(render) rs.insert(std::string{str, len});
            else rarray.push_back(std::string{str, len});
        }
        else if(alevel == 1 && olevel == 1 && "type" == key && "notojs.Render" == std::string_view{str, len})
        {
            if(!rarray.empty())
            {
                for(auto it = std::begin(rarray); it != std::end(rarray);)
                {
                    rs.insert(*it); it = rarray.erase(it);
                }
            }
            render = true;
        }
        return true;
    }

    BOOST_FORCEINLINE bool StartArray()
    {
        ++alevel;
        return true;
    }

    BOOST_FORCEINLINE bool EndArray(rapidjson::SizeType)
    {
        render = false;
        --alevel;
        return true;
    }

    BOOST_FORCEINLINE bool StartObject()
    {
        ++olevel;
        return true;
    }

    BOOST_FORCEINLINE bool EndObject(rapidjson::SizeType)
    {
        --olevel;
        rarray.clear();
        return true;
    }

private:
    std::string key;
    bool render{false};
    std::size_t alevel{0};
    std::size_t olevel{0};
    std::vector<std::string> rarray;
    std::unordered_set<std::string> &rs;
};

template<Mode M>
struct Parser : Writer, Base<M>
{
    template<typename ...Args>
    BOOST_FORCEINLINE Parser(std::string &output, Args&& ...args)
    : Writer{output}
    , Base<M>{std::forward<Args>(args)...}
    {
        Writer::start();
    }

    BOOST_FORCEINLINE ~Parser()
    {
        if constexpr (M == Mode::Full)
        {
            if(std::exchange(object, false)) Writer::endObject();
        }
        Writer::end();
    }

    struct Line
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            parser.content.append(x3::_attr(ctx));
            parser.content.append("\n");
        }
    };

    struct Skip
    {
        template<typename C>
        void operator () (C& ctx) {}
    };

    struct Part
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            if("application/javascript" == parser.type)
            {
                if constexpr (M == Mode::Full)
                {
                    if(std::exchange(parser.object, true)) parser.Writer::endObject();
                    parser.Writer::startObject();
                    if(parser.content.length()) parser.content.pop_back();
                    parser.Writer::string("code", parser.content.c_str(), parser.content.length());
                    if(parser.role)
                    {
                        parser.Writer::string("role", parser.role->c_str(), parser.role->length());
                        parser.role.reset();
                    }
                }
            }
            else if("application/json" == parser.type)
            {
                parser.Base<M>::body(parser.content);
                if constexpr (M == Mode::Full)
                {
                    parser.Writer::string("body", parser.content.c_str(), parser.content.length());
                }
                else
                {
                    parser.Writer::string(std::move(parser.content));
                }
            }
            parser.type.reset();
        }
    };

    struct Role
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            parser.role = x3::_attr(ctx);
        }
    };

    struct Type
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            parser.type = x3::_attr(ctx);
            parser.content.clear();
        }
    };

    void lib(std::ifstream &&stream)
    {
        auto const str = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        Writer::startObject();
        Writer::string("code", str.c_str(), str.size());
        Writer::endObject();
    }

    bool object{false};
    std::string content;
    std::optional<std::string> role;
    std::optional<std::string> type;
};

template<>
struct Parser<Mode::Code> : detail::Textcode
{
    struct Line
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            parser.content.append(x3::_attr(ctx));
            parser.content.append("\n");
        }
    };

    struct Skip
    {
        template<typename C>
        void operator () (C& ctx) {}
    };

    struct Part
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            if(parser.consume && "application/javascript" == parser.type)
            {
                if(parser.content.length()) parser.content.pop_back();
                if(auto &code = parser.emplace_back(std::move(parser.content)); parser.disabled)
                    ++code.data()[code.size()];
                if(1 == parser.consume) parser.consume = 0;
            }
            parser.type.reset();
        }
    };

    struct Role
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            if("handler" == x3::_attr(ctx))
                --parser.consume;
            else if("disabled" == x3::_attr(ctx))
                parser.disabled = true;
        }
    };

    struct Type
    {
        template<typename C>
        void operator () (C& ctx)
        {
            Parser &parser = x3::_val(ctx);
            parser.type = x3::_attr(ctx);
            parser.content.clear();
            parser.disabled = false;
        }
    };

    bool disabled{false};
    unsigned consume{2};
    std::string content;
    std::optional<std::string> type;
};

template<Mode M>
auto make_multipart()
{
    using Parser_ = Parser<M>;

    auto bs = x3::char_('\n') >> x3::lit(BOUNDARY)[typename Parser_::Part()] >> -x3::char_('\r');
    auto be = x3::char_('\n') >> x3::lit(BOUNDARY "--")[typename Parser_::Part()] >> -x3::char_('\r');
    auto ct = x3::char_('\n') >> x3::lit("Content-type: ") >> (*(x3::char_ - x3::char_("\n\r:")))[typename Parser_::Type()] >> -x3::char_('\r');
    auto cr = x3::char_('\n') >> x3::lit("Content-role: ") >> (*(x3::char_ - x3::char_("\n\r:")))[typename Parser_::Role()] >> -x3::char_('\r');
    auto line = x3::char_('\n') >> (*(x3::char_ - x3::char_("\r\n")))[typename Parser_::Line()] >> -x3::char_('\r');
    auto header = x3::lit("Content-type: multipart/mixed; boundary=\"" BOUNDARY "\"") >> -x3::char_('\r') >> x3::char_('\n') >> -x3::char_('\r');
    auto multipart_def = header[typename Parser_::Skip()] >> *(be | (bs >> ct >> -cr >> x3::char_('\n') >> -x3::char_('\r')) | line);

    struct multipart;
    x3::rule<struct multipart, Parser_> const multipart = "multipart";
    return multipart = multipart_def;
}

std::regex const ConfigRegex{"^([A-Za-z0-9_.@-]+) = (.+)$"};

} // namespace

std::string const Folder::http{"sys:httpdata"};
std::string const Folder::pkgs{"sys:packages"};

Folder::~Folder()
{
    try {
        lmdb::env_close(env_);
    } catch(...) {}
}

void Folder::init_db()
{
    auto const db = (path / ".db");
    if(!std::filesystem::exists(db))
        std::filesystem::create_directories(db);

    auto const dp = db.u8string();
    lmdb::env_create(&env_);
    lmdb::env_set_max_dbs(env_, 128);
    lmdb::env_set_mapsize(env_, size);
    lmdb::env_open(env_, dp.c_str(), 0, 0666);
    mdb_reader_check(env_, NULL);

    try {
        auto tx = lmdb::txn::begin(env_);
        lmdb::dbi::open(tx, pkgs.c_str(), MDB_CREATE);
        lmdb::dbi::open(tx, http.c_str(), MDB_CREATE);
        tx.commit();
    } catch(...) {}
}

void Folder::configure(boost::property_tree::ptree const &pt)
{
    if(auto path = pt.get_optional<std::string>("folder.path"))
        this->path = std::move(*path);
    else
    {
        if (auto* pw = getpwuid(getuid()))
            this->path = std::filesystem::path(pw->pw_dir) / "NotoJS";
        else
            throw std::runtime_error("Cannot find user home directory");
        if(!std::filesystem::exists(this->path))
            std::filesystem::create_directories(this->path);
    }
    if(!std::filesystem::is_directory(path))
        throw std::invalid_argument("folder.path should be a directory");

    if(auto const app = (path / "app"); !std::filesystem::exists(app))
        std::filesystem::create_directory(app);

    if(auto const lib = (path / "lib"); !std::filesystem::exists(lib))
        std::filesystem::create_directory(lib);

    if(auto size = pt.get_optional<std::string>("folder.lmdb"); size)
    {
        std::string mult;
        std::istringstream(*size) >> this->size >> mult;
        if("mb" == mult) this->size <<= 20;
        else if("gb" == mult) this->size <<= 30;
        else throw std::runtime_error("Unknown database size multiplier " + mult);
    }

    init_db();
}

void Folder::set_path(std::filesystem::path &&path)
{
    this->path = std::move(path);
    if(!env_) init_db();
}

std::optional<std::filesystem::path> Folder::lib(std::string const &name) const
{
    if(auto const p = this->path / "lib" / name; std::filesystem::exists(p))
        return p;
    return std::nullopt;
}

boost::beast::http::status Folder::list(std::string &json) const
{
    NOTOJS_LOG_THIS_THREAD
    Writer writer(json);
    writer.startObject();
    writer.startArray("notebooks");

    for(auto const &entry : std::filesystem::directory_iterator(this->path))
    {
        auto const path = entry.path();
        if(auto ext = path.extension(); ext == ".notojs")
            writer.string(path.stem().u8string());
    }

    writer.endArray();
    writer.startArray("applications");

    for(auto const &entry : std::filesystem::directory_iterator(this->path / "app"))
    {
        auto const path = entry.path();
        if(auto ext = path.extension(); ext == ".notojs")
            writer.string(path.stem().u8string());
    }

    writer.endArray();
    writer.startArray("libraries");

    for(auto const &entry : std::filesystem::directory_iterator(this->path / "lib"))
    {
        auto const path = entry.path();
        if(auto ext = path.extension(); ext == ".js")
            writer.string(path.stem().u8string());
    }

    writer.endArray();
    writer.startArray("packages");

    try {
        lmdb::val k, v, c;
        auto tx = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto db = lmdb::dbi::open(tx, pkgs.c_str());
        std::optional<lmdb::dbi> ht;
        try {
            ht = lmdb::dbi::open(tx, http.c_str());
        } catch(...) {}
        auto cr = lmdb::cursor::open(tx, db);
        if(cr.get(k, v, MDB_FIRST)) do {
            if(k.data()[0])
            {
                writer.startObject();
                writer.string("name", k.data(), k.size());
                writer.string("url", v.data(), v.size() - detail::INI::tail_size);
                if(ht && ht->get(tx, lmdb::val(v.data(), v.size() - detail::INI::tail_null), c))
                {
                    writer.integer("size", c.size());
                }
                writer.integer("type", static_cast<int>(detail::INI::line_type(v)) - 1);
                writer.endObject();
            }
        } while(cr.get(k, v, MDB_NEXT));

        cr.close();
        tx.abort();
    } catch(std::runtime_error const &e) {}
    writer.endArray();

    writer.startArray("databases");
    if(env_) try {
        lmdb::val k{"sys:storage:"}, v;
        auto tx = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto db = lmdb::dbi::open(tx, nullptr);
        auto cr = lmdb::cursor::open(tx, db);

        if(cr.get(k, v, MDB_SET_RANGE)) do {
            writer.startObject();
            writer.string("name", k.data(), k.size());
            auto dbx = lmdb::dbi::open(tx, k.data());
            writer.integer("size", dbx.stat(tx).ms_entries);
            writer.endObject();
        } while(cr.get(k, v, MDB_NEXT));

        cr.close();
        tx.abort();
    } catch(std::runtime_error const &e) {}
    writer.endArray();

    writer.endObject();
    return boost::beast::http::status::ok;
}

boost::beast::http::status Folder::load(std::string &json, std::string const &path) const
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        auto const f = this->path / p;
        auto const library = ("lib" == f.parent_path().filename().u8string());
        if(library)
        {
            std::ifstream lfs(std::filesystem::path(f).replace_extension(".js"));
            if(!lfs.is_open())
            {
                json = "[]";
                return boost::beast::http::status::not_found;
            }

            Parser<Mode::Full> parser(json);
            parser.lib(std::move(lfs));

            if(std::ifstream ifs(this->path / p); ifs.is_open())
            {
                ifs.unsetf(std::ios::skipws);
                x3::parse(Iterator(ifs), Iterator{}, make_multipart<Mode::Full>(), parser);
            }
            return boost::beast::http::status::ok;
        }
        else if(std::ifstream ifs(this->path / p); !ifs.is_open())
        {
            json = "[]";
            return boost::beast::http::status::not_found;
        }
        else
        {
            Parser<Mode::Full> parser(json);

            ifs.unsetf(std::ios::skipws);
            x3::parse(Iterator(ifs), Iterator{}, make_multipart<Mode::Full>(), parser);
            return boost::beast::http::status::ok;
        }
    }
    return boost::beast::http::status::not_found;
}

template<>
std::optional<detail::Textcode> Folder::load<detail::Textcode>(std::string const &path) const
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        if(std::ifstream ifs(this->path / p); ifs.is_open())
        {
            ifs.unsetf(std::ios::skipws);

            Parser<Mode::Code> parser;
            x3::parse(Iterator(ifs), Iterator{}, make_multipart<Mode::Code>(), parser);
            return std::move(static_cast<detail::Textcode &>(parser));
        }
    }
    return std::nullopt;
}

template<>
std::optional<detail::Version> Folder::load<detail::Version>(std::string const &path) const
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        if(auto const q = this->path / p; std::filesystem::is_regular_file(q))
            return std::filesystem::last_write_time(q);
    }
    return std::nullopt;
}

boost::beast::http::status Folder::eval(std::string &json, std::string const &path) const
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        if(std::ifstream ifs(this->path / p); ifs.is_open())
        {
            Parser<Mode::Data> parser(json);

            ifs.unsetf(std::ios::skipws);
            x3::parse(Iterator(ifs), Iterator{}, make_multipart<Mode::Data>(), parser);
            return boost::beast::http::status::ok;
        }
    }
    json = "[]";
    return boost::beast::http::status::not_found;
}

boost::beast::http::status Folder::eval(std::string &json, std::string const &path, std::unordered_set<std::string> &rs) const
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        if(std::ifstream ifs(this->path / p); ifs.is_open())
        {
            Parser<Mode::Json> parser(json, rs);

            ifs.unsetf(std::ios::skipws);
            x3::parse(Iterator(ifs), Iterator{}, make_multipart<Mode::Json>(), parser);
            return boost::beast::http::status::ok;
        }
    }
    json = "[]";
    return boost::beast::http::status::not_found;
}

Folder::File::File(std::string const &name, Folder const &fold)
: path{fold.path / (name + "." + std::to_string(++tmpid))}
, stream{path}
{
    stream << "Content-type: multipart/mixed; boundary=\"" << BOUNDARY << "\"\n\n";
}

void Folder::File::code(std::string const &code, bool enabled)
{
    stream << BOUNDARY << '\n';
    stream << "Content-type: application/javascript\n";
    if(!enabled) stream << "Content-role: disabled\n";
    stream << '\n' << code << "\n";
}

void Folder::File::data(std::string const &data)
{
    stream << BOUNDARY << '\n';
    stream << "Content-type: application/json\n\n";
    stream << data << "\n";
}

Folder::File::~File()
{
    if(stream.is_open())
    {
        NOTOJS_LOG_THIS_THREAD
        stream << BOUNDARY << "--";
        stream.close();

        if(discard)
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
            if(ec) std::clog << ec << '\n';
        }
        else
        {
            std::error_code ec;
            auto const target = path.parent_path() / path.stem();
            auto const backup = path.parent_path() / (path.stem().u8string() + ".bak");

            std::filesystem::rename(target, backup, ec);
            if(ec) { std::clog << ec << '\n'; return; }

            std::filesystem::rename(path, target, ec);
            if(ec) { std::clog << ec << '\n'; return; }

            std::filesystem::remove(backup, ec);
            if(ec) { std::clog << ec << '\n'; return; }
        }
    }
}

boost::beast::http::status Folder::save(std::string const &json, std::string const &path)
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        auto const f = this->path / p;
        rapidjson::Document document;
        try {
            document.Parse(json.c_str(), json.size());

            auto const &cells = document.GetArray();
            auto const library = ("lib" == f.parent_path().filename().u8string());
            if(library)
            {
                if(cells.Size() && cells[0].HasMember("code") && cells[0]["code"].IsString())
                {
                    std::ofstream(std::filesystem::path(f).replace_extension(".js")) << cells[0]["code"].GetString();
                    cells.Erase(cells.begin());
                    if(cells.Empty())
                    {
                        if(std::filesystem::exists(f)) std::filesystem::remove(f);
                        return boost::beast::http::status::ok;
                    }
                }
            }

            std::ofstream fs(f);
            fs << "Content-type: multipart/mixed; boundary=\"" << BOUNDARY << "\"\n\n";
            for(auto const &cell : cells)
            {
                bool code{false};
                if(code = (cell.HasMember("code") && cell["code"].IsString()); code)
                {
                    fs << BOUNDARY << '\n';
                    fs << "Content-type: application/javascript\n";
                    if(cell.HasMember("role") && cell["role"].IsString())
                    {
                        fs << "Content-role: " << cell["role"].GetString() << '\n';
                    }
                    fs << '\n' << cell["code"].GetString() << '\n';
                }
                if(code && cell.HasMember("body") && cell["body"].IsString())
                {
                    fs << BOUNDARY << '\n';
                    fs << "Content-type: application/json\n\n";
                    fs << cell["body"].GetString() << "\n";
                }
            }
            fs << BOUNDARY << "--";
            fs.close();

            if(!library)
            {
                get<Plugin>().updated(path);
                get<Timers>().updated(path);
            }
            return boost::beast::http::status::ok;
        } catch(...) {
            std::filesystem::remove(f);
            std::clog << "Exception" << notojs::values(path);
            return boost::beast::http::status::internal_server_error;
        }
    }
    return boost::beast::http::status::not_found;
}

boost::beast::http::status Folder::move(std::string const &path, std::string name)
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        auto f = this->path / p;
        auto const library = ("lib" == f.parent_path().filename().u8string());
        try {
            if(library)
            {
                if(std::filesystem::exists(f)) std::filesystem::rename(f, this->path / name);
                f = f.replace_extension(".js");
                name = std::filesystem::path(name).replace_extension(".js");
            }

            std::filesystem::rename(f, this->path / name);
            if(!library)
            {
                get<Plugin>().removed(path);
                get<Timers>().removed(path);
            }
            return boost::beast::http::status::ok;
        } catch(...) {
            std::clog << "Exception" << notojs::values(path);
            return boost::beast::http::status::internal_server_error;
        }
    }
    return boost::beast::http::status::not_found;
}

boost::beast::http::status Folder::remove(std::string const &path)
{
    NOTOJS_LOG_THIS_THREAD
    if(auto const p = std::filesystem::path(path); p.extension() == ".notojs")
    {
        auto f = this->path / p;
        auto const library = ("lib" == f.parent_path().filename().u8string());
        try {
            if(library)
            {
                if(std::filesystem::exists(f))
                    if(!std::filesystem::remove(f)) return boost::beast::http::status::internal_server_error;
                f = f.replace_extension(".js");
            }
            bool const removed = std::filesystem::remove(f);
            if(removed && !library)
            {
                get<Plugin>().removed(path);
                get<Timers>().removed(path);
            }
            return removed
                ? boost::beast::http::status::ok
                : boost::beast::http::status::internal_server_error
            ;
        } catch(...) {
            std::clog << "Exception" << notojs::values(path);
            return boost::beast::http::status::internal_server_error;
        }
    }
    return boost::beast::http::status::not_found;
}

boost::beast::http::status Folder::get_packages(std::string &target) const
{
    try {
        std::map<detail::INI::size_type, std::string> lines;
        auto tx = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto db = lmdb::dbi::open(tx, pkgs.c_str());
        auto cr = lmdb::cursor::open(tx, db);

        lmdb::val k, v;
        if(cr.get(k, v, MDB_FIRST)) do
        {
            if(!k.size()) continue;
            if(!k.data()[0])
            {
                lines.emplace(
                    ntohs(*reinterpret_cast<detail::INI::size_type*>(&k.data()[1])),
                    std::string(v.data(), v.size())
                );
            }
            else
            {
                std::string line(k.data(), k.size());
                line.append(" = ", 3);
                line.append(v.data(), v.size() - 2 - sizeof(detail::INI::size_type));
                lines.emplace(
                    htons(*reinterpret_cast<detail::INI::size_type*>(&v.data()[v.size() - sizeof(detail::INI::size_type)])),
                    std::move(line)
                );
            }
        } while(cr.get(k, v, MDB_NEXT));

        cr.close();
        tx.abort();
        for(auto const &[_, line]: lines)
        {
            if(!target.empty()) target.append("\n");
            target.append(line);
        }
    } catch(...) {
        return boost::beast::http::status::internal_server_error;
    }
    return boost::beast::http::status::ok;
}

boost::beast::http::status Folder::set_packages(std::string &source) const
{
    detail::INI::Line type = detail::INI::Line::SYNTAX;
    std::unordered_set<std::string> names;
    std::vector<detail::INI::Entry> lines;
    int seen = 0;

    std::istringstream iss(source);
    for(std::string line; std::getline(iss, line);)
    {
        if(line.empty() || '#' == line[0])
        {
            auto &entry = lines.emplace_back();
            entry[0].line(lines.size());
            entry[1].text(line);
        }
        else if('[' == line[0])
        {
            if(line == "[scripts]")
            {
                if(seen & static_cast<int>(detail::INI::Line::SCRIPT))
                {
                    source = "Duplicate section " + line + "@" + std::to_string(1 + lines.size());
                    return boost::beast::http::status::internal_server_error;
                }
                seen |= static_cast<int>(detail::INI::Line::SCRIPT);
                type = detail::INI::Line::SCRIPT;

                auto &entry = lines.emplace_back();
                entry[0].line(lines.size());
                entry[1].text(line);
            }
            else if(line == "[modules]")
            {
                if(seen & static_cast<int>(detail::INI::Line::MODULE))
                {
                    source = "Duplicate section " + line + "@" + std::to_string(1 + lines.size());
                    return boost::beast::http::status::internal_server_error;
                }
                seen |= static_cast<int>(detail::INI::Line::MODULE);
                type = detail::INI::Line::MODULE;

                auto &entry = lines.emplace_back();
                entry[0].line(lines.size());
                entry[1].text(line);
            }
            else
            {
                source = "Invalid section " + line + "@" + std::to_string(1 + lines.size());
                return boost::beast::http::status::internal_server_error;
            }
        }
        else if(std::smatch match; std::regex_match(line, match, ConfigRegex))
        {
            std::string u = match[2];
            if(auto url = boost::urls::parse_uri(u); !url)
            {
                source = "Invalid URL " + u + "@" + std::to_string(1 + lines.size());
                return boost::beast::http::status::internal_server_error;
            }
            else if(boost::urls::scheme::http != url->scheme_id() && boost::urls::scheme::https != url->scheme_id())
            {
                std::string scheme = url->scheme();
                source = "Unsupported scheme " + scheme + "@" + std::to_string(1 + lines.size());
                return boost::beast::http::status::internal_server_error;
            }
            if(names.count(match[1]))
            {
                source = "Duplicate package name@" + std::to_string(1 + lines.size());
                return boost::beast::http::status::internal_server_error;
            }
            names.insert(match[1]);

            auto &entry = lines.emplace_back();
            entry[0].text(match[1]);
            entry[1].path(type, u, lines.size());
        }
        else
        {
            source = "Invalid syntax@" + std::to_string(1 + lines.size());
            return boost::beast::http::status::internal_server_error;
        }
    }

    std::unordered_set<std::string_view> urls;
    for(auto &entry : lines)
    {
        entry[0].make();
        entry[1].make();

        if(detail::INI::Line::SYNTAX != detail::INI::line_type(entry[1].val))
            urls.insert(std::string_view{
                entry[1].val.data(),
                entry[1].val.size() - detail::INI::tail_null
            });
    }

    lmdb::val k, v;
    try {
        auto tx = lmdb::txn::begin(env_);
        auto db = lmdb::dbi::open(tx, pkgs.c_str(), MDB_CREATE);
        auto ht = lmdb::dbi::open(tx, http.c_str(), MDB_CREATE);

        auto cr = lmdb::cursor::open(tx, ht);
        if(cr.get(k, v, MDB_FIRST)) do {
            if(k.size() && !k.data()[k.size() - 1] && !urls.count(std::string_view{k.data(), k.size()}))
                ht.del(tx, k);
        } while(cr.get(k, v, MDB_NEXT));
        cr.close();

        db.drop(tx, false);
        for(auto &entry : lines)
            db.put(tx, entry[0].val, entry[1].val);

        tx.commit();
    } catch(std::runtime_error const &e) {
        source = std::string{"std::runtime_error ["} + e.what() + "]";
        return boost::beast::http::status::internal_server_error;
    }
    source.clear();
    return boost::beast::http::status::ok;
}

} // namespace notoj
