#pragma once
#include <boost/config.hpp>
#include <filesystem>
#include <string>
#include <variant>
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

    MDB_stat stat() const;
    MDB_envinfo info() const;
    void backup(std::filesystem::path const &p) const;

    using Data = std::variant<std::vector<std::uint8_t>, std::error_code>;

    static Data data(JSContext *, std::filesystem::path);
    static Data data(JSContext *, std::filesystem::path, std::variant_alternative_t<0, Data> &&);

protected:
    BOOST_FORCEINLINE DB()
    : env{nullptr} {}

    MDB_env *env;
};

} // namespace notojs
