#include <notojs/cacher.hpp>
#include <notojs/config.hpp>
#include <notojs/global.hpp>
#include <notojs/engine.hpp>
#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/router.hpp>
#include <notojs/server.hpp>
#include <notojs/timers.hpp>
#include <notojs/window.hpp>

int main(int argc, char **argv)
{
    if(notojs::Config::kernel(argc, argv))
    {
        return Container<
            notojs::Config,
            notojs::Engine,
            notojs::Folder,
            notojs::Global,
            notojs::Module,
            notojs::Plugin,
            notojs::Server
        >{}.main<notojs::Config>(argc, argv);
    }
    else
    {
        return Container<
            notojs::Cacher,
            notojs::Config,
            notojs::Engine,
            notojs::Errors,
            notojs::Folder,
            notojs::Global,
            notojs::Module,
            notojs::Plugin,
            notojs::Router,
            notojs::Server,
            notojs::Timers,
            notojs::Window
        >{}.main<notojs::Config>(argc, argv);
    }
}
