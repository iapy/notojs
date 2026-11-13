#include <boost/test/unit_test.hpp>
#include <notojs/detail/config.hpp>
#include <notojs/module/assert.hpp>
#include <notojs/module/doc.hpp>
#include <notojs/module/fs.hpp>
#include <notojs/cacher.hpp>
#include <notojs/config.hpp>
#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/engine.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/server.hpp>
#include "test_engine.hpp"

namespace notojs::testing {

notojs::Engine *engine{nullptr};
notojs::Folder *folder{nullptr};
notojs::Global *global{nullptr};
notojs::Server *server{nullptr};

} // namespace notojs::testing

class Main : public Requires<notojs::Cacher, notojs::Engine, notojs::Errors, notojs::Folder, notojs::Global, notojs::Module, notojs::Plugin, notojs::Server>
{
public:
    int main(int argc, char **argv);
    static bool init_test();
};

Container
<
    notojs::Cacher,
    notojs::Engine,
    notojs::Errors,
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
    notojs::testing::engine = &get<notojs::Engine>();
    notojs::testing::folder = &get<notojs::Folder>();
    notojs::testing::global = &get<notojs::Global>();
    notojs::testing::server = &get<notojs::Server>();

    auto const p = std::filesystem::path{CMAKE_CURRENT_BINARY_DIR} / "fs";
    if(std::filesystem::exists(p)) std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);

    auto const d = std::filesystem::path{CMAKE_CURRENT_BINARY_DIR} / ".db";
    if(std::filesystem::exists(d)) std::filesystem::remove_all(d);

    notojs::detail::Config cfg{};
    cfg.put("engine.sopath", CMAKE_CURRENT_BINARY_DIR);
    cfg.put("engine.jspath", CMAKE_CURRENT_SOURCE_DIR);
    cfg.put("plugin.path", CMAKE_CURRENT_BINARY_DIR);
    cfg.put("plugin:plugin.enabled", true);
    cfg.put("folder.path", CMAKE_CURRENT_BINARY_DIR);
    cfg.put("global.agent", "curl/8.7.1");
    cfg.put("global.prefix", "http://127.0.0.1:" + std::to_string(TEST_SERVER_PORT));
    cfg.put("server.bind", "127.0.0.1:" + std::to_string(TEST_SERVER_PORT));

    cfg.put("module:fs./rw", p.u8string() + ",rw");
    cfg.put("module:fs./ro", p.u8string() + ",ro");
    cfg.put("module:doc.suite", NOTOJS_DOC_SUITE);

    notojs::Config::configure(
        cfg,
        *notojs::testing::engine,
        *notojs::testing::folder,
        *notojs::testing::global,
        get<notojs::Module>(),
        get<notojs::Plugin>());

    notojs::testing::server->sync = std::make_unique<boost::asio::thread_pool>(1);
    notojs::testing::server->disk = notojs::testing::server->sync.get();

    boost::unit_test::framework::register_observer(observer);
    boost::unit_test::framework::master_test_suite().p_name.value = "NotoJS";
    return boost::unit_test::unit_test_main(&Main::init_test, argc, argv);
}

bool Main::init_test()
{
    return true;
}

int main(int argc, char **argv)
{
    return container.main<Main>(argc, argv);
}
