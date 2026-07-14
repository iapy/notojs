#include <boost/test/unit_test.hpp>
#include <notojs/module/assert.hpp>
#include <notojs/module/fs.hpp>
#include <notojs/config.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/engine.hpp>
#include <notojs/module.hpp>
#include <notojs/server.hpp>
#include "test_engine.hpp"

namespace notojs::testing {

notojs::Engine *engine{nullptr};
notojs::Global *global{nullptr};
notojs::Folder *folder{nullptr};
notojs::Server *server{nullptr};

} // namespace notojs::testing

class Main : public Requires<notojs::Engine, notojs::Folder, notojs::Global, notojs::Server>
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
    notojs::testing::engine->set_sopath(CMAKE_CURRENT_BINARY_DIR);
    notojs::testing::engine->set_jspath(CMAKE_CURRENT_SOURCE_DIR);
    notojs::testing::global->set_agent("curl/8.7.1");
    notojs::testing::global->set_prefix("http://127.0.0.1:" + std::to_string(TEST_SERVER_PORT));
    boost::unit_test::framework::register_observer(observer);
    boost::unit_test::framework::master_test_suite().p_name.value = "NotoJS";
    notojs::testing::server->sync = std::make_unique<boost::asio::thread_pool>(1);
    notojs::testing::server->disk = notojs::testing::server->sync.get();

    auto const p = std::filesystem::path{CMAKE_CURRENT_BINARY_DIR} / "fs";
    if(std::filesystem::exists(p))
        std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    boost::property_tree::ptree tree;
    boost::property_tree::ptree fs;
    fs.put("/rw", p.u8string() + ",rw");
    fs.put("/ro", p.u8string() + ",ro");
    tree.add_child("module:fs", fs);
    notojs::notojs_init_fs(tree);
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

