#include <notojs/detail/notodb.hpp>
#include <notojs/global.hpp>
#include <notojs/folder.hpp>
#include <lmdbxx/lmdb++.h>

#include <bridge.hpp>
#include <notodb.hpp>

namespace notojs {
namespace {

BOOST_FORCEINLINE std::uint64_t bswap64(std::uint64_t v)
{
#if defined(_MSC_VER)
    return _byteswap_uint64(v);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(v);
#else
    // Portable fallback
    return  ((v & 0x00000000000000FFULL) << 56) |
            ((v & 0x000000000000FF00ULL) << 40) |
            ((v & 0x0000000000FF0000ULL) << 24) |
            ((v & 0x00000000FF000000ULL) <<  8) |
            ((v & 0x000000FF00000000ULL) >>  8) |
            ((v & 0x0000FF0000000000ULL) >> 24) |
            ((v & 0x00FF000000000000ULL) >> 40) |
            ((v & 0xFF00000000000000ULL) >> 56);
#endif
}

BOOST_FORCEINLINE bool host_is_little_endian()
{
    std::uint16_t const x = 1;
    return *reinterpret_cast<uint8_t const *>(&x) == 1;
}

BOOST_FORCEINLINE std::uint64_t to_be_u64(std::uint64_t v)
{
    return host_is_little_endian() ? bswap64(v) : v;
}

BOOST_FORCEINLINE std::uint64_t from_be_u64(std::uint64_t v)
{
    return host_is_little_endian() ? bswap64(v) : v;
}

BOOST_FORCEINLINE void encode_i64_key(std::int64_t x, std::array<std::uint8_t, 9> &out)
{
    out[0] = 0;
    std::uint64_t be = to_be_u64(static_cast<std::uint64_t>(x) ^ 0x8000000000000000ULL);
    std::memcpy(&out[1], &be, 8);
}

BOOST_FORCEINLINE int64_t decode_i64_key(lmdb::val const &in)
{
    std::uint64_t be;
    std::memcpy(&be, in.data() + 1, 8);
    return static_cast<std::int64_t>(from_be_u64(be) ^ 0x8000000000000000ULL);
}

} // namesoace

DB::DB(JSContext *ctx)
: env{Global::ptr(ctx)->get<Folder>().env()}
{}

std::uint64_t DB::db_to_host(std::uint64_t v)
{
    return from_be_u64(v);
}

std::uint64_t DB::host_to_db(std::uint64_t v)
{
    return to_be_u64(v);
}

std::pair<lmdb::txn, lmdb::dbi> DB::open(Access a, DB::Namespace ns, std::string_view const &n) const
{
    std::string name;
    switch(ns)
    {
        case SYS: name.append("sys:"); break;
        case USR: name.append("usr:"); break;
        case VAR: name.append("var:"); break;
    }
    name.append(n);
    switch(a)
    {
    case RO:
        {
            auto tx = lmdb::txn::begin(env);
            auto db = lmdb::dbi::open(tx, nullptr);

            lmdb::val k{name}, v;
            if(!db.get(tx, k, v))
            {
                (void)lmdb::dbi::open(tx, name.c_str(), MDB_CREATE);
                tx.commit();
            }
            else
            {
                tx.abort();
            }

            tx = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
            db = lmdb::dbi::open(tx, name.c_str());
            return {std::move(tx), std::move(db)};
        }
        break;
    case RT:
        {
            auto tx = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
            auto db = lmdb::dbi::open(tx, name.c_str());
            return {std::move(tx), std::move(db)};
        }
        break;
    case RW:
        {
            auto tx = lmdb::txn::begin(env);
            auto db = lmdb::dbi::open(tx, name.c_str(), MDB_CREATE);
            return {std::move(tx), std::move(db)};
        }
        break;
    case WT:
        {
            auto tx = lmdb::txn::begin(env);
            auto db = lmdb::dbi::open(tx, name.c_str());
            return {std::move(tx), std::move(db)};
        }
        break;
    }
    throw;
}

std::pair<lmdb::txn, std::vector<lmdb::dbi>> DB::open(Access a, DB::Namespace ns, std::vector<std::string> const &names) const
{
    std::string name;
    switch(ns)
    {
        case SYS: name.append("sys:"); break;
        case USR: name.append("usr:"); break;
        case VAR: name.append("var:"); break;
    }
    switch(a)
    {
    case RO:
        {
            auto tx = lmdb::txn::begin(env);
            auto db = lmdb::dbi::open(tx, nullptr);

            bool commit{false};
            for(auto const &n : names)
            {
                name.replace(4, name.size() - 4, n);
                lmdb::val k{name}, v;
                if(!db.get(tx, k, v))
                {
                    (void)lmdb::dbi::open(tx, name.c_str(), MDB_CREATE);
                    commit = true;
                }
            }
            if(commit) tx.commit();
            else tx.abort();

            tx = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

            std::vector<lmdb::dbi> dbs;
            std::transform(std::begin(names), std::end(names), std::back_inserter(dbs), [&tx, &name](auto const &n) {
                name.replace(4, name.size() - 4, n);
                return lmdb::dbi::open(tx, name.c_str());
            });
            return {std::move(tx), std::move(dbs)};
        }
        break;
    case RT:
        {
            auto tx = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

            std::vector<lmdb::dbi> dbs;
            std::transform(std::begin(names), std::end(names), std::back_inserter(dbs), [&tx, &name](auto const &n) {
                name.replace(4, name.size() - 4, n);
                return lmdb::dbi::open(tx, name.c_str());
            });
            return {std::move(tx), std::move(dbs)};
        }
        break;
    case RW:
        {
            auto tx = lmdb::txn::begin(env);

            std::vector<lmdb::dbi> dbs;
            std::transform(std::begin(names), std::end(names), std::back_inserter(dbs), [&tx, &name](auto const &n) {
                name.replace(4, name.size() - 4, n);
                return lmdb::dbi::open(tx, name.c_str(), MDB_CREATE);
            });
            return {std::move(tx), std::move(dbs)};
        }
        break;
    case WT:
        {
            auto tx = lmdb::txn::begin(env);

            std::vector<lmdb::dbi> dbs;
            std::transform(std::begin(names), std::end(names), std::back_inserter(dbs), [&tx, &name](auto const &n) {
                name.replace(4, name.size() - 4, n);
                return lmdb::dbi::open(tx, name.c_str());
            });
            return {std::move(tx), std::move(dbs)};
        }
        break;
    }
    throw;
}

std::pair<lmdb::txn, lmdb::dbi> DB::http(Access a) const
{
    return open(a, SYS, Folder::http.substr(4));
}

std::pair<lmdb::txn, lmdb::dbi> DB::pkgs(Access a) const
{
    return open(a, SYS, Folder::pkgs.substr(4));
}

MDB_envinfo DB::info() const
{
    MDB_envinfo info;
    mdb_env_info(env, &info);
    return info;
}

MDB_stat DB::stat() const
{
    MDB_stat stat;
    mdb_env_stat(env, &stat);
    return stat;
}

void DB::backup(std::filesystem::path const &p) const
{
    lmdb::env_copy(env, p.u8string().c_str());
}

DB::Type DB::type(lmdb::val const &f)
{
    if(!f.size()) return Type::String;
    if(!f.data()[0]) return Type::Number;
    if(detail::is_object<false>(f)) return Type::Object;
    return Type::String;
}

void DB::set(lmdb::val &t, bridge::String &f)
{
    auto const &fv = static_cast<std::string_view const &>(f);
    t.assign(fv.data(), fv.size());
}

template<>
void DB::set<bridge::Number>(lmdb::val &t, bridge::Number &f, DB::Allocator &a)
{
    auto &b = a.emplace<1>();
    encode_i64_key(static_cast<std::int64_t>(f), b);
    t.assign(b.data(), b.size());
}

template<>
void DB::set<bridge::String>(lmdb::val &t, bridge::String &f, DB::Allocator &)
{
    auto const &fv = static_cast<std::string_view const &>(f);
    t.assign(fv.data(), fv.size());
}

template<>
void DB::set<bridge::Value>(lmdb::val &t, bridge::Value &f, DB::Allocator &a)
{
    bridge::String j{f, JS_JSONStringify(f, f, JS_UNDEFINED, JS_UNDEFINED)};
    auto const &jv = static_cast<std::string_view const &>(j);
    t.assign(jv.data(), jv.size() + 1);
    a.emplace<2>(f, std::move(j));
}

JSValue DB::get(JSContext *ctx, lmdb::val const &f)
{
    switch(type(f))
    {
    case Type::Number:
        return bridge::Number(ctx, decode_i64_key(f));
    case Type::Object:
        return JS_ParseJSON(ctx, f.data(), f.size() - 1, "<json>");
    case Type::String:
        return bridge::String(ctx, std::string_view(f.data(), f.size()));
    }
    return JS_UNDEFINED;
}

DB::Storage::Storage(JSContext *ctx, std::string_view const &name)
: DB{ctx}, ctx{ctx}, name{name.data(), name.size()}
{}

JSValue DB::Storage::key(bridge::Number const &n) const
{
    JSValue rs = JS_NULL;
    lmdb::val k, v;
    std::int64_t m = static_cast<std::int64_t>(n);

    auto [tx, db] = open(Access::RO, SYS, name);
    auto cr = lmdb::cursor::open(tx, db);
    if(cr.get(k, v, MDB_FIRST)) do {
        if(!m--) {
            rs = bridge::String(ctx, std::string_view(k.data(), k.size()));
            break;
        }
    } while(cr.get(k, v, MDB_NEXT));
    cr.close();
    tx.abort();

    return rs;
}

JSValue DB::Storage::get(bridge::String &k)
{
    auto const &kv = static_cast<std::string_view const &>(k);
    lmdb::val lk{kv.data(), kv.size()};

    JSValue rs = JS_NULL;

    auto [tx, db] = open(Access::RO, SYS, name);
    if(lmdb::val v; db.get(tx, lk, v))
        rs = DB::get(ctx, v);

    tx.abort();
    return rs;
}

void DB::Storage::set(bridge::String &k, bridge::String &v)
{
    lmdb::val lk;
    lmdb::val lv;

    DB::set(lk, k);
    DB::set(lv, v);

    auto [tx, db] = open(Access::RW, SYS, name);
    db.put(tx, lk, lv);
    tx.commit();
}

void DB::Storage::set(bridge::String &k, bridge::Value &v)
{
    Allocator av;
    lmdb::val lk;
    lmdb::val lv;

    DB::set(lk, k);
    DB::set(lv, v, av);

    auto [tx, db] = open(Access::RW, SYS, name);
    db.put(tx, lk, lv);
    tx.commit();
}

void DB::Storage::remove(bridge::String &k)
{
    auto const &kv = static_cast<std::string_view const &>(k);
    lmdb::val lk{kv.data(), kv.size()};

    auto [tx, db] = open(Access::RW, SYS, name);
    db.del(tx, lk);
    tx.commit();
}

void DB::Storage::remove() const
{
    auto [tx, db] = open(Access::RW, SYS, name);
    db.drop(tx, true);
    tx.commit();
}

std::uint64_t DB::Storage::count() const
{
    auto [tx, db] = open(Access::RO, SYS, name);
    auto rs = static_cast<std::uint64_t>(db.stat(tx).ms_entries);
    tx.abort();
    return rs;
}

} // namespace notojs
