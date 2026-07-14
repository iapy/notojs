#pragma once
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <boost/config.hpp>
#include <unordered_set>
#include <optional>
#include <string>
#include <array>

namespace notojs {

class Writer
{
public:
    Writer(std::string &output)
    : writer{*this}
    , output{output}
    {}

public: // Engine interface
    BOOST_FORCEINLINE void start()
    {
        writer.StartArray();
    }

    BOOST_FORCEINLINE void end()
    {
        writer.EndArray();
    }

    BOOST_FORCEINLINE void startObject(char const *type, std::size_t len)
    {
        writer.StartObject();
        writer.Key("type");
        writer.String(type, len);
        writer.Key("data");
    }

    BOOST_FORCEINLINE void string(char const *value, std::size_t len)
    {
        writer.String(value, len);
    }

    BOOST_FORCEINLINE void string(char const *key, char const *value, std::size_t len)
    {
        writer.Key(key);
        writer.String(value, len);
    }

    BOOST_FORCEINLINE void json(char const *key, char const *value, std::size_t len, rapidjson::Type type)
    {
        writer.Key(key);
        writer.RawValue(value, len, type);
    }

    BOOST_FORCEINLINE void json(char const *value, std::size_t len, rapidjson::Type type)
    {
        writer.RawValue(value, len, type);
    }

    BOOST_FORCEINLINE void integer(char const *key, std::int32_t val)
    {
        writer.Key(key);
        writer.Int(val);
    }

    BOOST_FORCEINLINE void null()
    {
        writer.Null();
    }

    BOOST_FORCEINLINE void startObject()
    {
        writer.StartObject();
    }

    BOOST_FORCEINLINE void endObject()
    {
        writer.EndObject();
    }

    BOOST_FORCEINLINE void startArray(char const *name)
    {
        writer.Key(name);
        writer.StartArray();
    }

    BOOST_FORCEINLINE void endArray()
    {
        writer.EndArray();
    }

    BOOST_FORCEINLINE void string(std::string &&value)
    {
        writer.String(value.c_str(), value.size());
    }

    virtual void renderers(std::unordered_set<std::string> const &rs)
    {
        constexpr std::string_view type{"notojs.Render"};
        if(rs.empty()) return;
        startObject(type.data(), type.size());
        writer.StartArray();
        for(auto const &r : rs) writer.String(r.c_str(), r.size());
        writer.EndArray();
        writer.EndObject();
    }

public: // rapidjson interface
    std::array<char, 256> buf;
    std::size_t pos{0};
    using Ch = char;

    BOOST_FORCEINLINE void Flush()
    {
        output.append(std::begin(buf), std::begin(buf) + pos);
        pos = 0;
    }
    BOOST_FORCEINLINE void Put(char c)
    {
        if(pos == buf.size()) Flush();
        buf[pos++] = c;
    }

private:
    rapidjson::Writer<Writer> writer;
    std::string &output;
};

class Silent
{
public:
    struct Error
    {
        std::string json;
        std::optional<std::string> stack;
    };

    using Output = std::optional<Error>;

    Silent(Output &output)
    : output{output} {}

public: // Engine interface
    BOOST_FORCEINLINE void start() {}
    BOOST_FORCEINLINE void end() {}

    BOOST_FORCEINLINE void startObject(char const *type, std::size_t len)
    {
        constexpr std::string_view Error{"Error"};
        if(1 == ++level && !output && len >= Error.size() && !std::strncmp(type + (len - Error.size()), Error.data(), Error.size()))
        {
            writer.emplace(output.emplace().json);
        }
        if(writer) writer->startObject(type, len);
    }

    BOOST_FORCEINLINE void string(char const *value, std::size_t len)
    {
        if(writer) writer->string(value, len);
    }

    BOOST_FORCEINLINE void string(char const *key, char const *value, std::size_t len)
    {
        if(writer)
        {
            if(std::strcmp(key, "stack")) writer->string(key, value, len);
            else output->stack.emplace(value, len);
        }
    }

    BOOST_FORCEINLINE void json(char const *key, char const *value, std::size_t len, rapidjson::Type type)
    {
        if(writer) writer->json(key, value, len, type);
    }

    BOOST_FORCEINLINE void json(char const *value, std::size_t len, rapidjson::Type type)
    {
        if(writer) writer->json(value, len, type);
    }

    BOOST_FORCEINLINE void integer(char const *key, std::int32_t val)
    {
        if(writer) writer->integer(key, val);
    }

    BOOST_FORCEINLINE void null()
    {
        if(writer) writer->null();
    }

    BOOST_FORCEINLINE void startObject()
    {
        if(++level; writer) writer->startObject();
    }

    BOOST_FORCEINLINE void endObject()
    {
        if(--level; writer)
        {
            writer->endObject();
            if(!level) writer.reset();
        }
    }

    BOOST_FORCEINLINE void startArray(char const *name)
    {
        if(writer) writer->startArray(name);
    }

    BOOST_FORCEINLINE void string(std::string &&value)
    {
        if(writer) writer->string(std::move(value));
    }

    BOOST_FORCEINLINE void endArray()
    {
        if(writer) writer->endArray();
    }

    BOOST_FORCEINLINE void renderers(std::unordered_set<std::string> const &) {}

private:
    Output &output;
    std::size_t level{0};
    std::optional<Writer> writer;
};

} // namespace notojs
