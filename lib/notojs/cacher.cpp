#include "notojs/detail/jscode.hpp"
#include <notojs/cacher.hpp>
#include <notojs/engine.hpp>
#include <notojs/errors.hpp>
#include <notojs/folder.hpp>
#include <notojs/logger.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>

#include <notojs/detail/cellid.hpp>
#include <memory.hpp>

namespace notojs {

Cacher::Result Cacher::cache(Cached &cached, detail::Version const &version, std::string const &name)
{
    auto it = cached.find(name);
    auto const id = std::this_thread::get_id();
    auto const ts = std::chrono::system_clock::now();
    if(it == std::end(cached) || it->second.version < version)
    {
        std::promise<std::optional<detail::Textcode>> code;
        auto fut = code.get_future();

        boost::asio::post(*get<Server>().disk, [&, this]{
            code.set_value(get<Folder>().load<detail::Textcode>(name));
        });

        if(auto code = fut.get(); code)
        {
            bridge::Context ctx{get<Engine>().get_context()};
            detail::Notebook bytecode;
            bytecode.reserve(code->size());

            Silent::Output error;
            for(std::size_t i = 0; i < code->size(); ++i)
            {
                if(detail::disabled(code->at(i))) continue;

                Silent w{error};
                Engine::preprocess(code->at(i));
                if(get<Engine>().eval(code->at(i), bytecode.emplace_back(), w, ctx.get(), detail::cell_id(i)); error)
                {
                    if(has<Errors>()) get<Errors>().error(name, std::move(error->json), std::move(error->stack));
                    else NOTOJS_LOG("Not compiled", (name)(notojs::values.raw(std::move(error->json))));
                    break;
                }
            }

            if(!error)
            {
                NOTOJS_LOG("Compiled", (name)(code->size())(bytecode.size())(std::chrono::system_clock::now() - ts)(id));
                it = cached.emplace(std::make_pair(name, detail::Cached{})).first;
                it->second.code = std::move(bytecode);
                it->second.version = version;
            }
        }
    }
    return std::make_tuple(it, id, ts);
}

} // namespace notojs
