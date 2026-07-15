#include <notojs/detail/scoped.hpp>
#include <notojs/module/db.hpp>
#include <lmdbxx/lmdb++.h>
#include <variant>
#include <vector>

#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>
#include <notodb.hpp>

#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>

namespace notojs {
namespace {

class DBException : public bridge::Exception<DBException>
{
    std::variant<std::error_code, std::reference_wrapper<std::string_view const>, std::runtime_error> ec;
public:
    DBException(std::error_code &&ec): ec{ec} {}
    DBException(std::string_view const &ec): ec{ec} {}
    DBException(std::runtime_error const &ec): ec{ec} {}
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        std::visit(boost::hana::overload_linearly(
            [ctx, &obj](std::error_code const &ec) {
                obj.set("code", bridge::Number(ctx, ec.value()));
                obj.set("message", bridge::String(ctx, ec.message()));
            },
            [ctx, &obj](std::reference_wrapper<std::string_view const> const &st) {
                obj.set("message", bridge::String(ctx, st.get()));
            },
            [ctx, &obj](std::runtime_error const &ec) {
                obj.set("message", bridge::String(ctx, std::string_view{ec.what()}));
            }
        ), ec);
    }
};

struct Handle_
{
    lmdb::txn *tx{nullptr};
    lmdb::dbi *db{nullptr};
};

JSValue mkstat(JSContext *ctx, MDB_stat res)
{
    bridge::Object obj{ctx};
    obj.set("psize", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_psize)));
    obj.set("depth", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_depth)));
    obj.set("branch_pages", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_branch_pages)));
    obj.set("leaf_pages", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_leaf_pages)));
    obj.set("overflow_pages", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_overflow_pages)));
    obj.set("entries", bridge::Number(ctx, static_cast<std::uint64_t>(res.ms_entries)));
    return obj;
}

struct Handle : bridge::Interface<Handle, Handle_>
{
    BOOST_FORCEINLINE static Handle_ &wrapped(JSValue value)
    {
        return Base::get(value);
    }

    static constexpr std::string_view ERROR{"Transaction is finished"};

