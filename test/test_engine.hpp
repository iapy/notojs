#include <rapidjson/document.h>
#include <notojs/engine.hpp>
#include <notojs/folder.hpp>
#include <notojs/global.hpp>
#include <notojs/server.hpp>
#include <notojs/writer.hpp>
#include <iostream>

namespace std {

template<typename T>
ostream &operator << (ostream &stream, std::optional<T> const &value)
{
    if(value) return stream << *value;
    return stream << "<null>";
}

inline ostream &operator << (ostream &stream, std::nullopt_t const &)
{
    return stream << "<null>";
}

inline ostream &operator << (ostream &stream, std::reference_wrapper<rapidjson::GenericValue<rapidjson::UTF8<>> const> const &value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.get().Accept(writer);
    return stream << buffer.GetString();
}

} // namespace std

namespace notojs::testing {

extern notojs::Engine *engine;
extern notojs::Global *global;
extern notojs::Folder *folder;
extern notojs::Server *server;

class Fixture
{
private:
    std::optional<rapidjson::Document> document;
    std::string output;

protected:
    Fixture()
    {
        if(auto p = std::filesystem::path(CMAKE_CURRENT_BINARY_DIR) / ".db"; std::filesystem::exists(p))
            std::filesystem::remove_all(p);
    }

    void db()
    {
        (void)folder->set_path(CMAKE_CURRENT_BINARY_DIR);
    }

    std::optional<std::reference_wrapper<rapidjson::GenericValue<rapidjson::UTF8<>> const>> get_output()
    {
        if(!document)
        {
            document.emplace();
            document->Parse(output.c_str(), output.size());
        }
        for(auto const &out : document->GetArray())
        {
            if(out.HasMember("type")
                && out["type"].IsString() 
                && strstr(out["type"].GetString(), "notojs.Output")
                && out.HasMember("data")
            ) return out["data"];
        }
        return std::nullopt;
    }

    std::optional<std::string> get_error()
    {
        if(!document)
        {
            document.emplace();
            document->Parse(output.c_str(), output.size());
        }
        for(auto const &out : document->GetArray())
        {
            if(out.HasMember("type")
                && out["type"].IsString() 
                && std::invoke([&]{
                    const char *type = out["type"].GetString();
                    return strlen(type) >= 5 && strstr(type + strlen(type) - 5, "Error");
                })
                && out.HasMember("data")
                && out["data"].IsObject()
                && out["data"].HasMember("message")
                && out["data"]["message"].IsString()
            ) return out["data"]["message"].GetString();
        }
        return std::nullopt;
    }

    bool eval(const char *code, detail::Bytecode &bytecode)
    {
        output.clear();
        notojs::Silent::Output output;
        notojs::Silent writer{output};
        engine->eval(code, bytecode, writer);
        return !!output;
    }

    boost::beast::http::status eval(const char *code)
    {
        output.clear();
        notojs::Writer writer{output};
        return engine->eval(code, writer);
    }

    boost::beast::http::status eval(detail::Bytecode const &bytecode)
    {
        output.clear();
        notojs::Writer writer{output};
        return engine->eval(bytecode, writer);
    }

    boost::beast::http::status eval(const char *code, JSContext *ctx, std::string &&name)
    {
        output.clear();
        notojs::Writer writer{output};
        return engine->eval(code, writer, ctx, std::move(name));
    }
};

class ContextFixture : public Fixture
{
public:
    ~ContextFixture();

protected:
    ContextFixture();

private:
    std::thread io_context;
};

} //namespace notojs::testing
