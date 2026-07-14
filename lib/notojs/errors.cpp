#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/logger.hpp>
#include <notojs/server.hpp>
#include <notodb.hpp>

#include <lmdbxx/lmdb++.h>
#include <atomic>
#include <chrono>

namespace notojs {
namespace {

constexpr std::string_view errors{"errorlog"};
std::atomic<std::uint32_t> index{0};

} // namespace

void Errors::error(std::string const &source, std::string &&json, std::optional<std::string> &&stack)
{
    std::array<std::uint8_t, sizeof(std::uint64_t) + sizeof(std::uint32_t)> key;
    *reinterpret_cast<std::uint64_t*>(&key[0]) = DB::host_to_db(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    *reinterpret_cast<std::uint32_t*>(&key[sizeof(std::uint64_t)]) = ++index;

    std::vector<std::uint8_t> val;
    val.insert(val.end(), json.c_str(), json.c_str() + json.size());
    val.insert(val.end(), std::uint8_t{0});
    val.insert(val.end(), source.c_str(), source.c_str() + source.size());
    val.insert(val.end(), std::uint8_t{0});
    if(stack) val.insert(val.end(), stack->c_str(), stack->c_str() + stack->size());

    lmdb::val k{&key[0], key.size()};
    lmdb::val v{&val[0], val.size()};
    try {
        auto [tx, db] = DB(get<Folder>().env()).open(DB::RW, DB::SYS, errors);
        db.put(tx, k, v);
        tx.commit();
    } catch(std::runtime_error const &e) {
        NOTOJS_LOG("LMDB error", (notojs::values(std::string{e.what()})));
    }
    NOTOJS_LOG(source, (notojs::values.raw(std::move(json))));
}

} // namespace notojs
