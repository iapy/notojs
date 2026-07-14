#include <boost/process/v2/stdio.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <regex>
#include <map>
#include <set>

#ifndef STANDALONE
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

std::map<std::string, std::string> RESOURCES;
std::map<std::string, std::string> const TYPES = {{"truetype", ".ttf"}};
#endif

using Resolver = boost::asio::ip::tcp::resolver;
using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;
using SSLStream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
boost::asio::ssl::context sslcontext{boost::asio::ssl::context::sslv23_client};

#define VERIFY(p, ...) if(!p) { std::cerr << __VA_ARGS__ << '\n'; exit(1); }

class Worker
{
public:
    Worker(std::filesystem::path &&cache)
    : cache(std::move(cache))
    , thread{[this]{
        ioc.run();
    }}
    {
        if(!std::filesystem::exists(this->cache))
            std::filesystem::create_directories(this->cache);
    }

    std::tuple<std::string, std::filesystem::path, bool> load(std::string const &url_)
    {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        VERIFY(ctx, "EVP_MD_CTX_new failed");

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;

        VERIFY((1 == EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)), "EVP_DigestInit");
        VERIFY((1 == EVP_DigestUpdate(ctx, url_.data(), url_.size())), "EVP_DigestUpdate");
        VERIFY((1 == EVP_DigestFinal_ex(ctx, hash, &hash_len)), "EVP_DigestFinal_ex");
        EVP_MD_CTX_free(ctx);

        static const char* hex = "0123456789abcdef";
        std::string hash_;
        hash_.reserve(hash_len * 2);

        for (unsigned i = 0; i < hash_len; ++i)
        {
            hash_.push_back(hex[hash[i] >> 4]);
            hash_.push_back(hex[hash[i] & 0x0f]);
        }

        auto cache = this->cache / hash_;
        if(std::filesystem::exists(cache))
        {
            std::ifstream ifs(cache, std::ios::binary);
            return std::make_tuple(std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()), cache, true);
        }

        auto url = boost::urls::parse_uri(url_);
        VERIFY(url, "URL not parsed: " << url_);

        std::string port = url->port();
        if(port.empty()) port = std::to_string(443);

        boost::system::error_code ec;
        auto const results = resolver.resolve(url->host(), port, ec);
        VERIFY(!ec, "Resolving " << url->host() << ": " << ec);

        std::string path = url->path();
        if(auto q = url->encoded_query(); !q.empty())
        {
            path.append("?");
            path.append(q);
        }

        auto request = Request{boost::beast::http::verb::get, path, 11};
        request.set(boost::beast::http::field::host, url->host());

        SSLStream stream{resolver.get_executor(), sslcontext};
        bool ssl = SSL_set_tlsext_host_name(stream.native_handle(), url->host().data());
        VERIFY(ssl, "SSL error " << url->host());

        boost::asio::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(boost::asio::ssl::stream_base::client);

        boost::beast::http::write(stream, request, ec);
        VERIFY(!ec, "Sending request " << url_ << ": " << ec);

        Response response;
        boost::asio::streambuf buffer;
        boost::beast::http::read(stream, buffer, response, ec);
        VERIFY(!ec, "Reading response " << url_ << ": " << ec);
        VERIFY((boost::beast::http::status::ok == response.result()), "Bad response code " <<  url_ << " :" << response.result());

        std::string data = response.body();
        std::ofstream(cache, std::ios::binary).write(data.c_str(), data.size());

        return std::make_tuple(std::move(data), cache, false);
    }

    ~Worker() {
        ioc.stop();
        thread.join();
    }

    BOOST_FORCEINLINE boost::asio::io_context & ctx() { return ioc; }

private:
    std::filesystem::path const cache;
    boost::asio::io_context ioc{1};
    Resolver resolver{ioc};
    std::thread thread;
};

#ifndef STANDALONE
const std::filesystem::path BINARY_DIR{CMAKE_CURRENT_BINARY_DIR};
boost::uuids::random_generator gen;

