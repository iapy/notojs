#include <notojs/timers.hpp>
#include <notojs/cacher.hpp>
#include <notojs/engine.hpp>
#include <notojs/errors.hpp>
#include <notojs/logger.hpp>
#include <notojs/folder.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>

#include <notojs/detail/cellid.hpp>
#include <memory.hpp>

#include <iostream>
#include <optional>
#include <sstream>

namespace notojs {

void Timers::end()
{
    if(!stop.load())
    {
        stop.store(true);
        cv.notify_all();
        thread.join();
    }
}

void Timers::removed(std::string const &name)
{
    std::unique_lock<std::mutex> lock(mutex);
    if(auto it = versions.find(name); it != std::end(versions))
        versions.erase(it->first);
}

void Timers::updated(std::string const &name)
{
    std::unique_lock<std::mutex> lock(mutex);
    if(auto it = handlers.find(name); it != std::end(handlers))
        versions[it->first] = get<Folder>().load<detail::Version>(name);
}

void Timers::run()
{
    using clock_t = std::chrono::system_clock;
    if(!schedule.empty())
    {
        stop.store(false);
        thread = std::thread([this]{
            Cacher::Cached cached;
            while(!stop.load(std::memory_order_relaxed))
            {
                auto now = clock_t::now();
                auto tnow = clock_t::to_time_t(now);
                decltype(now) next;

                std::tm tm = *std::localtime(&tnow);
                tm.tm_sec = 0;
                do
                {
                    auto it = schedule.lower_bound(tm.tm_min);
                    if(it == std::end(schedule))
                    {
                        tm.tm_hour += 1;
                        tm.tm_min = 0;
                    }
                    else
                    {
                        tm.tm_min = *it;
                    }
                    next = clock_t::from_time_t(std::mktime(&tm));
                } while(++tm.tm_min, next <= now);

                tnow = clock_t::to_time_t(next);
                tm = *std::localtime(&tnow);

                std::unique_lock<std::mutex> lock(mutex);
                if(cv.wait_until(lock, next, [this] { return stop.load(); }))
                    break;

                for(auto const &[name, schedule]: handlers)
                {
                    if(!schedule[tm.tm_min]) continue;

                    std::optional<detail::Version> version;
                    if(auto it = versions.find(name); it != std::end(versions))
                        version = it->second;

                    if(!version)
                    {
                        cached.erase(name);
                        continue;
                    }
                    lock.unlock();
                    if(auto [it, id, ts] = get<Cacher>().cache(cached, *version, name); it != std::end(cached))
                    {
                        NOTOJS_LOG("Timer", (name));
                        bridge::Context ctx{get<Engine>().get_context()};

                        JSValue glob = JS_GetGlobalObject(ctx.get());
                        JSValue input = JS_NewObject(ctx.get());
                        JS_SetPropertyStr(ctx.get(), input, "hour", JS_NewInt32(ctx.get(), tm.tm_hour));
                        JS_SetPropertyStr(ctx.get(), input, "minute", JS_NewInt32(ctx.get(), tm.tm_min));
                        JS_SetPropertyStr(ctx.get(), glob, "input", input);
                        JS_FreeValue(ctx.get(), glob);

                        Silent::Output error;
                        for(std::size_t i = 0; i < it->second.code->size(); ++i)
                        {
                            Silent w{error};
                            if(get<Engine>().eval(it->second.code->at(i), w, ctx.get(), detail::cell_id(i)); error)
                            {
                                get<Errors>().error(name, std::move(error->json), std::move(error->stack));
                                break;
                            }
                        }
                    }
                    lock.lock();
                }
            }
        });
        boost::asio::post(*get<Server>().disk, [s = schedule.size(), h = handlers.size(), i = thread.get_id()] {
            std::clog << "timers" << notojs::values(s, h, i);
        });
    }
}

void Timers::configure(boost::property_tree::ptree const &pt)
{
    if(auto config = pt.get_child_optional("timers"))
    {
        int n;
        std::string m;
        for(auto const &[k, v]: *config)
        {
            std::istringstream(k) >> n >> m;
            if(m != "m") throw std::invalid_argument("Expected Nm format for timer");
            if(!n || (60 % n)) throw std::invalid_argument("Interval should be divisor of 60");

            std::string nb = v.get_value<std::string>();
            if(nb.find('/') != std::string::npos) throw std::invalid_argument("Notebook name must not contain dir separators");
            if(std::filesystem::path(nb).extension() != ".notojs") throw std::invalid_argument("Notebook name must have .notojs extension");

            for(int i = 0; i < 60; i += n) {
                handlers[nb].set(i);
                schedule.insert(i);
            }

            if(!versions.count(nb)) {
                if(auto version = get<Folder>().load<detail::Version>(nb))
                    versions.emplace(std::make_pair(nb, std::move(*version)));
            }
        }
    }
}

} // namespace notojs
