#pragma once
#include <clang/AST/ASTContext.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace notojs {

class Output;

// Any type
struct Type
{
    enum Format
    {
        FULL,
        TYPE,
    };

    struct Detail
    {
        virtual void write(Output &, Type const &, Format) const;
        virtual ~Detail() {};
    };

    Type() = default;
    Type(std::string const &);

    std::string name;
    std::unique_ptr<Detail> detail;
    std::optional<std::string> scope;

    template<typename T> T *as()
    {
        return dynamic_cast<T*>(detail.get());
    }

    template<typename T> T const *as() const
    {
        return dynamic_cast<T const *>(detail.get());
    }

    void write(Output &, Format) const;
    static std::string canonical_name(std::string const &);
};

struct Args : std::vector<std::weak_ptr<Type>> {};

// Exception
struct Exception : Type::Detail
{
    void write(Output &, Type const &, Type::Format) const override;
};

// Struct
struct Struct : Type::Detail
{
    std::vector<std::pair<std::string, std::weak_ptr<Type>>> fields;
    void write(Output &, Type const &, Type::Format) const override;
};

// Class template
struct Template : Type::Detail
{
    Args args;
    void write(Output &, Type const &, Type::Format) const override;
};

// Tail template
struct Tail : Template
{
    std::size_t min{0};
    void write(Output &, Type const &, Type::Format) const override;
};

// Validator class
struct Validator : Type::Detail
{
    std::weak_ptr<Type> base;
    void write(Output &, Type const &, Type::Format) const override;
};

// Exported class
struct Class : Type::Detail
{
    std::vector<Args> ctor;
    std::weak_ptr<Type> base;
    std::vector<std::weak_ptr<Type>> impls;
    std::vector<std::weak_ptr<Type>> inner;
    std::vector<std::weak_ptr<Type>> valid;
    std::map<std::string, std::pair<bool, Args>> fields;
    std::map<std::string, std::vector<Args>> methods;
    std::map<std::string, std::vector<Args>> statics;

    void write(Output &, Type const &, Type::Format) const override;
};

class Module
{
public:
    void dump(std::optional<std::filesystem::path> &&);

    template<typename T = Type::Detail>
    std::pair<std::shared_ptr<Type>, T*> find(std::string const &name) const
    {
        if(auto it = types.find(name); it != std::end(types))
            return {it->second, dynamic_cast<T*>(it->second->detail.get())};
        return {nullptr, nullptr};
    }

    template<typename T = Type::Detail>
    std::pair<std::shared_ptr<Type>, T*> make(std::string const &name)
    {
        std::shared_ptr<Type> type;
        if(auto it = types.find(name); it != std::end(types))
            type = it->second;
        else
            type = types[name] = std::make_shared<Type>(name);
        if constexpr (std::is_same_v<T, Class> || std::is_same_v<T, Exception>)
            api.push_back(type);
        if(!dynamic_cast<T*>(type->detail.get()))
            type->detail = std::make_unique<T>();
        return {type, dynamic_cast<T*>(type->detail.get())};
    }

    template<typename T>
    auto make(std::string const &name, Class *owner)
    -> std::enable_if_t<std::is_same_v<T, Struct> || std::is_same_v<T, Validator>, std::pair<std::shared_ptr<Type>, T*>>
    {
        auto [ptr, det] = make<T>(name);
        if constexpr (std::is_same_v<T, Struct>)
        {
            for(auto const &q: owner->inner)
                if(auto r = q.lock(); ptr == r) return {ptr, det};
            owner->inner.push_back(ptr);
        }
        else
        {
            for(auto const &q: owner->valid)
                if(auto r = q.lock(); ptr == r) return {ptr, det};
            owner->valid.push_back(ptr);
        }
        return {ptr, det};
    }

    std::shared_ptr<Type> make(clang::QualType const &type, clang::ASTContext &ctx);
    std::map<std::string, std::vector<Args>> fns;
private:
    // exported types
    std::vector<std::weak_ptr<Type>> api;

    // maps c++ type name to JavaScript class
    std::map<std::string, std::shared_ptr<Type>> types;
};

struct Interface : Type::Detail
{
    struct Signature
    {
        std::string type;
        std::vector<std::string> args;

        static std::string cleanup(std::string);
        static void write(Output &, std::string const &, std::vector<Signature> const &);
    };
    std::map<std::string, std::vector<Signature>> methods;
    std::map<std::string, std::vector<Signature>> statics;

    void write(Output &, Type const &, Type::Format) const override;
};

class Header
{
public:
    void dump(std::optional<std::filesystem::path> &&);

    template<typename T = Interface>
    std::pair<std::shared_ptr<Type>, T*> make(std::string const &name)
    {
        std::shared_ptr<Type> type;
        if(auto it = types.find(name); it != std::end(types))
            type = it->second;
        else
            type = types[name] = std::make_shared<Type>(name);

        if(!dynamic_cast<T*>(type->detail.get()))
            type->detail = std::make_unique<T>();
        return {type, dynamic_cast<T*>(type->detail.get())};
    }

    std::map<std::string, std::vector<Interface::Signature>> fns;

private:
    std::map<std::string, std::shared_ptr<Type>> types;
};

} // namespace notojs
