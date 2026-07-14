#pragma once
#include <boost/config.hpp>
#include <unordered_map>
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace notojs::doc {

struct Module
{
    struct Doc
    {
        std::string text;
        std::vector<std::string> also;
    };

    struct Exception
    {
        Doc doc;
    };

    struct Field
    {
        Doc doc;
        std::string type;
    };

    struct Type
    {
        Doc doc;
        std::variant<std::string, std::map<std::string, Field>> data;
    };

    struct Function
    {
        Doc doc;
        std::vector<std::string> signatures;
    };

    struct Property
    {
        Doc doc;
        bool getter;
        std::vector<std::string> setter;
    };

    struct Class
    {
        Doc doc;
        std::optional<std::string> base;
        std::map<std::string, Type> types;
        std::set<std::string> implements;
        std::optional<Function> constructor;
        std::map<std::string, Function> statics;
        std::map<std::string, Function> methods;
        std::map<std::string, Property> properties;
    };

    std::map<std::string, Exception> exceptions;
    std::map<std::string, Function> functions;
    std::map<std::string, Class> classes;
    Module::Doc doc;
};

struct Script
{
    std::map<std::string, Module::Type> types;
    std::map<std::string, Module::Function> functions;
    Module::Doc doc;
};

struct Header
{
    struct Function
    {
        std::vector<std::string> signatures;
        Module::Doc doc;
    };

    struct Interface
    {
        std::map<std::string, Function> methods;
        std::map<std::string, Function> statics;
        Module::Doc doc;
    };

    std::map<std::string, Interface> interfaces;
    std::map<std::string, Function> functions;
    Module::Doc doc;
};

struct Suite
{
    std::map<std::string, Module> modules;
    std::map<std::string, Script> scripts;
    std::map<std::string, Header> headers;

    std::unordered_map<std::string, std::variant<std::string, std::map<std::string, std::string>>> topics;
};

} // notojs::doc
