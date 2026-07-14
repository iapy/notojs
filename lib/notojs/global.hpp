#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/url.hpp>

#include <quickjs/quickjs.h>
#include <nanodi.hpp>

#include <unordered_set>
#include <optional>
#include <utility>
#include <string>

namespace notojs {

class Config;
class Folder;
class Module;
class Server;
class SocketBase;

class Global : public Requires<Folder, Module, Server>
{
public:
    struct Context
    {
        BOOST_FORCEINLINE Context(JSValue output)
        : output{output} {}

        JSValue output;
        std::optional<JSValue> perror;
        std::unordered_set<std::string> cleanup;
        std::unordered_set<std::string> renderers;

        void wait(JSContext *);
        void free(JSContext *);

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

    void set_agent(std::string &&agent) const;
    void set_prefix(std::string &&prefix) const;

    static void set_handle(JSContext *ctx, SocketBase &socket);
private:
    void configure(boost::property_tree::ptree const &);
    friend class Config;
};

} // namespace notojs