void list(std::set<std::string> &target, std::string &&source)
{
    std::string value;
    for(std::istringstream is(source); is;)
    {
        is >> value;
        if(value.empty()) continue;
        target.insert(std::move(value));
    }
}

std::string variable(std::string name)
{
    std::replace(std::begin(name), std::end(name), '.', '_');
    std::replace(std::begin(name), std::end(name), '-', '_');
    std::transform(std::begin(name), std::end(name), std::begin(name), [](char ch){
        return ::toupper(ch);
    });
    return name;
}

int server(boost::property_tree::ptree const &pt, int argc, char **argv)
{
    Worker worker(std::filesystem::path(CMAKE_CURRENT_BINARY_DIR) / "cache");

    std::ostringstream os;
    os << pt.get<std::string>("icons.source");

    std::set<std::string> icons, statics, bundles;
    list(icons, pt.get<std::string>("icons.icons"));
    list(statics, pt.get<std::string>("statics"));
    list(bundles, pt.get<std::string>("bundles"));

    bool first{true};
    for(auto const &icon: icons)
    {
        if(icon.empty()) continue;
        if(!std::exchange(first, false)) os << ',';
        os << icon;
    }

    for(auto const &[name, node]: pt.get_child("sources"))
        RESOURCES[name] = node.get_value<std::string>();

    std::ofstream output(BINARY_DIR / "bundle.cpp");
    output << "#include <string_view>\n";
    output << "#include <string>\n";
    output << "namespace notojs {\n";

    // Generate icons
    {
        auto [content, _, cached]  = worker.load(std::move(os.str()));
        std::cout << (cached ? "Cached " : "Fetched ") <<  pt.get<std::string>("icons.target")<< " - " << content.size() << "b\n";
        if(std::smatch m; std::regex_search(content, m, std::regex("src:[ ]*url\\(([^\\)]*)\\)[ ]*format\\('([a-z]*)'\\)"))) {
            if(auto const it = TYPES.find(m[2]); it != std::end(TYPES)) {
                auto const path = std::filesystem::path(pt.get<std::string>("icons.target"))
                    .replace_extension(it->second).string();

                RESOURCES[path] = m[1];
                content.replace(m[1].first, m[1].second, "\"./" + path + "\"");
            }
        }

        auto const name = variable("icons.css");
        output << "extern const std::string_view " << name << ";\n";
        output << "const std::string_view " << name << " = R\"GENERATED(";
        output << content;
        output << ")GENERATED\";\n\n";
    }

    for(auto const &[n, url]: RESOURCES)
    {
        auto [content, q, cached] = worker.load(url);
        std::cout << (cached ? "Cached " : "Fetched ") << n << " - " << content.size() << "b\n";
        if(auto p = std::filesystem::path(n); p.extension() == ".ttf")
        {
            auto const name = variable(n);
            output << "extern const std::string_view " << name << ";\n";
            output << "const unsigned char " << name << "_bytes[] = {";

            first = true;
            for(std::size_t i = 0; i < content.size(); ++i)
            {
                output << (std::exchange(first, false) ? "0x" : ",0x") << std::hex << static_cast<std::uint32_t>(static_cast<std::uint8_t>(content[i]));
            }
            output << "};\n";
            output << "const std::string_view " << name << "{reinterpret_cast<char const*>(&" << name << "_bytes[0]), 0x" << content.size() << "};\n\n" << std::dec;
        }
        else if(p.filename() != p)
        {
            std::filesystem::create_directories(BINARY_DIR / "cache" / p.parent_path());
            auto const target = BINARY_DIR / "cache" / p;
            if(!std::filesystem::exists(target))
            {
                std::filesystem::create_symlink(q, target);
            }
            else if(std::filesystem::read_symlink(target) != q)
            {
                std::filesystem::remove(target);
                std::filesystem::create_symlink(q, target);
            }
        }
        else if(statics.count(n) || p.extension() == ".css")
        {
            auto const name = variable(n);
            output << "extern const std::string_view " << name << ";\n";
            output << "const std::string_view " << name << " = R\"GENERATED(";
            output << content;
            output << ")GENERATED\";\n\n";
        }
        else
        {
            auto const target = BINARY_DIR / n;
            if(!std::filesystem::exists(target))
            {
                std::filesystem::create_symlink(q, target);
            }
            else if(std::filesystem::read_symlink(target) != q)
            {
                std::filesystem::remove(target);
                std::filesystem::create_symlink(q, target);
            }
        }
    }

    std::cout.flush();
    std::array<char, 4096> buffer;
    for(int i = 1; i < argc; ++i)
    {
        auto const path = std::filesystem::path(argv[i]);
        auto const asis = path.extension() == ".html" || path.extension() == ".ini" || path.extension() == ".webmanifest";
        auto const name = variable(path.filename().u8string());

        const char *type = path.extension() == ".html"
            ? "std::string"
            : "const std::string_view"
        ;

        output << "extern " << type << ' ' << name << ";\n";
        if(path.extension() == ".ico" || path.extension() == ".png")
        {
            output << "const unsigned char " << name << "_bytes[] = {";

            first = true;
            auto stream = std::ifstream(path);
            auto content = std::vector<std::uint8_t>{
                std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>()
            };
            for(std::size_t i = 0; i < content.size(); ++i)
            {
                output << (std::exchange(first, false) ? "0x" : ",0x") << std::hex << static_cast<std::uint32_t>(static_cast<std::uint8_t>(content[i]));
            }
            output << "};\n";
            output << "const std::string_view " << name << "{reinterpret_cast<char const*>(&" << name << "_bytes[0]), 0x" << content.size() << "};\n\n" << std::dec;
        }
        else
        {
            output << type << ' ' << name << " = R\"GENERATED(";
            boost::system::error_code ec;
            namespace bp = boost::process::v2;
            if(auto fn = path.filename(); asis)
            {
                output << std::ifstream(path).rdbuf();
            }
            else if(!bundles.count(fn.u8string()))
            {
                boost::asio::readable_pipe pipe{worker.ctx()};
                bp::process proc(
                    worker.ctx(),
                    (BINARY_DIR / "esbuild").u8string(),
                    {path.u8string(), "--legal-comments=none", "--minify-whitespace"},
                    bp::process_stdio{{}, pipe, {}});

                while (true)
                {
                    std::size_t n = pipe.read_some(boost::asio::buffer(buffer), ec);
                    if (ec == boost::asio::error::eof)
                        break;
                    if (ec == boost::asio::error::interrupted)
                        continue;
                    VERIFY(!ec, "Error reading pipe");
                    output.write(buffer.data(), n);
                }
                proc.wait();
                if(0 != proc.exit_code())
                {
                    output.close();
                    std::filesystem::remove(BINARY_DIR / "bundle.cpp");
                    return EXIT_FAILURE;
                }
            }
            else
            {
                std::filesystem::copy(path, BINARY_DIR / fn, std::filesystem::copy_options::overwrite_existing);
                boost::asio::readable_pipe pipe{worker.ctx()};
                bp::process proc(
                    worker.ctx(),
                    (BINARY_DIR / "esbuild").u8string(),
                    {(BINARY_DIR / fn).u8string(), "--legal-comments=none", "--bundle", "--format=esm", "--minify-syntax", "--minify-whitespace"},
                    bp::process_stdio{{}, pipe, {}});

                while (true)
                {
                    std::size_t n = pipe.read_some(boost::asio::buffer(buffer), ec);
                    if (ec == boost::asio::error::eof)
                        break;
                    if (ec == boost::asio::error::interrupted)
                        continue;
                    VERIFY(!ec, "Error reading pipe");
                    output.write(buffer.data(), n);
                }
                proc.wait();
                if(0 != proc.exit_code())
                {
                    output.close();
                    std::filesystem::remove(BINARY_DIR / "bundle.cpp");
                    return EXIT_FAILURE;
                }
            }
            output << ")GENERATED\";\n\n";
        }
    }

    output << "extern std::string_view ETAG;\nstd::string_view ETAG{\"\\\"" << boost::uuids::to_string(gen()) << "\\\"\"};\n\n";

    output << "} // namespace notojs\n";
    output.close();

    return EXIT_SUCCESS;
}
#endif