    template<typename K>
    JSValue del_(JSContext *ctx, K k)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        lmdb::val key;
        DB::Allocator ka;
        DB::set(key, k, ka);
        try {
            return ref().db->del(*ref().tx, key) ? JS_TRUE : JS_FALSE;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using del = bridge::Function
    <
        &Handle::del_<bridge::Number>,
        &Handle::del_<bridge::String>
    >;

    template<typename K>
    JSValue get_(JSContext *ctx, K k)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        lmdb::val key;
        DB::Allocator ka;
        DB::set(key, k, ka);
        try {
            if(lmdb::val v; ref().db->get(*ref().tx, key, v))
                return DB::get(ctx, v);
            return JS_NULL;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using get = bridge::Function
    <
        &Handle::get_<bridge::Number>,
        &Handle::get_<bridge::String>
    >;

    template<typename K>
    JSValue has_(JSContext *ctx, K k)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        lmdb::val key;
        DB::Allocator ka;
        DB::set(key, k, ka);
        try {
            lmdb::val v;
            return ref().db->get(*ref().tx, key, v) ? JS_TRUE : JS_FALSE;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using has = bridge::Function
    <
        &Handle::has_<bridge::Number>,
        &Handle::has_<bridge::String>
    >;

    template<typename K, typename V>
    JSValue set_(JSContext *ctx, K k, V v)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        lmdb::val key;
        DB::Allocator ka;
        DB::set(key, k, ka);

        lmdb::val val;
        DB::Allocator va;
        DB::set(val, v, va);
        try {
            ref().db->put(*ref().tx, key, val);
            return JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using set = bridge::Function
    <
        &Handle::set_<bridge::String, bridge::String>,
        &Handle::set_<bridge::Number, bridge::String>,
        &Handle::set_<bridge::String, bridge::Value>,
        &Handle::set_<bridge::Number, bridge::Value>
    >;

    std::optional<JSValue> range_(JSContext *ctx, bridge::Array &range,
        lmdb::val &b, DB::Allocator &ba,
        lmdb::val &e, DB::Allocator &ea)
    {
        if(auto v = range[0]; bridge::Number::check(ctx, +v))
        {
            bridge::Number n(ctx, v);
            DB::set(b, n, ba);
        }
        else if(bridge::String::check(ctx, +v))
        {
            auto &s = ba.emplace<2>(ctx, v.release());
            DB::set(b, s);
        }
        else if(!bridge::Null::check(ctx, +v))
        {
            return JS_ThrowTypeError(ctx, "Expecting number, string or null");
        }

        if(auto v = range[1]; bridge::Number::check(ctx, +v))
        {
            bridge::Number n(ctx, v);
            DB::set(e, n, ea);
        }
        else if(bridge::String::check(ctx, +v))
        {
            auto &s = ea.emplace<2>(ctx, v.release());
            DB::set(e, s);
        }
        else if(!bridge::Null::check(ctx, +v))
        {
            return JS_ThrowTypeError(ctx, "Expecting number, string or null");
        }
        return std::nullopt;
    }

    JSValue each_0(JSContext *ctx, bridge::Lambda lambda)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        try {
            lmdb::val k, v;
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(cr.get(k, v, MDB_FIRST)) do {
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 2>{jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
            } while(cr.get(k, v, MDB_NEXT));
            return JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue each_1(JSContext *ctx, bridge::Array range, bridge::Lambda lambda)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        if(range.size() != 2) return JS_ThrowTypeError(ctx, "Expecting array of size 2");

        DB::Allocator ba;
        lmdb::val k{NULL, 0};

        DB::Allocator ea;
        lmdb::val e{NULL, 0};

        if(auto err = range_(ctx, range, k, ba, e, ea)) return *err;
        try {
            lmdb::val v;
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(k.data() ? cr.get(k, v, MDB_SET_RANGE) : cr.get(k, v, MDB_FIRST)) do {
                if(e.data() && mdb_cmp(*ref().tx, *ref().db, k, e) > 0)
                    break;
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 2>{jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
            } while(cr.get(k, v, MDB_NEXT));
            return JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using each = bridge::Function
    <
        &Handle::each_0,
        &Handle::each_1
    >;

    JSValue map_0(JSContext *ctx, bridge::Lambda lambda)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        try {
            lmdb::val k, v;
            bridge::Strong<bridge::Array> ret{ctx, bridge::Array{ctx}};
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(cr.get(k, v, MDB_FIRST)) do {
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 2>{jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
                else ret.append(r.release());
            } while(cr.get(k, v, MDB_NEXT));
            return ret.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue map_1(JSContext *ctx, bridge::Array range, bridge::Lambda lambda)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        if(range.size() != 2) return JS_ThrowTypeError(ctx, "Expecting array of size 2");

        DB::Allocator ba;
        lmdb::val k{NULL, 0};

        DB::Allocator ea;
        lmdb::val e{NULL, 0};

        if(auto err = range_(ctx, range, k, ba, e, ea)) return *err;
        try {
            lmdb::val v;
            bridge::Strong<bridge::Array> ret{ctx, bridge::Array{ctx}};
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(k.data() ? cr.get(k, v, MDB_SET_RANGE) : cr.get(k, v, MDB_FIRST)) do {
                if(e.data() && mdb_cmp(*ref().tx, *ref().db, k, e) > 0)
                    break;
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 2>{jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
                else ret.append(r.release());
            } while(cr.get(k, v, MDB_NEXT));
            return ret.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using map = bridge::Function
    <
        &Handle::map_0,
        &Handle::map_1
    >;

    JSValue reduce_0(JSContext *ctx, bridge::Lambda lambda, bridge::Value acc)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        try {
            lmdb::val k, v;
            bridge::Strong<void> ret{ctx, acc};
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(cr.get(k, v, MDB_FIRST)) do {
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 3>{ret, jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
                else JS_FreeValue(ctx, ret.swap(r.release()));
            } while(cr.get(k, v, MDB_NEXT));
            return ret.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue reduce_1(JSContext *ctx, bridge::Array range, bridge::Lambda lambda, bridge::Value acc)
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        if(range.size() != 2) return JS_ThrowTypeError(ctx, "Expecting array of size 2");

        DB::Allocator ba;
        lmdb::val k{NULL, 0};

        DB::Allocator ea;
        lmdb::val e{NULL, 0};

        if(auto err = range_(ctx, range, k, ba, e, ea)) return *err;
        try {
            lmdb::val v;
            bridge::Strong<void> ret{ctx, acc};
            auto cr = lmdb::cursor::open(*ref().tx, *ref().db);
            if(k.data() ? cr.get(k, v, MDB_SET_RANGE) : cr.get(k, v, MDB_FIRST)) do {
                if(e.data() && mdb_cmp(*ref().tx, *ref().db, k, e) > 0)
                    break;
                bridge::Strong<void> jk{ctx, DB::get(ctx, k), false};
                bridge::Strong<void> jv{ctx, DB::get(ctx, v), false};
                if(auto r = lambda(std::array<JSValue, 3>{ret, jk, jv});
                    bridge::Error::check(ctx, +r) || (bridge::Boolean::check(ctx, +r) && !bridge::Boolean(ctx, r)))
                        return r.release();
                else JS_FreeValue(ctx, ret.swap(r.release()));
            } while(cr.get(k, v, MDB_NEXT));
            return ret.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using reduce = bridge::Function
    <
        &Handle::reduce_0,
        &Handle::reduce_1
    >;

    JSValue stat(JSContext *ctx) const
    {
        if(!ref().db) return DBException::throw_(ctx, ERROR);
        return notojs::mkstat(ctx, ref().db->stat(*ref().tx));
    }

    using ctor = bridge::Unconstructable<Handle>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Handle::funcs[] = {
    JS_CGETSET_DEF("stat", &bridge::Getter<&Handle::stat>, NULL),
    JS_CFUNC_DEF("each", 1, &Handle::each::invoke),
    JS_CFUNC_DEF("get", 1, &Handle::get::invoke),
    JS_CFUNC_DEF("del", 1, &Handle::del::invoke),
    JS_CFUNC_DEF("has", 1, &Handle::has::invoke),
    JS_CFUNC_DEF("map", 2, &Handle::map::invoke),
    JS_CFUNC_DEF("reduce", 2, &Handle::reduce::invoke),
    JS_CFUNC_DEF("set", 2, &Handle::set::invoke),
};

struct Database : bridge::Interface<Database, std::vector<std::string>>
{
    struct Name : bridge::String
    {
        using bridge::String::String;
        bool valid(std::string &message)
        {
            auto const &data = static_cast<std::string_view const &>(*this);
            message = data;

            if(data.empty()) return false;
            if(data[0] == ':') return false;
            for(std::size_t i = 0; i < data.size(); ++i)
            {
                if(!std::isalpha(data[i]) && (data[i] != ':' || data[i - 1] == ':' || i == (data.size() - 1)))
                    return false;
            }
            return data.substr(0,4) != "sys:";
        }
    };

    JSValue length(JSContext *ctx) const
    {
        return bridge::Number(ctx, static_cast<std::uint64_t>(ref().size()));
    }

    template<DB::Access a>
    JSValue tx(JSContext *ctx, bridge::Lambda lambda)
    {
        try {
            auto [tx, db] = DB(ctx).open(a, DB::USR, ref());
            std::vector<JSValue> dbs;
            std::transform(std::begin(db), std::end(db), std::back_inserter(dbs), [&tx=tx, ctx](auto &db) {
                return Handle::from(ctx, Handle_{&tx, &db});
            });
            detail::Scoped scoped([&]{
                for(auto &handle : dbs)
                {
                    Handle::wrapped(handle).db = nullptr;
                    JS_FreeValue(ctx, handle);
                }
            });
            bridge::Strong<void> res = lambda(dbs);
            if constexpr (a == DB::RO)
            {
                tx.abort();
            }
            else
            {
                if(JS_IsException(res)) tx.abort();
                else tx.commit();
            }
            return res.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue ro(JSContext *ctx, bridge::Lambda lambda)
    {
        return tx<DB::RO>(ctx, lambda);
    }

    JSValue rw(JSContext *ctx, bridge::Lambda lambda)
    {
        return tx<DB::RW>(ctx, lambda);
    }

    JSValue drop(JSContext *ctx)
    {
        try {
            auto [tx, db] = DB(ctx).open(DB::RW, DB::USR, ref());
            for(auto &d: db) d.drop(tx, true);
            tx.commit();
            return JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    using ctor = bridge::Unconstructable<Database>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Database::funcs[] = {
    JS_CGETSET_DEF("length", &bridge::Getter<&Database::length>, NULL),
    JS_CFUNC_DEF("drop", 0, &bridge::Function<&Database::drop>::invoke),
    JS_CFUNC_DEF("rw", 1, &bridge::Function<&Database::rw>::invoke),
    JS_CFUNC_DEF("ro", 1, &bridge::Function<&Database::ro>::invoke)
};

struct System : bridge::Interface<System, std::pair<DB::Namespace, std::string>>
{
    struct Name : bridge::String
    {
        using bridge::String::String;
        std::optional<DB::Namespace> valid(std::string &message)
        {
            auto const prefix = static_cast<std::string_view const &>(*this).substr(0,4);
            if("sys:" == prefix) return DB::SYS;
            if("var:" == prefix) return DB::VAR;
            return std::nullopt;
        }
    };

    System(std::reference_wrapper<std::pair<DB::Namespace, std::string>> &&rw) : Base(std::move(rw)) {}

    JSValue stat(JSContext *ctx) const
    {
        MDB_stat stat;
        try {
            auto [tx, db] = DB(ctx).open(DB::RT, ref().first, ref().second);
            stat = db.stat(tx);
            tx.abort();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
        return notojs::mkstat(ctx, stat);
    }

    JSValue drop(JSContext *ctx)
    {
        try {
            auto [tx, db] = DB(ctx).open(DB::WT, ref().first, ref().second);
            db.drop(tx, DB::VAR == ref().first);
            tx.commit();
            return JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
        return JS_UNDEFINED;
    }

    using ctor = bridge::Unconstructable<System>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const System::funcs[] = {
    JS_CGETSET_DEF("stat", &bridge::Getter<&System::stat>, NULL),
    JS_CFUNC_DEF("drop", 0, &bridge::Function<&System::drop>::invoke)
};

struct ErrorLog : bridge::Interface<ErrorLog, std::pair<DB::Namespace, std::string>, System>
{
    JSValue data(JSContext *ctx) const
    {
        try {
            lmdb::val k, v;
            bridge::Strong<bridge::Array> arr{ctx, bridge::Array{ctx}};

            auto [tx, db] = DB(ctx).open(DB::RO, ref().first, ref().second);
            auto cr = lmdb::cursor::open(tx, db);
            if(cr.get(k, v, MDB_FIRST)) do {
                auto const j = std::strlen(v.data());
                bridge::Object obj{ctx, JS_ParseJSON(ctx, v.data(), j, "<json>")};

                auto *ws = v.data() + j + 1;
                auto *we = std::find(ws, v.data() + v.size(), 0);
                obj.set("where", bridge::String{ctx, std::string_view(ws, we - ws)});

                if(auto const s = ++we - v.data(); s < v.size())
                    obj.set("stack", bridge::String{ctx, std::string_view(we, v.size() - s)});

                obj.set("time", bridge::Number{ctx, DB::db_to_host(*reinterpret_cast<std::uint64_t*>(k.data()))});
                arr.append(obj);
            } while(cr.get(k, v, MDB_NEXT));
            cr.close();
            tx.abort();
            return arr.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue drop_0(JSContext *ctx, bridge::Number keep)
    {
        std::uint64_t n{0};
        std::uint64_t const kp = static_cast<std::int64_t>(keep);
        try {
            lmdb::val k, v;

            auto [tx, db] = DB(ctx).open(DB::WT, ref().first, ref().second);
            auto cr = lmdb::cursor::open(tx, db);
            if(cr.get(k, v, MDB_FIRST)) do {
                if(DB::db_to_host(*reinterpret_cast<std::uint64_t*>(k.data())) > kp) break;
                db.del(tx, k); ++n;
            } while(cr.get(k, v, MDB_NEXT));
            cr.close();
            tx.commit();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
        return bridge::Number{ctx, n};
    }

    using drop = bridge::Function
    <
        &System::drop,
        &ErrorLog::drop_0
    >;

    using ctor = bridge::Unconstructable<ErrorLog>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const ErrorLog::funcs[] = {
    JS_CGETSET_DEF("data", &bridge::Getter<&ErrorLog::data>, NULL),
    JS_CFUNC_DEF("drop", 2, &ErrorLog::drop::invoke)
};

struct HTTPData : bridge::Interface<HTTPData, std::pair<DB::Namespace, std::string>, System>
{
    enum class CacheType : std::size_t
    {
        fetch,
        system,
        internal,
        _count
    };

    static constexpr std::array<std::string_view, static_cast<std::size_t>(CacheType::_count)> strings{
        std::string_view{"fetch"},
        std::string_view{"system"},
        std::string_view{"internal"}
    };

    JSValue data(JSContext *ctx) const
    {
        try {
            lmdb::val k, v;

            std::optional<std::string> base;
            bridge::Strong<bridge::Array> arr{ctx, bridge::Array{ctx}};
            std::array<std::unordered_map<std::string, std::uint64_t>, static_cast<std::size_t>(CacheType::_count)> paths;

            auto [tx, db] = DB(ctx).open(DB::RO, ref().first, ref().second);
            auto cr = lmdb::cursor::open(tx, db);
            if(cr.get(k, v, MDB_FIRST)) do {
                CacheType ctp;
                boost::beast::http::verb verb = boost::beast::http::verb::unknown;

                decltype(boost::urls::parse_uri(std::declval<char*>())) url;
                if(k.size() > 1 && k.data()[k.size() - 2] == 0)
                {
                    url = boost::urls::parse_uri(k.data());
                    ctp = CacheType::fetch;
                    verb = static_cast<boost::beast::http::verb>(k.data()[k.size() - 1]);
                }
                else if(k.size() && k.data()[k.size() - 1])
                {
                    url = boost::urls::parse_uri(std::string_view{k.data(), k.size()});
                    ctp = CacheType::internal;
                }
                else
                {
                    url = boost::urls::parse_uri(k.data());
                    ctp = CacheType::system;
                }

                if(url)
                {
                    auto b = std::string(url->scheme()) + "://" + std::string(url->host());
                    if(url->has_port()) b += url->port();

                    if(base != b)
                    {
                        if(base)
                        {
                            for(std::size_t i = 0; i < paths.size(); ++i)
                            {
                                if(paths[i].empty()) continue;
                                bridge::Object e{ctx};
                                bridge::Object p{ctx};
                                for(auto const &[pk, pv] : paths[i])
                                    p.set(pk.c_str(), bridge::Number{ctx, pv});

                                e.set("path", p);
                                e.set("base", bridge::String{ctx, *base});
                                e.set("type", bridge::String{ctx, strings[i]});
                                arr.append(e);
                                paths[i].clear();
                            }
                        }
                        base = std::move(b);
                    }

                    auto p = std::string(url->encoded_path());
                    if(url->has_query()) p += "?" + std::string(url->encoded_query());

                    std::string md5;
                    boost::uuids::detail::md5 hash;
                    boost::uuids::detail::md5::digest_type digest;

                    hash.process_bytes(p.data(), p.size());
                    hash.get_digest(digest);

                    const auto* bytes = reinterpret_cast<const unsigned char*>(&digest);
                    boost::algorithm::hex_lower(bytes, bytes + sizeof(digest), std::back_inserter(md5));

                    paths[static_cast<std::size_t>(ctp)].emplace(std::move(md5), v.size());
                }
            } while(cr.get(k, v, MDB_NEXT));
            cr.close();
            tx.abort();

            for(std::size_t i = 0; i < paths.size(); ++i)
            {
                if(paths[i].empty()) continue;
                bridge::Object e{ctx};
                bridge::Object p{ctx};
                for(auto const &[pk, pv] : paths[i])
                    p.set(pk.c_str(), bridge::Number{ctx, pv});

                e.set("path", p);
                e.set("base", bridge::String{ctx, *base});
                e.set("type", bridge::String{ctx, strings[i]});
                arr.append(e);
            }
            return arr.release();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
    }

    JSValue drop_0(JSContext *ctx, bridge::String t)
    {
        CacheType type = CacheType::_count;
        auto const ctype = static_cast<std::string_view const &>(t);
        for(std::size_t i = 0; i < strings.size(); ++i)
        {
            if(strings[i] == ctype)
            {
                type = static_cast<CacheType>(i);
                break;
            }
        }
        if(CacheType::_count == type)
            return JS_ThrowTypeError(ctx, "Invalid cache type: [%s]", ctype.data());

        std::uint64_t removed{0};
        try {
            lmdb::val k, v;

            auto [tx, db] = DB(ctx).open(DB::RW, ref().first, ref().second);
            auto cr = lmdb::cursor::open(tx, db);

            if(cr.get(k, v, MDB_FIRST)) do {
                if(std::invoke([&]{
                    if(k.size() > 1 && k.data()[k.size() - 2] == 0)
                    {
                        return type == CacheType::fetch;
                    }
                    else if(k.size() && k.data()[k.size() - 1])
                    {
                        return type == CacheType::internal;
                    }
                    return type == CacheType::system;
                }))
                {
                    db.del(tx, k); ++removed;
                }
            } while(cr.get(k, v, MDB_NEXT));
            cr.close();
            tx.commit();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
        return bridge::Number{ctx, removed};
    }

    JSValue drop_1(JSContext *ctx, bridge::String t, bridge::String pref)
    {
        CacheType type = CacheType::_count;
        auto const ctype = static_cast<std::string_view const &>(t);
        for(std::size_t i = 0; i < strings.size(); ++i)
        {
            if(strings[i] == ctype)
            {
                type = static_cast<CacheType>(i);
                break;
            }
        }
        if(CacheType::_count == type)
            return JS_ThrowTypeError(ctx, "Invalid cache type: [%s]", ctype.data());

        std::uint64_t removed{0};

        auto const &cpref = static_cast<std::string_view const &>(pref);
        if(auto const url = boost::urls::parse_uri(cpref); !url || !url->encoded_path().empty())
            return JS_ThrowTypeError(ctx, "Invalid prefix: [%s]", cpref.data());
        else try
        {
            lmdb::val k, v;
            k.assign(cpref.data(), cpref.size());
            decltype(boost::urls::parse_uri(std::declval<char*>())) u;

            auto [tx, db] = DB(ctx).open(DB::RW, ref().first, ref().second);
            auto cr = lmdb::cursor::open(tx, db);

            if(cr.get(k, v, MDB_SET_RANGE)) do {
                if(cpref != std::string_view{k.data(), k.size()}.substr(0, cpref.size())) break;
                if(std::invoke([&]{
                    if(k.size() > 1 && k.data()[k.size() - 2] == 0)
                    {
                        if(type == CacheType::fetch) u = boost::urls::parse_uri(k.data());
                    }
                    else if(k.size() && k.data()[k.size() - 1])
                    {
                        if(type == CacheType::internal) u = boost::urls::parse_uri(std::string_view{k.data(), k.size()});
                    }
                    else if(type == CacheType::system)
                    {
                        u = boost::urls::parse_uri(k.data());
                    }
                }); u && u->host() == url->host())
                {
                    db.del(tx, k); ++removed;
                }
            } while(cr.get(k, v, MDB_NEXT));
            cr.close();
            tx.commit();
        } catch(std::runtime_error const &e) {
            return DBException::throw_(ctx, e);
        }
        return bridge::Number{ctx, removed};
    }

    using drop = bridge::Function
    <
        &System::drop,
        &HTTPData::drop_0,
        &HTTPData::drop_1
    >;

    using ctor = bridge::Unconstructable<HTTPData>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTTPData::funcs[] = {
    JS_CGETSET_DEF("data", &bridge::Getter<&HTTPData::data>, NULL),
    JS_CFUNC_DEF("drop", 2, &HTTPData::drop::invoke)
};

JSValue backup(JSContext *ctx, fs::facade::Path path)
{
    if(auto [native, writable] = path->native(); !writable)
        return DBException::throw_(ctx, std::make_error_code(std::errc::read_only_file_system));
    else try {
        if(!std::filesystem::exists(native))
            std::filesystem::create_directories(native);
        DB(ctx).backup(native);
    } catch(std::runtime_error const &re) {
        return DBException::throw_(ctx, re);
    }
    return JS_UNDEFINED;
}

JSValue open(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
{
    std::string message;
    std::vector<std::string> names;
    for(int i = 0; i < argc; ++i)
    {
        if(!i && 1 == argc && System::Name::check(ctx, argv)) {
            auto name = System::Name(ctx, *argv);
            if(auto type = name.valid(message)) {
                if("sys:errorlog" == static_cast<std::string_view const &>(name))
                    return ErrorLog::from(ctx, {*type, static_cast<std::string>(name).substr(4)});
                else if("sys:httpdata" == static_cast<std::string_view const &>(name))
                    return HTTPData::from(ctx, {*type, static_cast<std::string>(name).substr(4)});
                return System::from(ctx, {*type, static_cast<std::string>(name).substr(4)});
            }
        }
        if(!Database::Name::check(ctx, argv + i))
            return JS_ThrowTypeError(ctx, "No matching function overload found");
        if(auto name = Database::Name(ctx, argv[i]); !name.valid(message))
            return JS_ThrowTypeError(ctx, "Invalid database name: [%s]", message.c_str());
        else names.push_back(static_cast<std::string>(name));
    }
    if(names.empty())
        return JS_ThrowTypeError(ctx, "No matching function overload found");

    return Database::from(ctx, std::move(names));
}

JSValue info(JSContext *ctx)
{
    auto res = DB(ctx).info();
    bridge::Object obj{ctx};
    obj.set("mapsize", bridge::Number(ctx, static_cast<std::uint64_t>(res.me_mapsize)));
    obj.set("last_pgno", bridge::Number(ctx, static_cast<std::uint64_t>(res.me_last_pgno)));
    obj.set("last_txnid", bridge::Number(ctx, static_cast<std::uint64_t>(res.me_last_txnid)));
    obj.set("maxreaders", bridge::Number(ctx, static_cast<std::uint64_t>(res.me_maxreaders)));
    obj.set("numreaders", bridge::Number(ctx, static_cast<std::uint64_t>(res.me_numreaders)));
    return obj;
}

JSValue stat(JSContext *ctx)
{
    auto res = DB(ctx).stat();
    return mkstat(ctx, res);
}

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("backup", 1, &bridge::Function<backup>::invoke),
    JS_CFUNC_DEF("info", 0, &bridge::Function<info>::invoke),
    JS_CFUNC_DEF("open", 1, &open),
    JS_CFUNC_DEF("stat", 0, &bridge::Function<stat>::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Database::init(ctx);
    System::init(ctx);
    ErrorLog::init(ctx);
    HTTPData::init(ctx);
    Handle::init(ctx);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_db()
{
    Database::init();
    System::init();
    ErrorLog::init();
    HTTPData::init();
    Handle::init();
}

void notojs_init_db(JSRuntime *rt)
{
    Database::init(rt);
    System::init(rt);
    HTTPData::init(rt);
    ErrorLog::init(rt);
    Handle::init(rt);
}

void notojs_init_db(boost::property_tree::ptree const &) {}

JSModuleDef *notojs_init_db(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // namespace notojs
