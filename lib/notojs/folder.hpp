#pragma once
#include <boost/property_tree/ptree.hpp>

#include <notojs/detail/jscode.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <nanodi.hpp>

#include <unordered_set>
#include <filesystem>
#include <optional>
#include <fstream>

struct MDB_env;

namespace notojs {

class Config;
class Plugin;
class Timers;

class Folder : public Requires<Plugin, Timers>
{
public:
    ~Folder();
    void set_path(std::filesystem::path &&path);

    template<typename R>
    std::optional<R> load(std::string const &path) const;

    std::optional<std::filesystem::path> lib(std::string const &name) const;

    boost::beast::http::status list(std::string &json) const;
    boost::beast::http::status eval(std::string &json, std::string const &path) const;
    boost::beast::http::status eval(std::string &json, std::string const &path, std::unordered_set<std::string> &) const;
    boost::beast::http::status load(std::string &json, std::string const &path) const;
    boost::beast::http::status move(std::string const &path, std::string name);
    boost::beast::http::status save(std::string const &json, std::string const &path);
    boost::beast::http::status remove(std::string const &path);

    boost::beast::http::status get_packages(std::string &target) const;
    boost::beast::http::status set_packages(std::string &source) const;

    class File
    {
    public:
        File(File &&) noexcept = default;
        File &operator = (File &&) noexcept = default;

        File(std::string const &, Folder const &);
        ~File();

        void code(std::string const &, bool);
        void data(std::string const &);

        BOOST_FORCEINLINE void save() const
        {
            discard = false;
        }

        BOOST_FORCEINLINE std::string name() const
        {
            return path.stem().u8string();
        }
    private:
        mutable bool discard{true};
        std::filesystem::path path;
        std::ofstream stream;
    };

    BOOST_FORCEINLINE MDB_env *env() { return env_; }
private:
    void configure(boost::property_tree::ptree const &);
    friend class Config;
    friend class DB;

private:
    void init_db();
    MDB_env *env_{nullptr};
    std::size_t size{10485760};
    std::filesystem::path path;
    static std::string const http;
    static std::string const pkgs;
};

} // namespace notojs
