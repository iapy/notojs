#pragma once
#include <boost/property_tree/ptree.hpp>
#include <boost/beast.hpp>

#include <notojs/detail/jscode.hpp>
#include <nanodi.hpp>
#include <filesystem>
#include <optional>

struct JSContext;
struct JSRuntime;
struct JSModuleDef;
struct JSValue;

namespace notojs {

class Config;
class Folder;
class Global;
class Module;

class Engine : public Requires<Global, Folder, Module>
{
    void configure(boost::property_tree::ptree const &);
    friend class Config;

    boost::optional<std::filesystem::path> jspath;
    boost::optional<std::filesystem::path> sopath;

public:
    template<typename W>
    void eval(std::string const &, detail::Bytecode &, W &,
        std::optional<std::string> &&name = std::nullopt
    ) const;

    template<typename W>
    void eval(std::string const &, detail::Bytecode &, W &, JSContext *,
        std::optional<std::string> &&name = std::nullopt
    ) const;

    template<typename W>
    boost::beast::http::status eval(std::string const &, W &, JSContext *,
        std::optional<std::string> &&name = std::nullopt
    ) const;

    template<typename W>
    boost::beast::http::status eval(detail::Bytecode const &, W &, JSContext *,
        std::optional<std::string> &&name = std::nullopt
    ) const;

    template<typename W>
    boost::beast::http::status eval(std::string const &, W &) const;

    template<typename W>
    boost::beast::http::status eval(detail::Bytecode const &, W &) const;

    void render(
        boost::beast::http::fields const &, std::string const &,
        boost::beast::http::response<boost::beast::http::string_body> &) const;

    void render(
        std::string const &name, std::string &output) const;

    void set_sopath(std::filesystem::path &&path);
    void set_jspath(std::filesystem::path &&path);

    JSContext *get_context() const;
    static void preprocess(std::string &code);

    Engine() = default;
    Engine(Engine const &) = delete;
private:
    JSRuntime *get_runtime() const;
    JSRuntime *make_runtime() const;

    static void tracker(JSContext *, JSValue, JSValue, int, void*);
    static JSModuleDef * loader(JSContext *ctx, const char *, void *);
    using js_module_init_fn = JSModuleDef *(*)(JSContext *, const char *);

    template<typename W>
    boost::beast::http::status eval(JSValue &, W &, JSContext *ctx, JSValue &, std::optional<std::string> &&) const;
};

} // namespace notojs
