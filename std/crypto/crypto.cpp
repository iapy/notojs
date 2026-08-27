#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <lmdbxx/lmdb++.h>
#include <boost/hana.hpp>
#include <algorithm>
#include <array>
#include <climits>
#include <unordered_map>

#include <bridge.hpp>
#include <notodb.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {

using Algorithm = const EVP_MD *(void);

std::unordered_map<std::string_view, Algorithm*> const algorithms{
    {"md5", &EVP_md5},
    {"sha1", &EVP_sha1},
    {"sha224", &EVP_sha224},
    {"sha256", &EVP_sha256},
    {"sha384", &EVP_sha384},
    {"sha512", &EVP_sha512}
};

JSValue digest(JSContext *ctx, Algorithm algo, std::uint8_t const *data, std::size_t size)
{
    EVP_MD_CTX *digest = EVP_MD_CTX_new();
    if(!digest)
        return JS_ThrowInternalError(ctx, "OpenSSL EVP_MD_CTX_new failed");

    unsigned int hlen = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];

    if(1 != EVP_DigestInit_ex(digest, algo(), nullptr))
    {
        EVP_MD_CTX_free(digest);
        return JS_ThrowInternalError(ctx, "OpenSSL EVP_DigestInit_ex failed");
    }

    if(size && 1 != EVP_DigestUpdate(digest, data, size))
    {
        EVP_MD_CTX_free(digest);
        return JS_ThrowInternalError(ctx, "OpenSSL EVP_DigestUpdate failed");
    }

    if(1 != EVP_DigestFinal_ex(digest, hash, &hlen))
    {
        EVP_MD_CTX_free(digest);
        return JS_ThrowInternalError(ctx, "OpenSSL EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(digest);
    return bridge::ArrayBuffer(ctx, hash, hlen);
}

JSValue hex(JSContext *ctx, bridge::ArrayBuffer arr)
{
    static constexpr char digits[] = "0123456789abcdef";
    auto const [data, size] = arr.data();

    std::string result;
    result.resize(size * 2);

    for(std::size_t i = 0; i < size; ++i)
    {
        auto const byte = data[i];
        result[2 * i] = digits[byte >> 4];
        result[2 * i + 1] = digits[byte & 0x0f];
    }

    return bridge::String{ctx, std::move(result)};
}

JSValue hash_0(JSContext *ctx, bridge::Lambda algo, bridge::ArrayBuffer arr)
{
    bridge::Strong<bridge::String> name(ctx, JS_GetPropertyStr(ctx, algo, "name"));

    auto algorithm = algorithms.find(name);
    if(algorithm == std::end(algorithms))
        return JS_ThrowInternalError(ctx, "unsupported algorithm [%s]", static_cast<std::string_view const &>(name).data());

    auto const [data, size] = arr.data();
    return digest(ctx, algorithm->second, data, size);
}

JSValue hash_1(JSContext *ctx, bridge::String name, bridge::ArrayBuffer arr)
{
    auto algorithm = algorithms.find(name);
    if(algorithm == std::end(algorithms))
        return JS_ThrowInternalError(ctx, "unsupported algorithm [%s]", static_cast<std::string_view const &>(name).data());

    auto const [data, size] = arr.data();
    return digest(ctx, algorithm->second, data, size);
}

template<Algorithm algo>
JSValue hash_t(JSContext *ctx, bridge::ArrayBuffer arr)
{
    auto const [data, size] = arr.data();
    return digest(ctx, algo, data, size);
}

using hash = bridge::Function<&hash_0, &hash_1>;

JSValue hmac_0(JSContext *ctx, bridge::Lambda algo, bridge::ArrayBuffer key, bridge::ArrayBuffer arr)
{
    bridge::Strong<bridge::String> name(ctx, JS_GetPropertyStr(ctx, algo, "name"));

    auto algorithm = algorithms.find(name);
    if(algorithm == std::end(algorithms))
        return JS_ThrowInternalError(ctx, "unsupported algorithm [%s]", static_cast<std::string_view const &>(name).data());

    auto const [key_data, key_size] = key.data();
    auto const [data, size] = arr.data();

    unsigned int hlen = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];

    if(!HMAC(algorithm->second(), key_data, static_cast<int>(key_size), data, size, hash, &hlen))
        return JS_ThrowInternalError(ctx, "OpenSSL HMAC failed");

    return bridge::ArrayBuffer(ctx, hash, hlen);
}

