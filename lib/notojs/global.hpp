#pragma once
#include <notojs/detail/config.hpp>
#include <boost/url.hpp>

#include <quickjs/quickjs.h>
#include <nanodi.hpp>

#include <unordered_set>
#include <optional>
#include <string>
#include <vector>

namespace notojs {

class Folder;
class Module;
class Server;
class SocketBase;

class Global : public Requires<Folder, Module, Server>
{
    void configure(detail::Config const &);
    friend class Config;

public:
    struct Context
    {
        BOOST_FORCEINLINE Context(JSValue output)
        : output{output} {}

        struct Rejection
        {
            JSValue promise;
            JSValue reason;
        };

        JSValue output;
        std::optional<JSValue> perror;
        std::vector<Rejection> rejections;
        std::unordered_set<std::string> cleanup;
        std::unordered_set<std::string> renderers;

        void wait(JSContext *);
        void free(JSContext *);
        JSValue load(JSContext *, char const *name);

        BOOST_FORCEINLINE static Context *ptr(JSContext *ctx)
        {
            return reinterpret_cast<Context*>(JS_GetContextOpaque(ctx));
        }
    };

    Global();
    void init(JSRuntime *) const;

    BOOST_FORCEINLINE static Global *ptr(JSContext *ctx)
    {
        return reinterpret_cast<Global*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    }

    std::unique_ptr<Context> make(JSContext *, JSValue) const;

    static void get_context(JSContext *ctx, JSValue &);
    static void set_context(JSContext *ctx, JSValue);
    static void set_context(JSContext *ctx, SocketBase &socket);
    static void set_context(JSContext *ctx, std::string const &, JSValue);
};

} // namespace notojs
