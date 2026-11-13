#include <notojs/cacher.hpp>
#include <notojs/engine.hpp>
#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/logger.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>

#include <notojs/detail/cellid.hpp>
#include <notojs/detail/jscode.hpp>
#include <boost/hana.hpp>
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

    bool run() override
    {
        if(data)
            std::clog << name << notojs::values(message, *data);
        else
            std::clog << name << notojs::values(message);
        return false;
    }

    void end() override {}
};

struct Notebook
{
    std::unordered_set<IPlugin*> plugins;
    std::optional<detail::Version> version = {};
};

std::shared_mutex mutex;
thread_local Cacher::Cached cached;
std::unordered_map<std::string, Notebook> notebooks;

} // namespace

void Plugin::configure(detail::Config const &cfg)
{
    auto path = cfg.get_optional<std::string>("plugin.path");
    if(!path) return;

    constexpr std::string_view PLUGIN{"plugin:"};
    auto const sopath = std::filesystem::path{std::move(*path)};

    JS_NewClassID(&IPlugin::cid);
    for(auto const& [sec, subtree] : cfg)
    {
        if(sec.size() > PLUGIN.size() && !sec.compare(0, PLUGIN.size(), PLUGIN))
        {
            auto const name = sec.substr(PLUGIN.size());
            auto const path = (sopath / name).string() + ".so";
            std::unique_ptr<IPlugin> plugin;
            if(void *handle = ::dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL))
            {
                std::ignore = ::dlerror();
                if(auto init = reinterpret_cast<IPlugin::Loader>(::dlsym(handle, "notojs_plugin")))
                {
                    plugin.reset(init(subtree));
                    if(!plugin) plugin = std::make_unique<Error>(name, "Init function returned null");
                }
                else
                {
                    plugin = std::make_unique<Error>(name, "No init function");
                }
            }
            else
            {
                char const *error = ::dlerror();
                plugin = std::make_unique<Error>(name, "Library not loaded", error ? error : "Unknown error");
            }
            plugin->host = this;
            plugins.emplace(name, std::move(plugin));
        }
    }

    detail::iterate(get<Folder>().path, [this](auto &&path){
        if(std::filesystem::is_regular_file(path) && ".notojs" == path.extension())
        {
            auto name = path.filename().u8string();
            auto const stem = path.stem().u8string();
            std::unordered_set<IPlugin*> p;

            for(auto &[_, plugin] : plugins)
            {
                if(plugin->updated(stem)) p.insert(plugin.get());
            }

            if(!p.empty())
            {
                auto v = get<Folder>().load<detail::Version>(name);
                notebooks[name].plugins.swap(p);
                notebooks[name].version.swap(v);
            }
        }
    });
}