JSValue hmac_1(JSContext *ctx, bridge::String name, bridge::ArrayBuffer key, bridge::ArrayBuffer arr)
{
    auto algorithm = algorithms.find(name);
    if(algorithm == std::end(algorithms))
        return JS_ThrowInternalError(ctx, "unsupported algorithm [%s]", static_cast<std::string_view const &>(name).data());

    auto const [key_data, key_size] = key.data();
    auto const [data, size] = arr.data();

    unsigned int hlen = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];

    if(!HMAC(algorithm->second(), key_data, static_cast<int>(key_size), data, size, hash, &hlen))
        return JS_ThrowInternalError(ctx, "OpenSSL HMAC failed");

    return bridge::ArrayBuffer(ctx, hash, hlen);
}

using hmac = bridge::Function<&hmac_0, &hmac_1>;

JSValue random_1(JSContext *ctx, bridge::Number count)
{
    auto const n = static_cast<std::int64_t>(count);
    if(n < 0)
        return JS_ThrowRangeError(ctx, "Byte count must not be negative");
    if(n > INT_MAX)
        return JS_ThrowRangeError(ctx, "Byte count is too large");

    std::vector<std::uint8_t> data(static_cast<std::size_t>(n));
    if(!data.empty() && RAND_bytes(data.data(), static_cast<int>(data.size())) != 1)
        return JS_ThrowInternalError(ctx, "OpenSSL RAND_bytes failed");

    return bridge::ArrayBuffer(ctx, data.data(), data.size());
}

JSValue random_2(JSContext *ctx, bridge::Value array)
{
    std::size_t offset;
    std::size_t length;

    bridge::Strong<bridge::Value> buffer{ctx, JS_GetTypedArrayBuffer(ctx, array, &offset, &length, nullptr)};
    if(JS_IsException(buffer)) return buffer.release();

    std::size_t dlen;
    std::uint8_t *data = JS_GetArrayBuffer(ctx, &dlen, buffer);
    if(!data) return JS_EXCEPTION;

    if(offset > dlen || length > dlen - offset)
        return JS_ThrowRangeError(ctx, "TypedArray range exceeds its ArrayBuffer");

    if(length && RAND_bytes(data + offset, static_cast<int>(length)) != 1)
        return JS_ThrowInternalError(ctx, "OpenSSL RAND_bytes failed");

    return JS_DupValue(ctx, array);
}

using random = bridge::Function<&random_1, &random_2>;

