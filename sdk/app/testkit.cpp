#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <boost/test/unit_test.hpp>
#include <memory.hpp>
#include <notojs/config.hpp>
#include <notojs/detail/config.hpp>
#include <notojs/engine.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>

namespace notojs {
extern std::atomic<std::uint64_t> assertions;
} // namespace notojs

class Main : public Requires<notojs::Engine, notojs::Folder, notojs::Global, notojs::Module, notojs::Plugin, notojs::Server>
{
public:
    int main(int argc, char **argv);
    static bool init_test();
};

Container
<
    notojs::Engine,
    notojs::Folder,
    notojs::Global,
    notojs::Module,
    notojs::Plugin,
    notojs::Server,
    Main
> container;

struct Observer : boost::unit_test::test_observer
{
    void test_finish() override
    {
        std::cerr << "JavaScript assertions " << notojs::assertions << '\n';
    }
} observer;

int Main::main(int argc, char **argv)
{
    constexpr auto error = [](char const *message) {
        std::cerr << message << '\n';
        return 1;
    };

    enum {
        KIND_MODULE,
        KIND_PLUGIN,
        KIND_SCRIPT,
    } kind{KIND_SCRIPT};

    notojs::detail::Config cfg{};
    std::filesystem::path library;
    std::filesystem::path testdir;

    int i = 1;
    for(; i < argc; ++i)
    {
        if(std::string_view const arg{argv[i]}; arg.substr(0, 2) == "-D")
        {
            auto const equal = arg.find('=', 2);
            if(equal == std::string_view::npos || equal == 2)
                return error("Expecting -Dkey=value");
            cfg.put(std::string{arg.substr(2, equal - 2)}, std::string{arg.substr(equal + 1)});
        }
        else if(arg == "--help") break;
        else if(library.empty())
        {
            library = std::filesystem::absolute(argv[i]);
            if(!std::filesystem::is_regular_file(library))
                return error("Expecting library");
        }
        else if(testdir.empty())
        {
            testdir = std::filesystem::absolute(argv[i]);
            if(!std::filesystem::is_directory(testdir))
                return error("Expecting directory");
        }
        else break;
        std::swap(argv[i], argv[i - 1]);
    }

    if(library.empty() || testdir.empty() || (i < argc && !strcmp(argv[i], "--help")))
    {
        std::cerr << argv[i - 1]
                  << " <library> <testdir> [-Dkey=value ...] [Boost.Test arguments ...]\n";
        if(i >= argc || strcmp(argv[i], "--help")) return 1;
        return boost::unit_test::unit_test_main(&Main::init_test, argc - i + 1, argv + i - 1);
    }

    if(".js" != library.extension())
    {
        void *handle = ::dlopen(library.c_str(), RTLD_NOW);
        if(!handle) {
            const char *error = ::dlerror();
            throw std::runtime_error("Cannot load library " + library.string() + ": "
                + (error ? error : "unknown dynamic loader error"));
        }
        kind = ::dlsym(handle, "notojs_plugin") != nullptr ? KIND_PLUGIN : KIND_MODULE;
        ::dlclose(handle);
    }

    switch(kind)
    {
    case KIND_PLUGIN:
        cfg.put("engine.jspath", testdir.string());
        cfg.put("plugin.path", library.parent_path().string());
        break;
    case KIND_MODULE:
        cfg.put("engine.jspath", testdir.string());
        cfg.put("engine.sopath", library.parent_path().string());
        break;
    case KIND_SCRIPT:
        cfg.put("engine.jspath", library.parent_path().string());
        break;
    }

    if(!cfg.count("folder.path")) cfg.put("folder.path", library.parent_path().string());
    if(!cfg.count("server.bind")) cfg.put("server.bind", "127.0.0.1:0");

    auto &server = get<notojs::Server>();
    server.sync = std::make_unique<boost::asio::thread_pool>(1);
    server.disk = server.sync.get();

    notojs::Config::configure(
        cfg,
        get<notojs::Engine>(),
        get<notojs::Folder>(),
        get<notojs::Global>(),
        get<notojs::Module>(),
        get<notojs::Plugin>());

    if(kind == KIND_PLUGIN) get<notojs::Plugin>().mod();

    auto* suite = BOOST_TEST_SUITE("JavaScript");
    for(auto p : std::filesystem::directory_iterator(testdir))
    {
        if(auto const stem = p.path().stem().string();
            p.is_regular_file() && stem.substr(0, 5) == "test_" && ".js" == p.path().extension())
        {
            suite->add(boost::unit_test::make_test_case(
                [engine=&get<notojs::Engine>(), p] {
                    std::ifstream stream{p.path(), std::ios::binary};
                    BOOST_REQUIRE_MESSAGE(stream.is_open(), "Cannot open test file: " << p.path());

                    std::string code{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
                    BOOST_REQUIRE_MESSAGE(!stream.bad(), "Cannot read test file: " << p.path());

                    notojs::Silent::Output error;
                    notojs::Silent writer{error};
                    bridge::Context context{engine->get_context()};

                    auto status = engine->eval(code, writer, context.get(), p.path().filename().string());

                    BOOST_REQUIRE_MESSAGE(!error, p.path() << ": " << (error ? error->json : "")
                        << (error && error->stack ? "\n" + *error->stack : ""));

                    BOOST_REQUIRE_MESSAGE(status == boost::beast::http::status::ok,
                        "JavaScript evaluation failed in " << p.path() << ": " << status);
                },
                stem,
                __FILE__,
                __LINE__
            ));
        }
    }

    boost::unit_test::framework::register_observer(observer);
    boost::unit_test::framework::master_test_suite().add(suite);
    return boost::unit_test::unit_test_main(&Main::init_test, argc - i + 1, argv + i - 1);
}

bool Main::init_test()
{
    return true;
}

int main(int argc, char **argv)
{
    try {
        return container.main<Main>(argc, argv);
    }
    catch(std::exception const &e) {
        std::cerr << "testkit: " << e.what() << '\n';
        return 1;
    }
    catch(...) {
        std::cerr << "testkit: Unknown exception\n";
        return 1;
    }
}
