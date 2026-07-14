#pragma once
#include <boost/config.hpp>
#include <filesystem>
#include <string>
#ifdef BRIDGE_DEFINE_STRUCT
#include <variant>
#endif
#include <vector>
#include <utility>

struct MDB_env;
struct MDB_stat;
struct MDB_envinfo;
struct JSContext;

namespace lmdb {
    class dbi;
    class txn;
    class val;
} // namespace lmdb

namespace notojs {

class DB
{
public:
    explicit DB(MDB_env *env)
    : env{env} {}

    explicit DB(JSContext *ctx);

    enum Access { RO, RT, RW, WT };
    enum Namespace { SYS, USR, VAR };

    std::pair<lmdb::txn, lmdb::dbi> http(Access a = RO) const;
    std::pair<lmdb::txn, lmdb::dbi> pkgs(Access a = RO) const;

    std::pair<lmdb::txn, lmdb::dbi> open(Access a, Namespace ns, std::string_view const &) const;
    std::pair<lmdb::txn, std::vector<lmdb::dbi>> open(Access a, Namespace ns, std::vector<std::string> const &) const;

#ifdef BRIDGE_DEFINE_STRUCT
    using Allocator = std::variant
    <
        std::monostate,
        std::array<std::uint8_t, 9>,
        bridge::Strong<bridge::String>
    >;

    enum class Type
    {
        Number,
        Object,
        String
    };

    static void set(lmdb::val &, bridge::String &);

    template<typename T>
    static auto set(lmdb::val &, T &, Allocator &) -> std::enable_if_t
    <
        std::is_same_v<T, bridge::Number>
    ||  std::is_same_v<T, bridge::String>
    ||  std::is_same_v<T, bridge::Value>
    >;

    static JSValue get(JSContext *, lmdb::val const &);
    static Type type(lmdb::val const &);
#endif

    MDB_stat stat() const;
    MDB_envinfo info() const;
    void backup(std::filesystem::path const &p) const;

    static std::uint64_t db_to_host(std::uint64_t);
    static std::uint64_t host_to_db(std::uint64_t);

    class Storage;
protected:
    BOOST_FORCEINLINE DB()
    : env{nullptr} {}

    MDB_env *env;
};

#ifdef BRIDGE_DEFINE_STRUCT
class DB::Storage : protected DB
{
public:
    Storage() = default;
    Storage(Storage &&) = default;
    Storage(Storage const &) = default;

    Storage& operator = (Storage &&) = default;
    Storage& operator = (Storage const &) = default;

    Storage(JSContext *ctx, std::string_view const &name);

    JSValue key(bridge::Number const &) const;

    void set(bridge::String &, bridge::String &);
    void set(bridge::String &, bridge::Value &);

    JSValue get(bridge::String &);

    void remove(bridge::String &);
    void remove() const;

    std::uint64_t count() const;
private:
    JSContext *ctx;
    std::string name;
};
#endif
} // namespace notojs
