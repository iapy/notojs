#pragma once
#include <nanodi.hpp>
#include <filesystem>

#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/process/v2/stdio.hpp>
#include <boost/process/v2/process.hpp>

namespace notojs {
namespace bp = boost::process::v2;

class Engine;
class Folder;
class Global;
class Module;
class Plugin;
class Server;
class Timers;

class Config : public Requires<Engine, Global, Folder, Module, Plugin, Server, Timers>
{
public:
    int main(int, char **);
    static bool kernel(int, char**);

    struct Kernel
    {
        template<typename Context>
        BOOST_FORCEINLINE Kernel(Context &ctx, Config const &cfg)
        : out{ctx}
        , err{ctx}
        , cin{ctx}
        , process{
            ctx,
            cfg.binary.string(),
            cfg.stdarg,
            bp::process_stdio{cin, out, err}
        } {}

        Kernel(Kernel &&) = default;
        Kernel(Kernel const &) = delete;

        boost::asio::readable_pipe out;
        boost::asio::readable_pipe err;
        boost::asio::writable_pipe cin;

        BOOST_FORCEINLINE void wait() { process.wait(); }
        BOOST_FORCEINLINE void kill() { process.terminate(); }
        BOOST_FORCEINLINE bool live()
        {
            try {
                return process.running();
            } catch(...) {
                return false;
            }
        }

        BOOST_FORCEINLINE pid_t pid() const { return process.id(); }
        BOOST_FORCEINLINE int  code() const { return process.exit_code(); }
    private:
        bp::process process;
    };

    template<typename Context>
    BOOST_FORCEINLINE Kernel fork(Context &ctx) const
    {
        return Kernel{ctx, *this};
    }

private:
    static bool kernel_;
    std::filesystem::path binary;
    std::filesystem::path config;
    std::vector<std::string> stdarg;
};

} // namespace notojs