int render(boost::property_tree::ptree const &pt, int argc, char **argv)
{
    namespace po = boost::program_options;
    po::options_description desc{"Options"};

    std::filesystem::path cache;
    std::filesystem::path esbuild;
    std::filesystem::path target;

    desc.add_options()
        ("cache", po::value<std::filesystem::path>(&cache)->required())
        ("esbuild", po::value<std::filesystem::path>(&esbuild)->required())
        ("target", po::value<std::filesystem::path>(&target)->required())
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(std::filesystem::exists(target)) VERIFY(std::filesystem::is_directory(target), "Expecting directory");

    Worker worker(std::move(cache));
    if(auto sources = pt.get_child_optional("sources")) for(auto const &[name, node]: *sources) {
        auto [text, path, cached] = worker.load(node.get_value<std::string>());
        std::cout << (cached ? "Cached " : "Fetched ") << name << " - " << text.size() << "b\n";

        if(auto target = std::filesystem::path(name); !std::filesystem::exists(name))
        {
            std::filesystem::create_symlink(path, target);
        }
        else if(std::filesystem::read_symlink(target) != path)
        {
            std::filesystem::remove(target);
            std::filesystem::create_symlink(path, target);
        }
    }

    namespace bp = boost::process::v2;
    std::filesystem::create_directories(target);

    auto const client = (target / "client.js");
    auto const client_src = boost::filesystem::unique_path("client-%%%%-%%%%.js").string();
    std::filesystem::copy(std::filesystem::path(argv[0]) / "client.js", client_src,
        std::filesystem::copy_options::update_existing);

    if(std::filesystem::is_regular_file(client)) std::filesystem::remove(client);
    auto clientp = bp::process(worker.ctx(), esbuild.u8string(), {
        "--bundle", client_src,
        "--legal-comments=none",
        "--minify-whitespace",
        "--format=esm",
        "--define:$RENDERER=\"notojs.Render/" + target.filename().u8string() + "\"",
    }, bp::process_stdio{{}, boost::filesystem::path(client), {}});

    auto const server = (target / "server.js");
    auto const server_src = boost::filesystem::unique_path("server-%%%%-%%%%.js").string();
    std::filesystem::copy(std::filesystem::path(argv[0]) / "server.js", server_src,
        std::filesystem::copy_options::update_existing);

    if(std::filesystem::is_regular_file(server)) std::filesystem::remove(server);
    auto serverp = bp::process(worker.ctx(), esbuild.u8string(), {
        "--bundle", server_src,
        "--legal-comments=none",
        "--minify-whitespace",
        "--format=esm",
        "--define:$RENDERER=\"notojs.Render/" + target.filename().u8string() + "\"",
    }, bp::process_stdio{{}, boost::filesystem::path(server), {}});

    clientp.wait();
    serverp.wait();

    std::filesystem::remove(client_src);
    std::filesystem::remove(server_src);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    VERIFY((argc > 1), "Not enough arguments");

    boost::property_tree::ptree pt;
    auto const path = std::filesystem::path(argv[1]);
    std::function<int(boost::property_tree::ptree const &, int, char**)> worker;
    if(std::filesystem::is_directory(path) && std::filesystem::is_regular_file(path / "bundle.ini"))
    {
        boost::property_tree::ini_parser::read_ini(path / "bundle.ini", pt);
        worker = &render;
    }
#ifndef STANDALONE
    else if(std::filesystem::is_regular_file(path))
    {
        boost::property_tree::ini_parser::read_ini(path, pt);
        worker = &server;
    }
#endif
    else VERIFY(false, "Bad argument " + path.u8string());
    return worker(pt, argc - 1, argv + 1);
}
