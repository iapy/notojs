#pragma once
#include <boost/property_tree/ptree.hpp>
#include <nanodi.hpp>

#include <boost/url.hpp>
#include <unordered_map>
#include <filesystem>
#include <string>

struct JSContext;
struct JSRuntime;
struct JSModuleDef;
struct JSValue;

namespace notojs {

class Config;
class Global;
class Plugin;
class Server;

class Module : public Requires<Global, Server>
{
public:
    static constexpr std::integral_constant<int, 0> MODULE = {};
    static constexpr std::integral_constant<int, 1> SCRIPT = {};

    Module();
    void init(JSRuntime *) const;

    JSModuleDef *load(JSContext *ctx, const char *name);
    JSModuleDef *load(JSContext *ctx, const char *name, std::filesystem::path const &);

    JSValue load(JSContext *ctx, decltype(SCRIPT), boost::urls::url &&, const char *name, bool cache = false);
    JSModuleDef *load(JSContext *ctx, decltype(MODULE), boost::urls::url &&, const char *name, bool cache = false);

private:
    struct Impl
    {
        void (*init0)();
        void (*init1)(JSRuntime *);
        void (*init2)(boost::property_tree::ptree const &);
        JSModuleDef *(*init3)(JSContext *, const char *);
    };
    std::unordered_map<std::string, Impl> const modules;
    std::unordered_map<std::string, decltype(Impl{}.init3)> plugins;

private:
    void configure(boost::property_tree::ptree const &);
    friend class Config;
    friend class Plugin;
};

} // namespace notojs
