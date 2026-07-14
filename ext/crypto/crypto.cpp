#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <unordered_map>
#include <bridge.hpp>

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

JSValue random(JSContext *ctx, bridge::Number count)
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

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("hex", 1, &bridge::Function<&hex>::invoke),
    JS_CFUNC_DEF("hash", 2, &hash::invoke),
    JS_CFUNC_DEF("hmac", 2, &hmac::invoke),
    JS_CFUNC_DEF("random", 1, &bridge::Function<&random>::invoke),
    JS_CFUNC_DEF("unhex", 1, &bridge::Function<&unhex>::invoke),

    JS_CFUNC_DEF("md5", 1, &bridge::Function<&hash_t<EVP_md5>>::invoke),
    JS_CFUNC_DEF("sha1", 1, &bridge::Function<&hash_t<EVP_sha1>>::invoke),
    JS_CFUNC_DEF("sha224", 1, &bridge::Function<&hash_t<EVP_sha224>>::invoke),
    JS_CFUNC_DEF("sha256", 1, &bridge::Function<&hash_t<EVP_sha256>>::invoke),
    JS_CFUNC_DEF("sha384", 1, &bridge::Function<&hash_t<EVP_sha384>>::invoke),
    JS_CFUNC_DEF("sha512", 1, &bridge::Function<&hash_t<EVP_sha512>>::invoke)
};

static int init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
