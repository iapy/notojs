#include <notojs/config.hpp>
#include <notojs/logger.hpp>
#include <notojs/handle.hpp>
#include <notojs/server.hpp>
#include <notojs/window.hpp>
#include <chrono>
#include <iostream>
#include <queue>

#include <boost/asio/system_timer.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace notojs {
namespace {

std::atomic<unsigned> total{0};
std::atomic<bool> killed{false};
boost::uuids::random_generator gen;

} // namespace

class Window::Kernel
: private Config::Kernel
, public std::enable_shared_from_this<Kernel>
{
public:
    Kernel(Config::Kernel &&process, Window &window)
    : Config::Kernel{std::move(process)}, window{window} { ++total; }

    void cerr()
    {
        err.async_read_some(boost::asio::buffer(buff), [self=shared_from_this()](boost::system::error_code ec, std::size_t n) {
            self->clog.append(&self->buff[0], n);
            for(auto pos = self->clog.find('\n'); pos != std::string::npos;)
            {
                if(!killed.load(std::memory_order::memory_order_acquire))
                    boost::asio::post(*self->window.get<Server>().disk, [message=std::string(self->clog.c_str(), pos), pid=self->pid()]{
                        (std::clog << "[" << pid << "] " << message << '\n').flush();
                    });
                self->clog.erase(0, pos + 1);
                pos = self->clog.find('\n');
            }
            if(!ec && self->live() && !killed.load(std::memory_order::memory_order_acquire)) self->cerr();
        });
    }

    using Config::Kernel::pid;

    void post(std::shared_ptr<Handle> handle)
    {
        if(live())
        {
            if(requests.push(handle); 1 == requests.size()) post();
        }
    }

    bool kill(int signal)
    {
        if(SIGTERM == signal)
            Config::Kernel::kill();
        else
            cin.close();
        wait();
        return !live();
    }

    ~Kernel()
    {
        if(--total; !killed.load(std::memory_order::memory_order_acquire))
            NOTOJS_LOG(&window, "Kernel stopped", (pid())(code()));
    }
private:
    void cout()
    {
        boost::beast::http::async_read(out, requests.front()->buffer, requests.front()->response,
            [self=shared_from_this()](boost::system::error_code ec, std::size_t n) {
                self->requests.front()->finish(nullptr);
                if(self->requests.pop(); !self->requests.empty() && self->live()) self->post();
            });
    }

    void post()
    {
        boost::beast::http::async_write(cin, requests.front()->parser.get(), [self=shared_from_this()](boost::beast::error_code ec, std::size_t){
            if(!killed.load(std::memory_order::memory_order_acquire))
            {
                if(ec)
                {
                    if(!killed.load(std::memory_order::memory_order_acquire))
                    {
                        NOTOJS_LOG(&self->window, "Error in kernel", (self->pid())(ec.message()));
                        if(!self->live()) NOTOJS_LOG(&self->window, "Kernel died", (self->pid())(self->code()));
                    }
                }
                else self->cout();
            }
        });
    }

private:
    Window &window;
    std::string clog;
    std::array<char, 1024> buff;
    std::queue<std::shared_ptr<Handle>> requests;
};

