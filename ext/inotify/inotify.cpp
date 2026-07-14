#include "quickjs/quickjs.h"
#include <bridge.hpp>
#include <global.hpp>
#include <module.hpp>
#include <plugin.hpp>

#include <boost/asio/thread_pool.hpp>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>

#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#elif defined(__linux__)
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <unordered_map>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace {

struct Event : public notojs::IContext
{
    std::string path;

    Event(std::string &&path)
    : path(std::move(path)) {}

    void input(JSContext *ctx, JSValue &inp) override
    {
        inp = bridge::String{ctx, path};
    }
};

class Plugin : public notojs::IPlugin
{
public:
    Plugin(boost::property_tree::ptree const &cfg)
    {
        if(auto path = cfg.get_optional<std::string>("path"))
            try {
                this->path = noto::fs::Path::v2n(*path);
            } catch(std::runtime_error const &e) {
                this->path.reset();
            }

        if(auto book = cfg.get_optional<std::string>("book"))
            this->book = *book;
    }

    bool run(notojs::IHost &host) override
    {
        if(!path)
        {
            host.clog("Path is not set");
            return false;
        }
        if(!book)
        {
            host.clog("Book is not set");
            return false;
        }
        host.load(*book);
        Plugin::self = this;
        Plugin::host = &host;
#if defined(__APPLE__)
        std::unique_ptr<__CFString const, Deleter> p{CFStringCreateWithCString(nullptr, this->path->c_str(), kCFStringEncodingUTF8)};
        if(!p)
        {
            host.clog("Failed to create CFStringRef for path");
            return false;
        }

        void const *values[] = {p.get()};
        std::unique_ptr<__CFArray const, Deleter> w{CFArrayCreate(nullptr, values, 1, &kCFTypeArrayCallBacks)};
        if(!w)
        {
            host.clog("Failed to create CFArrayRef for watch");
            return false;
        }
        stream.reset(FSEventStreamCreate(nullptr, [](ConstFSEventStreamRef, void* context, size_t count, void* paths,
            FSEventStreamEventFlags const flags[], FSEventStreamEventId const[])
        {
            if(!Plugin::self->thread) Plugin::self->thread.emplace(1);

            auto const changed = static_cast<char const **>(paths);
            for(std::size_t i = 0; i < count; ++i)
            {
                if(std::filesystem::exists(changed[i]) && std::filesystem::is_regular_file(changed[i]))
                {
                    if((flags[i] & kFSEventStreamEventFlagItemCreated) || (flags[i] & kFSEventStreamEventFlagItemModified)) {
                        Event e{noto::fs::Path::n2v(changed[i]).string()};
                        boost::asio::post(*Plugin::self->thread, [e=std::move(e)]() mutable {
                            Plugin::host->exec(*Plugin::self->book, e);
                        });
                    }
                }
            }
        }, nullptr, w.get(), kFSEventStreamEventIdSinceNow, 0, kFSEventStreamCreateFlagFileEvents));

        if(!stream)
        {
            host.clog("Failed to create FSEventStreamRef for watch");
            return false;
        }
        if(!(queue = dispatch_queue_create("notojs.inotify", nullptr)))
        {
            host.clog("Failed to create FSEvents dispatch queue");
            stream.reset();
            return false;
        }
        if(FSEventStreamSetDispatchQueue(stream.get(), queue); !FSEventStreamStart(stream.get()))
        {
            host.clog("Failed to start FSEventStream");
            dispatch_release(std::exchange(queue, nullptr));
            stream.reset();
            return false;
        }
#elif defined(__linux__)
        if((fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK)) < 0)
        {
            host.clog("Failed to initialize inotify: " + std::string(std::strerror(errno)));
            return false;
        }
        if(!add_watch_tree(std::filesystem::path{*this->path}))
        {
            host.clog("Failed to add inotify watch for " + *this->path + ": " + std::string(std::strerror(errno)));
            ::close(std::exchange(fd, -1));
            return false;
        }

        watcher = std::thread{[this]() {
            std::array<char, 16 * 1024> buffer;
            pollfd pfd{fd, POLLIN, 0};
            watching = true;

            while(watching)
            {
                int const ready = ::poll(&pfd, 1, 250);
                if(ready < 0)
                {
                    if(errno == EINTR) continue;
                    break;
                }
                if(ready == 0 || !(pfd.revents & POLLIN)) continue;

                for(;;)
                {
                    ssize_t const length = ::read(fd, buffer.data(), buffer.size());
                    if(length < 0)
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        if(errno == EINTR) continue;
                        return;
                    }
                    if(length == 0) break;

                    for(char const *ptr = buffer.data(); ptr < buffer.data() + length;)
                    {
                        auto const *event = reinterpret_cast<inotify_event const *>(ptr);
                        if(auto it = watches.find(event->wd); it != watches.end())
                        {
                            std::filesystem::path changed = it->second;
                            if(event->len) changed /= event->name;

                            if(event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
                            {
                                watches.erase(it);
                            }
                            else
                            {
                                if(event->mask & IN_ISDIR)
                                {
                                    if(event->mask & (IN_CREATE | IN_MOVED_TO)) add_watch_tree(changed);
                                }
                                else if(event->mask & (IN_CREATE | IN_MODIFY | IN_MOVED_TO))
                                {
                                    std::error_code ec;
                                    if(!std::filesystem::exists(changed, ec) || !std::filesystem::is_regular_file(changed, ec)) return;

                                    if(!thread) thread.emplace(1);
                                    Event e{noto::fs::Path::n2v(changed).string()};
                                    boost::asio::post(*Plugin::self->thread, [e=std::move(e)]() mutable {
                                        Plugin::host->exec(*Plugin::self->book, e);
                                    });
                                }
                            }
                        }
                        ptr += sizeof(inotify_event) + event->len;
                    }
                }
            }
        }};
#endif
        return true;
    }

    void end(notojs::IHost &) override
    {
#if defined(__APPLE__)
        if(stream)
        {
            stream.reset();
            dispatch_release(queue);
        }
#elif defined(__linux__)
        watching = false;
        if(watcher.joinable()) watcher.join();
        if(fd >= 0) ::close(std::exchange(fd, -1));
        watches.clear();
#endif
        if(thread) thread->join();
    }

    static JSValue _path(JSContext *ctx);
    notojs::IPlugin::Module mod(notojs::IHost &) override;

private:
    std::optional<std::string> book;
    std::optional<std::string> path;

private:
#if defined(__APPLE__)
    struct Deleter
    {
        void operator() (CFArrayRef s)
        {
            CFRelease(s);
        }
        void operator() (CFStringRef s)
        {
            CFRelease(s);
        }
        void operator() (FSEventStreamRef s)
        {
            FSEventStreamStop(s);
            FSEventStreamInvalidate(s);
            FSEventStreamRelease(s);
        }
    };
    dispatch_queue_t queue{nullptr};
    std::unique_ptr<__FSEventStream, Deleter> stream;
#elif defined(__linux__)
    int fd{-1};
    std::thread watcher;
    std::atomic_bool watching{false};
    std::unordered_map<int, std::filesystem::path> watches;

    static constexpr std::uint32_t mask = IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF;

    bool add_watch(std::filesystem::path const &dir)
    {
        int const wd = inotify_add_watch(fd, dir.c_str(), mask);
        if(wd < 0) return false;

        watches[wd] = dir;
        return true;
    }

    bool add_watch_tree(std::filesystem::path const &dir)
    {
        std::error_code ec;
        if(!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return false;

        bool const root = add_watch(dir);
        for(std::filesystem::recursive_directory_iterator i{dir, std::filesystem::directory_options::skip_permission_denied, ec}, end;
            i != end;
            i.increment(ec))
        {
            if(ec)
            {
                ec.clear();
                continue;
            }
            if(i->is_directory(ec)) add_watch(i->path());
            if(ec) ec.clear();
        }
        return root;
    }
#endif
    std::optional<boost::asio::thread_pool> thread;

    static Plugin *self;
    static notojs::IHost *host;
    static JSCFunctionListEntry func[];
    static int init(JSContext *ctx, JSModuleDef *def);
};

Plugin *Plugin::self{nullptr};
notojs::IHost *Plugin::host{nullptr};

JSCFunctionListEntry Plugin::func[] = {
    JS_CFUNC_DEF("path", 0, bridge::Function<&Plugin::_path>::invoke),
};

JSValue Plugin::_path(JSContext *ctx)
{
    if(!self->path) return JS_NULL;
    return noto::fs::Path::make(ctx, std::filesystem::path(noto::fs::Path::n2v(*self->path)));
}

int Plugin::init(JSContext *ctx, JSModuleDef *m)
{
    notojs::fs::init(ctx);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

notojs::IPlugin::Module Plugin::mod(notojs::IHost &host)
{
    Plugin::self = this;
    Plugin::host = &host;
    return [](JSContext *ctx, char const *name) -> JSModuleDef * {
        JSModuleDef *mod = JS_NewCModule(ctx, name, &init);
        if(!mod) return NULL;

        JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
        return mod;
    };
}

} // namespace

extern "C" {

notojs::IPlugin *notojs_plugin(boost::property_tree::ptree const &pt)
{
    return new Plugin(pt);
}

} // extern "C"