JSValue unhex(JSContext *ctx, bridge::String str)
{
    auto const value = static_cast<std::string_view const &>(str);
    if(value.size() % 2)
        return JS_ThrowRangeError(ctx, "Hex string must contain an even number of characters");

    auto const nibble = [](char c) -> int {
        if(c >= '0' && c <= '9') return c - '0';
        if(c >= 'a' && c <= 'f') return c - 'a' + 10;
        if(c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    std::vector<std::uint8_t> data(value.size() / 2);
    for(std::size_t i = 0; i < data.size(); ++i)
    {
        auto const hi = nibble(value[2 * i]);
        auto const lo = nibble(value[2 * i + 1]);
        if(hi < 0 || lo < 0)
            return JS_ThrowRangeError(ctx, "Invalid hex character at offset %zu", hi < 0 ? 2 * i : 2 * i + 1);

        data[i] = static_cast<std::uint8_t>((hi << 4) | lo);
    }

    return bridge::ArrayBuffer(ctx, data.data(), data.size());
}

JSValue uuid(JSContext *ctx)
{
    boost::uuids::random_generator gen;
    return bridge::String(ctx, boost::uuids::to_string(gen()));
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("hex", 1, &bridge::Function<&hex>::invoke),
    JS_CFUNC_DEF("hash", 2, &hash::invoke),
    JS_CFUNC_DEF("hmac", 2, &hmac::invoke),
    JS_CFUNC_DEF("random", 1, &random::invoke),
    JS_CFUNC_DEF("unhex", 1, &bridge::Function<&unhex>::invoke),
    JS_CFUNC_DEF("uuid", 0, &bridge::Function<&uuid>::invoke),

    JS_CFUNC_DEF("md5", 1, &bridge::Function<&hash_t<EVP_md5>>::invoke),
    JS_CFUNC_DEF("sha1", 1, &bridge::Function<&hash_t<EVP_sha1>>::invoke),
    JS_CFUNC_DEF("sha224", 1, &bridge::Function<&hash_t<EVP_sha224>>::invoke),
    JS_CFUNC_DEF("sha256", 1, &bridge::Function<&hash_t<EVP_sha256>>::invoke),
    JS_CFUNC_DEF("sha384", 1, &bridge::Function<&hash_t<EVP_sha384>>::invoke),
    JS_CFUNC_DEF("sha512", 1, &bridge::Function<&hash_t<EVP_sha512>>::invoke)
};

struct Storage : bridge::Interface<Storage, std::string>
{
    using Key = std::array<std::uint8_t, 32>;

    static bool keyed_hash(Key const &key, std::string_view data, Key &result)
    {
        unsigned int size = 0;
        return HMAC(
            EVP_sha256(),
            key.data(), static_cast<int>(key.size()),
            reinterpret_cast<unsigned char const *>(data.data()), data.size(),
            result.data(), &size
        ) && size == result.size();
    }

    static bool encrypt(Key const &key, std::string_view data, std::string_view aad, std::vector<std::uint8_t> &result)
    {
        static constexpr std::size_t nonce_size = 12;
        static constexpr std::size_t tag_size = 16;
        static constexpr std::uint8_t version = 1;

        if(data.size() > INT_MAX || aad.size() > INT_MAX) return false;

        result.resize(1 + nonce_size + data.size() + tag_size);
        result[0] = version;

        auto *nonce = result.data() + 1;
        auto *ciphertext = nonce + nonce_size;
        auto *tag = ciphertext + data.size();
        if(RAND_bytes(nonce, static_cast<int>(nonce_size)) != 1) return false;

        EVP_CIPHER_CTX *cipher = EVP_CIPHER_CTX_new();
        if(!cipher) return false;

        int length = 0;
        int total = 0;
        bool const success =
            EVP_EncryptInit_ex(cipher, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
            EVP_CIPHER_CTX_ctrl(cipher, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce_size), nullptr) == 1 &&
            EVP_EncryptInit_ex(cipher, nullptr, nullptr, key.data(), nonce) == 1 &&
            (aad.empty() || (
                EVP_EncryptUpdate(
                    cipher, nullptr, &length,
                    reinterpret_cast<unsigned char const *>(aad.data()), static_cast<int>(aad.size())
                ) == 1
            )) &&
            (data.empty() || (
                EVP_EncryptUpdate(
                    cipher, ciphertext, &length,
                    reinterpret_cast<unsigned char const *>(data.data()), static_cast<int>(data.size())
                ) == 1 &&
                (total = length, true)
            )) &&
            EVP_EncryptFinal_ex(cipher, ciphertext + total, &length) == 1 &&
            (total += length) == static_cast<int>(data.size()) &&
            EVP_CIPHER_CTX_ctrl(cipher, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag_size), tag) == 1;

        EVP_CIPHER_CTX_free(cipher);
        if(!success) result.clear();
        return success;
    }

    static bool decrypt(Key const &key, std::string_view data, std::string_view aad, std::vector<std::uint8_t> &result)
    {
        static constexpr std::size_t nonce_size = 12;
        static constexpr std::size_t tag_size = 16;
        static constexpr std::uint8_t version = 1;
        static constexpr std::size_t overhead = 1 + nonce_size + tag_size;

        if(data.size() < overhead ||
           static_cast<std::uint8_t>(data[0]) != version ||
           data.size() - overhead > INT_MAX || aad.size() > INT_MAX)
            return false;

        auto const *nonce = reinterpret_cast<unsigned char const *>(data.data() + 1);
        auto const *ciphertext = nonce + nonce_size;
        auto const ciphertext_size = data.size() - overhead;
        auto const *tag = ciphertext + ciphertext_size;

        result.resize(ciphertext_size + EVP_MAX_BLOCK_LENGTH);
        EVP_CIPHER_CTX *cipher = EVP_CIPHER_CTX_new();
        if(!cipher)
        {
            result.clear();
            return false;
        }

        int length = 0;
        int total = 0;
        bool const success =
            EVP_DecryptInit_ex(cipher, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
            EVP_CIPHER_CTX_ctrl(cipher, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce_size), nullptr) == 1 &&
            EVP_DecryptInit_ex(cipher, nullptr, nullptr, key.data(), nonce) == 1 &&
            (aad.empty() || (
                EVP_DecryptUpdate(
                    cipher, nullptr, &length,
                    reinterpret_cast<unsigned char const *>(aad.data()), static_cast<int>(aad.size())
                ) == 1
            )) &&
            (ciphertext_size == 0 || (
                EVP_DecryptUpdate(
                    cipher, result.data(), &length,
                    ciphertext, static_cast<int>(ciphertext_size)
                ) == 1 &&
                (total = length, true)
            )) &&
            EVP_CIPHER_CTX_ctrl(
                cipher, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag_size),
                const_cast<unsigned char *>(tag)
            ) == 1 &&
            EVP_DecryptFinal_ex(cipher, result.data() + total, &length) == 1 &&
            (total += length) == static_cast<int>(ciphertext_size);

        EVP_CIPHER_CTX_free(cipher);
        if(success) result.resize(total);
        else result.clear();
        return success;
    }


    static bool getkey(JSContext *ctx, Key &result)
    {
        using Data = std::variant_alternative_t<0, notojs::DB::Data>;

        auto const assign = [&result](Data const &data) {
            if(data.size() != result.size()) return false;
            std::copy(data.begin(), data.end(), result.begin());
            return true;
        };

        return std::visit(boost::hana::overload_linearly(
            assign,
            [&](std::error_code const &error) -> bool {
                if(error != std::errc::no_such_file_or_directory) return false;

                Data candidate(result.size());
                if(RAND_bytes(candidate.data(), static_cast<int>(candidate.size())) != 1)
                    return false;

                auto installed = notojs::DB::data(ctx, "data.aes", std::move(candidate));
                return std::visit(boost::hana::overload_linearly(
                    assign,
                    [](std::error_code const &) { return false; }
                ), installed);
            }
        ), notojs::DB::data(ctx, "data.aes"));
    }

    Storage(bridge::String name)
    {
        ref() = name;
    }

    JSValue clear(JSContext *ctx)
    {
        try {
            auto [tx, db] =  notojs::DB{ctx}.open(notojs::DB::Access::RW, notojs::DB::VAR, std::string_view{ref().c_str(), ref().size()});
            db.drop(tx, true);
            return tx.commit(), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue getItem(JSContext *ctx, bridge::String k)
    {
        Key master;
        if(!getkey(ctx, master))
            return JS_ThrowInternalError(ctx, "Failed to load AES key");

        Key value_key;
        if(!keyed_hash(master, "notojs.storage.value", value_key))
            return JS_ThrowInternalError(ctx, "OpenSSL HMAC failed");

        auto const &name = static_cast<std::string_view const &>(k);
        std::string identity = ref();
        identity.push_back('\0');
        identity.append(name);

        try {
            lmdb::val key{name.data(), name.size()};
            auto [tx, db] = notojs::DB{ctx}.open(notojs::DB::Access::RO, notojs::DB::VAR, std::string_view{ref().c_str(), ref().size()});

            lmdb::val value;
            if(!db.get(tx, key, value))
            {
                tx.abort();
                return JS_NULL;
            }

            std::vector<std::uint8_t> decrypted;
            bool const success = decrypt(
                value_key,
                std::string_view{value.data(), value.size()},
                identity,
                decrypted);
            tx.abort();

            if(!success)
                return JS_ThrowInternalError(ctx, "Failed to authenticate encrypted value");

            return bridge::String{ctx, std::string{decrypted.begin(), decrypted.end()}};
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue key(JSContext *ctx, bridge::Number m)
    {
        auto index = static_cast<std::int64_t>(m);
        if(index < 0) return JS_NULL;

        try {
            auto [tx, db] = notojs::DB{ctx}.open(
                notojs::DB::Access::RO, notojs::DB::VAR,
                std::string_view{ref().c_str(), ref().size()});
            auto cursor = lmdb::cursor::open(tx, db);
            lmdb::val stored_index;
            lmdb::val stored_value;

            bool found = cursor.get(stored_index, stored_value, MDB_FIRST);
            while(found && index-- > 0)
                found = cursor.get(stored_index, stored_value, MDB_NEXT);

            if(!found)
            {
                cursor.close();
                tx.abort();
                return JS_NULL;
            }

            JSValue result = bridge::String{
                ctx, std::string_view{stored_index.data(), stored_index.size()}};
            cursor.close();
            tx.abort();
            return result;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue setItem(JSContext *ctx, bridge::String k, bridge::String v)
    {
        Key master;
        if(!getkey(ctx, master))
            return JS_ThrowInternalError(ctx, "Failed to load AES key");

        Key value_key;
        if(!keyed_hash(master, "notojs.storage.value", value_key))
            return JS_ThrowInternalError(ctx, "OpenSSL HMAC failed");

        auto const &name = static_cast<std::string_view const &>(k);
        std::string identity = ref();
        identity.push_back('\0');
        identity.append(name);

        std::vector<std::uint8_t> encrypted;
        if(!encrypt(value_key, static_cast<std::string_view const &>(v), identity, encrypted))
            return JS_ThrowInternalError(ctx, "OpenSSL AES-256-GCM encryption failed");

        try {
            lmdb::val key{name.data(), name.size()};
            lmdb::val value{encrypted.data(), encrypted.size()};
            auto [tx, db] = notojs::DB{ctx}.open(notojs::DB::Access::RW, notojs::DB::VAR, std::string_view{ref().c_str(), ref().size()});
            db.put(tx, key, value);
            return tx.commit(), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue removeItem(JSContext *ctx, bridge::String k)
    {
        auto const &name = static_cast<std::string_view const &>(k);
        try {
            lmdb::val key{name.data(), name.size()};
            auto [tx, db] = notojs::DB{ctx}.open(
                notojs::DB::Access::RW, notojs::DB::VAR,
                std::string_view{ref().c_str(), ref().size()});
            db.del(tx, key);
            return tx.commit(), JS_UNDEFINED;
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    JSValue get_length(JSContext *ctx) const
    {
        try {
            auto [tx, db] = notojs::DB{ctx}.open(notojs::DB::Access::RW, notojs::DB::VAR, std::string_view{ref().c_str(), ref().size()});
            auto rs = static_cast<std::uint64_t>(db.stat(tx).ms_entries);
            tx.abort();
            return bridge::Number{ctx, rs};
        } catch(std::runtime_error const &e) {
            return JS_ThrowInternalError(ctx, "std::runtime_error [%s]", e.what());
        }
    }

    using ctor = bridge::Constructor<Storage(bridge::String)>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Storage::funcs[] = {
    JS_CFUNC_DEF("key", 1, &bridge::Function<&Storage::key>::invoke),
    JS_CFUNC_DEF("clear", 0, &bridge::Function<&Storage::clear>::invoke),
    JS_CFUNC_DEF("getItem", 1, &bridge::Function<&Storage::getItem>::invoke),
    JS_CFUNC_DEF("setItem", 2, &bridge::Function<&Storage::setItem>::invoke),
    JS_CFUNC_DEF("removeItem", 1, &bridge::Function<&Storage::removeItem>::invoke),

    JS_CGETSET_DEF("length", &bridge::Getter<&Storage::get_length>, NULL)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Storage::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, Storage::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
