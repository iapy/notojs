#include <boost/algorithm/string.hpp>
#include <plugin.hxx>
#include <bridge.hpp>

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace NOTOJS_UNIQUE_NAMESPACE {
namespace {

using Clock = std::chrono::system_clock;
using Minute = std::chrono::minutes::rep;

struct Schedule
{
    enum class Type
    {
        fixedtime,
        recurring
    };

    BOOST_FORCEINLINE bool due(Minute minute) const
    {
        return Type::fixedtime == type ? value == minute : minute % value == 0;
    }

    Type type;
    Minute value;
};

struct {
    std::mutex mutex;
    std::condition_variable wakeup;
    std::condition_variable pending;

    std::unordered_map<std::string, Schedule> notebooks;
    std::unordered_set<std::string> active;

    std::deque<std::pair<std::string, Minute>> queue;
    std::thread scheduler;
    std::thread executor;

    std::function<void(std::string const &)> execute;
    bool stopping{false};
    Minute minute{0};
} state;

std::optional<Minute> fixedtime_schedule(std::string const &name, std::string const &prefix)
{
    if(!boost::algorithm::starts_with(name, prefix))
        return std::nullopt;

    auto const offset = prefix.size();
    if(name.size() - offset != 5 ||
       name[offset] < '0' || name[offset] > '9' ||
       name[offset + 1] < '0' || name[offset + 1] > '9' ||
       name[offset + 2] != ':' ||
       name[offset + 3] < '0' || name[offset + 3] > '9' ||
       name[offset + 4] < '0' || name[offset + 4] > '9')
        return std::nullopt;

    auto const hour = (name[offset] - '0') * 10 + name[offset + 1] - '0';
    auto const minute = (name[offset + 3] - '0') * 10 + name[offset + 4] - '0';
    if(hour >= 24 || minute >= 60)
        return std::nullopt;
    return hour * 60 + minute;
}

std::optional<Minute> recurring_schedule(std::string const &name, std::string const &prefix)
{
    if(!boost::algorithm::starts_with(name, prefix))
        return std::nullopt;

    auto const offset = prefix.size();
    if(name.size() - offset < 2 || name.back() != 'm')
        return std::nullopt;

    Minute interval = 0;
    for(auto i = offset; i + 1 < name.size(); ++i)
    {
        if(name[i] < '0' || name[i] > '9')
            return std::nullopt;
        interval = interval * 10 + name[i] - '0';
        if(interval > 1440)
            return std::nullopt;
    }
    if(interval == 0 || 1440 % interval != 0)
        return std::nullopt;
    return interval;
}

void schedule()
{
    using Minutes = std::chrono::minutes;

    std::unique_lock<std::mutex> lock(state.mutex);
    while(!state.stopping)
    {
        auto const next = std::chrono::time_point_cast<Minutes>(Clock::now()) + Minutes{1};
        if(state.wakeup.wait_until(lock, next, []{ return state.stopping; }))
            break;

        auto const now = Clock::to_time_t(Clock::now());
        std::tm local{};
        ::localtime_r(&now, &local);
        auto const minute = static_cast<Minute>(local.tm_hour * 60 + local.tm_min);

        bool notify = false;
        for(auto const &[name, schedule] : state.notebooks)
        {
            if(schedule.due(minute) && state.active.insert(name).second)
            {
                state.queue.push_back({name, minute});
                notify = true;
            }
        }
        if(notify)
            state.pending.notify_one();
    }
}

void execute()
{
    std::unique_lock<std::mutex> lock(state.mutex);
    while(true)
    {
        state.pending.wait(lock, []{ return state.stopping || !state.queue.empty(); });
        if(state.stopping && state.queue.empty())
            break;

        auto [name, minute] = std::move(state.queue.front());
        state.queue.pop_front();
        if(state.notebooks.find(name) == std::end(state.notebooks))
        {
            state.active.erase(name);
            continue;
        }
        lock.unlock();

        state.minute = minute;
        state.execute(name);

        lock.lock();
        state.active.erase(name);
    }
}

} // namespace

Plugin::Plugin(boost::property_tree::ptree const &pt)
{
    prefix = pt.get_optional<std::string>("prefix");
}

void Plugin::set_context(JSContext *ctx, JSValue &value)
{
    bridge::Object obj{ctx};
    obj.set("hour", bridge::Number{ctx, static_cast<std::int32_t>(state.minute / 60)});
    obj.set("minute", bridge::Number{ctx, static_cast<std::int32_t>(state.minute % 60)});
    value = obj;
}

bool Plugin::run()
{
    if(!prefix)
        return false;

    state.execute = [this](std::string const &name) {
        try {
            std::ignore = host->exec(name, *this);
        } catch(std::exception const &e) {
            host->clog("Execution failed", {name, std::string(e.what())});
        } catch(...) {
            host->clog("Execution failed", {name});
        }
    };

    for(auto const &[name, _] : state.notebooks)
        host->clog("Scheduled", {name, true});

    try {
        state.executor = std::thread(&execute);
        state.scheduler = std::thread(&schedule);
    } catch(...) {
        end();
        throw;
    }
    return true;
}

void Plugin::removed(std::string const &name)
{
    std::unique_lock<std::mutex> lock(state.mutex);
    bool const erased = 0 != state.notebooks.erase(name);
    lock.unlock();

    if(erased) host->clog("Scheduled", {name, false});
}

bool Plugin::updated(std::string const &name)
{
    if(!prefix)
        return false;

    std::optional<Schedule> parsed;
    if(auto minute = fixedtime_schedule(name, *prefix))
        parsed = Schedule{Schedule::Type::fixedtime, *minute};
    else if(auto interval = recurring_schedule(name, *prefix))
        parsed = Schedule{Schedule::Type::recurring, *interval};
    else
        return false;

    std::unique_lock<std::mutex> lock(state.mutex);
    state.notebooks.insert_or_assign(name, *parsed);
    lock.unlock();

    host->clog("Scheduled", {name, true});
    return true;
}

void Plugin::end()
{
    std::unique_lock<std::mutex> lock(state.mutex);
    if(state.stopping) return;
    state.stopping = true;
    lock.unlock();

    state.wakeup.notify_all();
    state.pending.notify_all();

    if(state.scheduler.joinable())
        state.scheduler.join();
    if(state.executor.joinable())
        state.executor.join();

    lock.lock();
    state.execute = {};
    state.notebooks.clear();
    state.active.clear();
    state.queue.clear();
}

} // namespace NOTOJS_UNIQUE_NAMESPACE
