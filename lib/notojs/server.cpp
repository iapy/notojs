#include <notojs/engine.hpp>
#include <notojs/handle.hpp>
#include <notojs/logger.hpp>
#include <notojs/module.hpp>
#include <notojs/plugin.hpp>
#include <notojs/router.hpp>
#include <notojs/server.hpp>
#include <notojs/timers.hpp>
#include <notojs/writer.hpp>
#include <notojs/window.hpp>
#include <memory.hpp>
#include <termios.h>

#include <notojs/detail/cellid.hpp>
#include <notojs/detail/header.hpp>
#include <notojs/detail/logger.hpp>

#include <boost/url.hpp>

#include <fstream>

namespace notojs {

extern std::string WINDOW_HTML;

namespace {

void loop(
    boost::asio::ip::tcp::acceptor &acceptor,
    boost::asio::thread_pool &pool,
    Engine &engine,
    Router &router)
{
    acceptor.async_accept([&](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if(!ec) std::make_shared<Handle>(std::move(socket))->process(engine, router, pool);
        if(boost::asio::error::operation_aborted != ec) loop(acceptor, pool, engine, router);
    });
}

void loop(
    boost::asio::posix::stream_descriptor &i,
    boost::asio::posix::stream_descriptor &o,
    decltype(Socket::buffer) &buffer,
    boost::asio::thread_pool &pool,
    Engine &engine)
{
    auto request = std::make_shared<decltype(Socket::parser)::value_type>();
    boost::beast::http::async_read(i, buffer, *request, [&, request](boost::beast::error_code ec, std::size_t) {
        if(ec) return;

        boost::asio::post(pool, [&, request] {
            thread_local bridge::Context context{engine.get_context()};

            auto response = std::make_shared<decltype(Socket::response)>();
            if(boost::beast::http::verb::put == request->method())
            {
                context.reset(engine.get_context());
                std::clog << "Context restarted\n";
            }
            else
            {
                std::optional<std::string> cid;
                if(auto url = boost::urls::parse_relative_ref(request->target()); url)
                {
                    auto const params = url->params();
                    if(auto it = params.find("cid"); it != params.end())
                        cid = (*it).value;
                }

                Writer writer{response->body()};

                auto const ts = std::chrono::system_clock::now();
                Engine::preprocess(request->body());

                response->set(boost::beast::http::field::content_type, "application/json");
                response->result(engine.eval(request->body(), writer, context.get(), std::move(cid)));

                auto const td = (std::chrono::system_clock::now() - ts);
                response->set(detail::EXEC_TIME, std::to_string(
                    std::chrono::duration_cast<std::chrono::milliseconds>(td).count()
                ) + "ms");
                response->set(detail::EXEC_PROC, std::to_string(
                    ::getpid()
                ));
            }
            response->prepare_payload();
            boost::beast::http::write(o, *response);
        });

        loop(i, o, buffer, pool, engine);
    });
}

std::optional<std::ofstream> logf;
std::unique_ptr<detail::Logger> logger;

} // namespace

void Server::configure(boost::property_tree::ptree const &pt)
{
    auto bind = pt.get<std::string>("server.bind");
    if (auto const pos = bind.find(':'); pos == std::string::npos)
    {
        throw std::invalid_argument("Expected host:port format");
    }
    else
    {
        std::string host = bind.substr(0, pos);
        std::string port = bind.substr(pos + 1);
        endpoint = *std::begin(resolver.resolve(host, port));
    }

    std::string threads = pt.get<std::string>("server.threads", "1,1");
    this->threads.first = std::max(1, std::atoi(threads.c_str()));

    if(auto const pos = threads.find(','); pos != std::string::npos)
    {
        this->threads.second = std::max(1, std::atoi(threads.c_str() + pos + 1));
    }

    WINDOW_HTML.replace(WINDOW_HTML.find("<base/>"), 7, "<base href=\"" + pt.get<std::string>("server.base", "/") + "\"/>");
    if(auto script = pt.get_optional<std::string>("server.script"); script)
    {
        WINDOW_HTML.replace(WINDOW_HTML.find("</head>"), 7, "<script src=\"" + *script + "\"></script></head>");
    }
    get<Router>().window(std::string_view(WINDOW_HTML.c_str(), WINDOW_HTML.size()));

    if (auto log = pt.get_optional<std::string>("server.log"); log)
    {
        std::filesystem::path path = std::filesystem::path(*log);
        std::clog.rdbuf(logf.emplace(path, std::ios_base::app).rdbuf());
        get<Router>().rotate([path]{
            logger.reset();
            logf->close();

            static constexpr char format[] = ".%Y%m%d_%H%M%S";
            static constexpr std::size_t fmtcnt = sizeof(".99991231_235959");

            char buffer[fmtcnt];

            auto const tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::strftime(buffer, fmtcnt, format, std::localtime(&tm));

            std::filesystem::path parent = path.parent_path();
            std::filesystem::path target = parent / (path.stem().u8string() + std::string(&buffer[0], fmtcnt - 1) + path.extension().u8string());
            std::filesystem::rename(path, target);

            std::clog.rdbuf(logf.emplace(path).rdbuf());
            logger.reset(new detail::Logger{});

            return target.u8string();
        });
    }
}

std::thread Server::run(std::function<std::shared_ptr<boost::asio::ip::tcp::acceptor>(boost::asio::io_context &)> &&server)
{
    return std::thread([this, server=std::move(server)]{
        auto socket = server(ioc);
        (void)ioc.run();
        socket->close();
    });
}

void Server::stop(std::thread &thread)
{
    ioc.stop();
    thread.join();
    ioc.restart();
}

int Server::main()
{
    int result{EXIT_FAILURE};
    signal(SIGPIPE, SIG_IGN);

    sync = std::make_unique<boost::asio::thread_pool>(threads.second);
    if(has<Window>())
    {
        termios term;
        logger.reset(new detail::Logger{});
        std::clog << "thread" << notojs::values(threads.first, threads.second);

        boost::asio::thread_pool disk{1};
        boost::asio::thread_pool pool{threads.first};
        boost::asio::ip::tcp::acceptor acceptor{ioc};

        this->disk = &disk;

        get<Plugin>().mod();
        get<Plugin>().run();
        get<Timers>().run();
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& error, int signal){
            if(error)
            {
                std::clog << "error" << notojs::values(error.value());
            }
            else
            {
                std::clog << "signal" << notojs::values(signal);
                get<Plugin>().end();
                get<Timers>().end();
                get<Window>().end();
                ioc.stop();
            }
        });

        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));

        acceptor.bind(endpoint);
        acceptor.listen(boost::asio::socket_base::max_listen_connections);

        tcgetattr(STDIN_FILENO, &term);
        term.c_lflag &= ~ECHOCTL;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);

        loop(acceptor, pool, get<Engine>(), get<Router>());
        result = 0 < ioc.run() ? EXIT_SUCCESS : EXIT_FAILURE;
        pool.join();
        disk.join();

        tcgetattr(STDIN_FILENO, &term);
        term.c_lflag |= ECHOCTL;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }
    else
    {
        std::clog << "started" << notojs::values(threads.second);
        this->disk = sync.get();
        get<Plugin>().mod();
        boost::asio::posix::stream_descriptor i(ioc, ::dup(STDIN_FILENO));
        boost::asio::posix::stream_descriptor o(ioc, ::dup(STDOUT_FILENO));

        boost::asio::thread_pool pool{1};
        decltype(Socket::buffer) buffer;

        loop(i, o, buffer, pool, get<Engine>());
        result = 0 < ioc.run() ? EXIT_SUCCESS : EXIT_FAILURE;
        pool.join();

        std::clog << "stopped\n";
    }
    sync->join();
    return result;
}

} // namespace notojs
