#include <notojs/script/crypto.hpp>
#include <notojs/global.hpp>
#include <notojs/module.hpp>
#include <global.hpp>
#include <bridge.hpp>

#include <openssl/rand.h>

namespace notojs {
namespace {

JSValue digest(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int, JSValue *data)
{
    std::string algo;
    if(argc < 2 || !bridge::String::check(ctx, argv))
        return JS_ThrowTypeError(ctx, "No matching function overload found");

    JSValue funcs[2] = {JS_UNDEFINED, JS_UNDEFINED};
    JSValue promise = JS_NewPromiseCapability(ctx, funcs);
    if(JS_IsException(promise))
    {
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        return promise;
    }

    auto a = bridge::String{ctx, *argv};
    if(auto const &n = static_cast<std::string_view const &>(a); "SHA-1" == n)
        algo = "sha1";
    else if("SHA-256" == n)
        algo = "sha256";
    else if("SHA-384" == n)
        algo = "sha384";
    else if("SHA-512" == n)
        algo = "sha512";
    else
    {
        JS_ThrowInternalError(ctx, "unsupported algorithm [%s]", n.data());
        bridge::Strong<void> error{ctx, JS_GetException(ctx), false};
        JSValue settled = JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, +error);
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        if(JS_IsException(settled))
        {
            JS_FreeValue(ctx, promise);
            return settled;
        }
        JS_FreeValue(ctx, settled);
        return promise;
    }

    bridge::Object lib{ctx, data[0]};
    auto hash = lib.get<bridge::Lambda>("hash");
    if(!hash)
    {
        JS_FreeValue(ctx, funcs[0]);
        JS_FreeValue(ctx, funcs[1]);
        JS_FreeValue(ctx, promise);
        return JS_ThrowTypeError(ctx, "crypto.hash is not a function");
    }

    bridge::Strong<bridge::String> alg{ctx, bridge::String{ctx, std::move(algo)}};
    auto ret = (*hash)(std::array<JSValue, 2>{alg, argv[1]});
    JSValue settled;

    if(JS_IsException(*ret))
    {
        bridge::Strong<void> exc{ctx, JS_GetException(ctx), false};
        settled = JS_Call(ctx, funcs[1], JS_UNDEFINED, 1, +exc);
    }
    else
    {
        settled = JS_Call(ctx, funcs[0], JS_UNDEFINED, 1, +ret);
    }

    JS_FreeValue(ctx, funcs[0]);
    JS_FreeValue(ctx, funcs[1]);

    if(JS_IsException(settled))
    {
        JS_FreeValue(ctx, promise);
        return settled;
    }
    else
    {
        JS_FreeValue(ctx, settled);
        return promise;
    }
}

JSValue init(JSContext *ctx, bool cleanup)
{
    JSValue crypto = Global::Context::ptr(ctx)->load(ctx, "crypto.so");
    if(JS_IsException(crypto) || JS_IsError(ctx, crypto)) return crypto;

    JSValue glob = JS_GetGlobalObject(ctx);
    JSValue impl = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, impl, "getRandomValues", JS_GetPropertyStr(ctx, crypto, "random"));
    JS_SetPropertyStr(ctx, impl, "randomUUID", JS_GetPropertyStr(ctx, crypto, "uuid"));

    JSValue subtle = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, subtle, "digest", JS_NewCFunctionData(ctx, &digest, 2, 0, 1, &crypto));
    JS_FreeValue(ctx, crypto);
    JS_DefinePropertyValueStr(ctx, impl, "subtle", subtle, JS_PROP_ENUMERABLE);
    JS_SetPropertyStr(ctx, glob, "crypto", impl);
    JS_FreeValue(ctx, glob);
    if(cleanup)
        Global::Context::ptr(ctx)->cleanup.insert("crypto");

    return JS_UNDEFINED;
}

} // namespace

JSValue notojs_init_crypto(JSContext *ctx)
{
    return init(ctx, false);
}

JSValue notojs_init_crypto(JSContext *ctx, ScriptConfig cfg)
{
    auto scope = cfg.get<ScriptConfig::Scope>("scope");
    return init(ctx, (scope && "cell" == static_cast<std::string_view const &>(*scope)));
}

} // namespace notojs
