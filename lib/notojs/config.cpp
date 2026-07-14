#include <notojs/config.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/engine.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/server.hpp>
#include <notojs/timers.hpp>

#include <boost/property_tree/ini_parser.hpp>
#include <iostream>

namespace notojs {

bool Config::kernel_;
bool Config::kernel(int argc, char **argv)
{
    return kernel_ = (argc == 3 && !strcmp(argv[2], "--kernel"));
}

int Config::main(int argc, char **argv)
{
    if(!kernel_ && argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " config [--kernel]\n";
        return EXIT_FAILURE;
    }

    config = std::filesystem::absolute(argv[1]);
    if(!std::filesystem::is_regular_file(config))
    {
        std::cerr << "File doesn't exist " << config << '\n';
        return EXIT_FAILURE;
    }

    binary = std::filesystem::absolute(argv[0]);

    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(config, pt);

    get<Engine>().configure(pt);
    get<Folder>().configure(pt);
    get<Global>().configure(pt);
    get<Module>().configure(pt);
    get<Plugin>().configure(pt);

    if(!kernel_)
    {
        stdarg = {config.string(), "--kernel"};
        get<Server>().configure(pt);
        get<Timers>().configure(pt);
    }
    return get<Server>().main();
}

} // namespace notojs