void Plugin::run()
{
    for(auto it = std::begin(plugins); it != std::end(plugins);)
    {
        if(!it->second->run())
        {
            if(auto jt = get<Module>().plugins.find(it->first); jt != std::end(get<Module>().plugins))
                get<Module>().plugins.erase(jt);
            NOTOJS_LOG("Plugin failed", (it->first));
            for(auto jt = std::begin(notebooks); jt != std::end(notebooks);)
            {
                jt->second.plugins.erase(it->second.get());
                if(jt->second.plugins.empty()) jt = notebooks.erase(jt);
                else ++jt;
            }
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
        if(auto def = it->second->mod())
        {
            get<Module>().plugins.emplace(it->first, def);
            NOTOJS_LOG("Registered module", (it->first));
        }
    }
}

void Plugin::removed(std::filesystem::path const &name)
{
    auto const stem = name.stem();
    std::unordered_set<IPlugin*> p;

    std::unique_lock<std::shared_mutex> lock(mutex);
    if(auto it = notebooks.find(name); it != std::end(notebooks))
    {
        p.swap(it->second.plugins);
        notebooks.erase(it);
    }
    lock.unlock();

    for(auto *plugin: p) plugin->removed(stem);
}

void Plugin::updated(std::filesystem::path const &name)
{
    auto const stem = name.stem();
    auto version = get<Folder>().load<detail::Version>(name);

    std::unique_lock<std::shared_mutex> lock(mutex);
    notebooks[name].version = std::move(version);
    lock.unlock();

    std::unordered_set<IPlugin*> p;
    for(auto &[_, plugin] : plugins)
    {
        if(plugin->updated(stem))
            p.insert(plugin.get());
    }

    lock.lock();
    if(p.empty()) notebooks.erase(name);
    else notebooks[name].plugins.swap(p);
}

void Plugin::end()
{
    for(auto it = std::rbegin(plugins); it != std::rend(plugins); ++it)
    {
        it->second->end();
        NOTOJS_LOG("Plugin stopped", (it->first));
    }
}

bool Plugin::exec(std::string const &stem, IContext &context)
{
    auto const name = stem + ".notojs";
    std::optional<detail::Version> version;
    {
        std::shared_lock<std::shared_mutex> lock{mutex};
        if(auto it = notebooks.find(name); it != std::end(notebooks))
            version = it->second.version;
    }

    if(!version)
    {
        cached.erase(name);
        return false;
    }

    if(auto [it, id, ts] = get<Cacher>().cache(cached, *version, name); it != std::end(cached))
    {
        bridge::Context ctx{get<Engine>().get_context()};

        JSValue value = JS_UNDEFINED;
        context.set_context(ctx.get(), value);
        Global::set_context(ctx.get(), stem, value);

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

        Global::get_context(ctx.get(), value);
        context.get_context(ctx.get(), value);
        JS_FreeValue(ctx.get(), value);
        return true;
    }
    return false;
}

void Plugin::clog(std::string &&line, IHost::Args &&args)
{
    std::ostringstream ss;
    ss << line;
    if(args.size())
    {
        ss << " ";
        detail::Values::Writer writer(ss);
        rapidjson::Writer<decltype(writer)> w(writer);

        w.StartArray();
        for(auto const &arg: args) std::visit([&w](auto const &a){
            detail::Values::write(w, a);
        }, arg);
        w.EndArray();

        w.Flush();
        ss << '\n';
    }

    if(get<Server>().disk) boost::asio::post(*get<Server>().disk, [line=std::move(ss.str())]{
        (std::clog << line).flush();
    });
}

void Plugin::clog(std::string &&line)
{
    if(get<Server>().disk) boost::asio::post(*get<Server>().disk, [line=std::move(line)]{
        (std::clog << line << '\n').flush();
    });
}

MDB_env *Plugin::lmdb()
{
    return get<Folder>().env();
}

void IContext::set_context(JSContext *, JSValue &) {}
void IContext::get_context(JSContext *, JSValue) {}

auto IPlugin::mod() -> Module
{
    return {};
}

void IPlugin::removed(std::string const &) {}
bool IPlugin::updated(std::string const &)
{
    return false;
}

std::uint32_t IPlugin::cid{0};

JSModuleDef *IPlugin::def_module(JSContext *ctx, char const *name, JSInit fn)
{
    static_assert(std::is_same_v<JSInit, JSModuleInitFunc>);
    if(JSRuntime *rt = JS_GetRuntime(ctx); !JS_IsRegisteredClass(rt, cid))
    {
        JSClassDef def{};
        def.class_name = "Plugin";
        if(JS_NewClass(rt, cid, &def) < 0)
        {
            JS_ThrowInternalError(ctx, "Cannot register class Plugin");
            return NULL;
        }
    }

    JSValue data = JS_NewObjectClass(ctx, cid);
    if(JS_IsException(data))
        return NULL;

    JS_SetOpaque(data, this);
    if(JSModuleDef *mod = JS_NewCModule(ctx, name, fn))
    {
        JS_SetModulePrivateValue(ctx, mod, data);
        return mod;
    }
    else
    {
        JS_FreeValue(ctx, data);
        return NULL;
    }
}

IPlugin *IPlugin::get_plugin(JSValue v)
{
    return static_cast<IPlugin *>(JS_GetOpaque(v, cid));
}

std::pair<JSValue, IPlugin *> IPlugin::get_plugin(JSContext *ctx, JSModuleDef *m)
{
    JSValue v = JS_GetModulePrivateValue(ctx, m);
    return {v, get_plugin(v)};
}

} // namespace notojs