class Window::Impl
: public std::enable_shared_from_this<Window::Impl>
{
public:
    Impl(Window &window)
    : window{window}
    , uuid{boost::uuids::to_string(gen())}
    {
    }

    void run(std::shared_ptr<Handle> handle)
    {
        std::unique_lock<std::shared_mutex> lock(window.mutex);
        window.windows[uuid] = shared_from_this();

        auto uuid = this->uuid;
        uuid.insert(0, "event: uuid\ndata: ");
        uuid.append("\n\n");

        NOTOJS_LOG(&window, "Window opened", (this->uuid)(window.windows.size()));
        handle->socket.async_send(boost::asio::const_buffer(uuid.data(), uuid.size()), [self=shared_from_this(), handle]
            (boost::beast::error_code ec, std::size_t)
            {
                if(!ec) self->run(handle, std::make_shared<boost::asio::system_timer>(handle->socket.get_executor()));
                else handle->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
            });
    }

    std::optional<std::string> kernel(std::shared_ptr<Handle> &handle)
    {
        if(auto proc = window.get<Config>().fork(handle->socket.get_executor()); proc.live())
        {
            NOTOJS_LOG(&window, "Kernel created", (proc.pid()));
            auto p = std::to_string(proc.pid());
            auto k = std::make_shared<Kernel>(std::move(proc), window);
            kernels.emplace(std::make_pair(p, k));
            k->cerr();
            return p;
        }
        return std::nullopt;
    }

    bool kernel(std::string const &pid)
    {
        if(auto it = kernels.find(pid); it != std::end(kernels))
        {
            if(auto k = it->second.lock(); k) k->kill(SIGTERM);
            NOTOJS_LOG(&window, "Kernel killed", (std::atoi(pid.c_str()))(SIGTERM));
            kernels.erase(it);
            return true;
        }
        return false;
    }

    bool jsexec(std::shared_ptr<Handle> &handle, std::string const &pid)
    {
        if(auto it = kernels.find(pid); it != std::end(kernels))
        {
            if(auto k = it->second.lock(); k)
            {
                k->post(handle);
                return true;
            }
        }
        return false;
    }

    ~Impl()
    {
        for(auto &it : kernels)
        {
            if(auto k = it.second.lock(); k) k->kill(SIGBUS);

            if(killed.load(std::memory_order::memory_order_acquire))
                std::clog << "Kernel killed" << notojs::values(std::atol(it.first.c_str()), SIGBUS);
            else
                NOTOJS_LOG(&window, "Kernel killed", (std::atoi(it.first.c_str()))(SIGBUS));
        }

        if(killed.load(std::memory_order::memory_order_acquire))
        {
            std::clog << "Window closed" << notojs::values(uuid);
        }
        else
        {
            std::unique_lock<std::shared_mutex> lock(window.mutex);
            window.windows.erase(this->uuid);
            NOTOJS_LOG(&window, "Window closed", (uuid)(window.windows.size()));
        }
    }

private:
    void run(std::shared_ptr<Handle> handle, std::shared_ptr<boost::asio::system_timer> timer)
    {

        timer->expires_after(std::chrono::seconds(5));
        timer->async_wait([self=shared_from_this(), handle, timer](boost::beast::error_code){
            std::unique_lock<std::shared_mutex> lock(self->window.mutex);
            self->data.clear();

            for(auto it = std::begin(self->kernels); it != std::end(self->kernels);)
            {
                if(auto k = it->second.lock(); !k)
                {
                    it = self->kernels.erase(it);
                }
                else
                {
                    if(!self->data.empty()) self->data.append(",");
                    self->data.append(it->first);
                    ++it;
                }
            }

            self->data.insert(0, "event: pids\ndata: ");
            self->data.append("\n\n");

            handle->socket.async_send(boost::asio::const_buffer(self->data.data(), self->data.size()), [self, handle, timer]
                (boost::beast::error_code ec, std::size_t)
                {
                    if(!ec) self->run(handle, timer);
                    else handle->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                });
        });
    }

private:
    Window &window;
    std::string data;
    std::string const uuid;
    std::unordered_map<std::string, std::weak_ptr<Kernel>> kernels;
};

void Window::create(std::shared_ptr<Handle> handle)
{
    handle->response.set(boost::beast::http::field::content_type, "text/event-stream");
    handle->response.set(boost::beast::http::field::cache_control, "no-cache");
    boost::asio::post(handle->socket.get_executor(), [this, handle]{
        handle->serializer.emplace(std::move(handle->response));
        boost::beast::http::async_write_header(handle->socket, *(handle->serializer),
            [this, handle](boost::beast::error_code ec, std::size_t){
                if(!ec) std::make_shared<Impl>(*this)->run(handle);
                else handle->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
            });
    });
}

bool Window::kernel(std::shared_ptr<Handle> handle, std::string const &uuid)
{
    switch(handle->parser.get().method())
    {
    case boost::beast::http::verb::get:
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            if(auto it = windows.find(uuid); it == std::end(windows))
                handle->response.result(boost::beast::http::status::not_found);
            else if(auto window = it->second.lock(); !window)
                handle->response.result(boost::beast::http::status::not_found);
            else if(auto pid = window->kernel(handle); pid)
                handle->response.body() = *pid;
            else
                handle->response.result(boost::beast::http::status::internal_server_error);
        }
        break;
    case boost::beast::http::verb::put:
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            if(auto it = windows.find(uuid); it == std::end(windows))
                handle->response.result(boost::beast::http::status::not_found);
            else if(auto window = it->second.lock(); !window)
                handle->response.result(boost::beast::http::status::not_found);
            else if(!window->jsexec(handle, handle->parser.get().body()))
                handle->response.result(boost::beast::http::status::not_found);
            return true;
        }
        break;
    case boost::beast::http::verb::delete_:
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            if(auto it = windows.find(uuid); it == std::end(windows))
                handle->response.result(boost::beast::http::status::not_found);
            else if(auto window = it->second.lock(); !window)
                handle->response.result(boost::beast::http::status::not_found);
            else if(auto killed = window->kernel(handle->parser.get().body()); !killed)
                handle->response.result(boost::beast::http::status::not_found);
        }
        break;
    default:
        handle->response.result(boost::beast::http::status::method_not_allowed);
        break;
    }
    return false;
}

bool Window::jsexec(std::shared_ptr<Handle> handle, std::string const &uuid, std::string const &pid)
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    if(auto it = windows.find(uuid); it == std::end(windows))
        return false;
    else if(auto window = it->second.lock(); !window)
        return false;
    else
        return window->jsexec(handle, pid);
}

void Window::end()
{
    killed.store(true);
}

} // namespace notojs
