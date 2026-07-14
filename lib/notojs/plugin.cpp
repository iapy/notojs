#include <notojs/cacher.hpp>
#include <notojs/engine.hpp>
#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/logger.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>

#include <notojs/detail/cellid.hpp>
#include <notojs/detail/jscode.hpp>

#include <unordered_map>
#include <shared_mutex>
#include <filesystem>
#include <memory.hpp>
#include <iostream>
#include <dlfcn.h>

namespace notojs {
namespace {

struct Error : IPlugin
{
    std::string const name;
    std::string const message;
    std::optional<std::string> const data;

    Error(std::string const &name, std::string const &message, std::optional<std::string> const &data = std::nullopt)
    : name{name}
    , message{message}
    , data{data}
    {}

    bool run(IHost &) override
    {
        if(data)
            std::clog << name << notojs::values(message, *data);
        else
            std::clog << name << notojs::values(message);
        return false;
    }

    void end(IHost &) override {}
};

std::shared_mutex mutex;
thread_local Cacher::Cached cached;
std::unordered_map<std::string, std::optional<detail::Version>> versions;

} // namespace

void Plugin::configure(boost::property_tree::ptree const &pt)
{
    auto path = pt.get_optional<std::string>("plugin.path");
    if(!path) return;

    constexpr std::string_view PLUGIN{"plugin:"};
    auto const sopath = std::filesystem::path{std::move(*path)};

    for(auto const& [sec, subtree] : pt)
    {
        if(sec.size() > PLUGIN.size() && !sec.compare(0, PLUGIN.size(), PLUGIN))
        {
            auto const name = sec.substr(PLUGIN.size());
            auto const path = (sopath / name).string() + ".so";
            if(void *handle = ::dlopen(path.c_str(), RTLD_NOW); handle)
            {
                if(IPlugin::Loader init = (IPlugin::Loader)::dlsym(handle, "notojs_plugin"); init)
                {
                    plugins.emplace(std::make_pair(name, init(subtree)));
                }
                else
                {
                    plugins.emplace(std::make_pair(name, new Error{name, "No init function"}));
                }
            }
            else
            {
                plugins.emplace(std::make_pair(name, new Error{name, "Library not loaded", dlerror()}));
            }
        }
    }
}

void Plugin::run()
{
    for(auto it = std::begin(plugins); it != std::end(plugins);)
    {
        if(!it->second->run(*this))
        {
            if(auto jt = get<Module>().plugins.find(it->first); jt != std::end(get<Module>().plugins))
                get<Module>().plugins.erase(jt);
            NOTOJS_LOG("Plugin failed", (it->first));
            it = plugins.erase(it);
        }
        else
        {
            NOTOJS_LOG("Plugin started", (it->first));
            ++it;
        }
    }
}

void Plugin::mod()
{
    for(auto it = std::begin(plugins); it != std::end(plugins); ++it)
    {
        if(auto def = it->second->mod(*this))
        {
            get<Module>().plugins.emplace(it->first, def);
            NOTOJS_LOG("Registered module", (it->first));
        }
    }
}

void Plugin::removed(std::string const &name)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    if(auto it = versions.find(name); it != std::end(versions))
        it->second = std::nullopt;
}

void Plugin::updated(std::string const &name)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    if(auto it = versions.find(name); it != std::end(versions))
        it->second = get<Folder>().load<detail::Version>(name);
}

void Plugin::end()
{
    for(auto &[name, plugin]: plugins) {
        std::clog << "Plugin stopped" << notojs::values(name);
        plugin->end(*this);
    }
}

void Plugin::load(std::string const &name)
{
    std::unique_lock<std::shared_mutex> lock{mutex};
    if(auto it = versions.find(name); it == std::end(versions))
        versions.emplace(std::make_pair(name, get<Folder>().load<detail::Version>(name)));
}

bool Plugin::exec(std::string const &name, IContext &context)
{
    std::optional<detail::Version> version;
    {
        std::shared_lock<std::shared_mutex> lock{mutex};
        if(auto it = versions.find(name); it != std::end(versions))
            version = it->second;
    }

    if(!version)
    {
        cached.erase(name);
        return false;
    }
    if(auto [it, id, ts] = get<Cacher>().cache(cached, *version, name); it != std::end(cached))
    {
        bridge::Context ctx{get<Engine>().get_context()};

        JSValue glob = JS_GetGlobalObject(ctx.get());
        JSValue input = JS_UNDEFINED;
        context.input(ctx.get(), input);
        if(!JS_IsUndefined(input))
            JS_SetPropertyStr(ctx.get(), glob, "input", input);
        JS_FreeValue(ctx.get(), glob);

        Silent::Output error;
        for(std::size_t i = 0; i < it->second.code->size(); ++i)
        {
            Silent w{error};
            if(get<Engine>().eval(it->second.code->at(i), w, ctx.get(), detail::cell_id(i)); error)
            {
                get<Errors>().error(name, std::move(error->json), std::move(error->stack));
                return false;
            }
        }

        context.output(ctx.get());
        return true;
    }
    return false;
}

void Plugin::clog(std::string &&line)
{
    boost::asio::post(*get<Server>().disk, [line=std::move(line)]{
        (std::clog << line << '\n').flush();
    });
}

MDB_env *Plugin::lmdb()
{
    return get<Folder>().env();
}

void IContext::input(JSContext *, JSValue &) {}
void IContext::output(JSContext *) {}

IPlugin::Module IPlugin::mod(notojs::IHost &)
{
    return NULL;
}

} // namespace notojs
