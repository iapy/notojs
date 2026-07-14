#include <notojs.hpp>

#include <unordered_map>
#include <iostream>
#include <fstream>

#include <clang/AST/DeclTemplate.h>
#include <yaml-cpp/yaml.h>

namespace notojs {

class Output
{
public:
    struct Docs : std::unordered_map<std::string, std::pair<
        std::optional<std::string>,
        std::optional<YAML::Node>
    >>
    {
        std::vector<YAML::Node> head;
        std::vector<YAML::Node> tail;
    };

    Output(Docs &&parsed)
    : parsed{parsed}
    {
        vector(parsed.head);
    }

    template<typename T>
    YAML::Emitter &operator << (T const &x)
    {
        return emitter << x;
    }

    void flush(std::ostream &stream)
    {
        vector(parsed.tail);
        stream << emitter.c_str() << '\n';
    }

    void docs(std::string const &key)
    {
        emitter << YAML::Key << "docs";
        emitter << YAML::Value << key;
        emitter << YAML::Key << "text";

        auto const doc = resolve(key);
        if(doc.first)
            emitter << YAML::Value << YAML::Literal << *doc.first;
        else
            emitter << YAML::Value << YAML::Literal << ("TODO: " + key);

        if(doc.second)
        {
            emitter << YAML::Key << "also";
            emitter << YAML::Value << *doc.second;
        }
    }

    static Docs parse(std::filesystem::path const &file)
    {
        Docs parsed;
        if(std::filesystem::exists(file))
        {
            std::vector<YAML::Node> *target = &parsed.head;
            for(auto const &doc: YAML::LoadAllFromFile(file))
            {
                if(doc.IsMap() && doc["docs"])
                {
                    target = &parsed.tail;
                    find(doc, parsed);
                }
                else
                {
                    target->push_back(doc);
                }
            }
        }
        return parsed;
    }

private:
    void vector(std::vector<YAML::Node> const &v)
    {
        for(auto const &m: v)
        {
            if(m.IsScalar()) emitter << YAML::Value << YAML::Literal << m.as<std::string>();
            else
            {
                emitter << YAML::BeginMap;
                for(auto const it: m)
                {
                    emitter << YAML::Key << it.first;
                    if("text" == it.first.as<std::string>())
                        emitter << YAML::Value << YAML::Literal << it.second;
                    else
                        emitter << YAML::Value << it.second;
                }
                emitter << YAML::EndMap;
            }
        }
    }

    static void find(YAML::Node const &node, Docs &parsed)
    {
        if(!node) return;

        if(node.IsMap())
        {
            if(auto text = node["text"]; text && text.IsScalar())
            {
                if(auto docs = node["docs"]; docs && docs.IsScalar())
                {
                    parsed[docs.as<std::string>()].first = text.as<std::string>();
                    if(auto also = node["also"]; also && also.IsSequence())
                        parsed[docs.as<std::string>()].second = also;
                }
            }
            for(auto it : node)
                find(it.second, parsed);
        }
        else if(node.IsSequence())
        {
            for(auto const& item : node)
                find(item, parsed);
        }
    }

private:
    Docs::mapped_type resolve(std::string const &key)
    {
        if(auto it = parsed.find(key); it != std::end(parsed))
            return it->second;
        return {key, std::nullopt};
    }

private:
    Docs parsed;
    YAML::Emitter emitter;
};

Type::Type(std::string const &name)
: name{canonical_name(name)} {}

std::string Type::canonical_name(std::string const &name)
{
    return name == "bridge::Value" || name == "bridge::Either"
        ? "Any"
        : name.rfind("bridge::")
            ? name.rfind("notojs::")
                ? name
                : name.substr(8)
            : name == "bridge::Lambda"
                ? "Function"
                : name.substr(8);
}

void Type::Detail::write(Output &o, Type const &t, Format f) const
{
    if(Type::TYPE == f)
    {
        if(t.scope) o << (*t.scope + "." + t.name);
        else o << t.name;
    }
}

void Type::write(Output &o, Type::Format f) const
{
    detail->write(o, *this, f);
}

void Exception::write(Output &o, Type const &t, Type::Format f) const
{
    o << YAML::BeginMap;
    o << YAML::Key << "exception";
    o << YAML::Value << t.name;
    o.docs(t.name);
    o << YAML::EndMap;
}

void Class::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL == f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << "class";
        o << YAML::Value << t.name;
        if(auto const b = base.lock())
        {
            o << YAML::Key << "base";
            o << YAML::Value << b->name;
        }
        o.docs(t.name);
        if(!impls.empty())
        {
            o << YAML::Key << "impl";
            o << YAML::BeginSeq;
            for(auto const &i : impls)
            {
                i.lock()->write(o, Type::TYPE);
            }
            o << YAML::EndSeq;
        }
        if(!inner.empty() || !valid.empty())
        {
            o << YAML::Key << "defs";
            o << YAML::BeginSeq;
            for(auto const &v: valid)
            {
                v.lock()->write(o, Type::FULL);
            }
            for(auto const &i: inner)
            {
                i.lock()->write(o, Type::FULL);
            }
            o << YAML::EndSeq;
        }
        if(!ctor.empty())
        {
            o << YAML::Key << "ctor";
            o << YAML::BeginMap;
            o << YAML::Key << "args";
            o << YAML::BeginSeq;
            for(auto const &c: ctor)
            {
                o << YAML::BeginSeq;
                for(auto const &t: c)
                    t.lock()->write(o, Type::TYPE);
                o << YAML::EndSeq;
            }
            o << YAML::EndSeq;
            o.docs(t.name + ".constructor");
            o << YAML::EndMap;
        }
        if(!fields.empty())
        {
            o << YAML::Key << "properties";
            o << YAML::BeginMap;
            for(auto const &[name, field]: fields)
            {
                o << YAML::Key << name;
                o << YAML::BeginMap;
                o << YAML::Key << "getter" << field.first;
                if(!field.second.empty())
                {
                    o << YAML::Key << "setter";
                    o << YAML::BeginSeq;
                    for(auto const &t: field.second)
                        t.lock()->write(o, Type::TYPE);
                    o << YAML::EndSeq;
                }
                o.docs(t.name + "." + name);
                o << YAML::EndMap;
            }
            o << YAML::EndMap;
        }
        if(!methods.empty())
        {
            o << YAML::Key << "methods";
            o << YAML::BeginMap;
            for(auto const &[name, sign]: methods)
            {
                o << YAML::Key << name;
                o << YAML::BeginMap;
                o << YAML::Key << "args";
                o << YAML::BeginSeq;
                for(auto const &s: sign)
                {
                    o << YAML::BeginSeq;
                    for(auto const &t: s)
                        t.lock()->write(o, Type::TYPE);
                    o << YAML::EndSeq;
                }
                o << YAML::EndSeq;
                o.docs(t.name + "." + name);
                o << YAML::EndMap;
            }
            o << YAML::EndMap;
        }
        if(!statics.empty())
        {
            o << YAML::Key << "statics";
            o << YAML::BeginMap;
            for(auto const &[name, sign]: statics)
            {
                o << YAML::Key << name;
                o << YAML::BeginMap;
                o << YAML::Key << "args";
                o << YAML::BeginSeq;
                for(auto const &s: sign)
                {
                    o << YAML::BeginSeq;
                    for(auto const &t: s)
                        t.lock()->write(o, Type::TYPE);
                    o << YAML::EndSeq;
                }
                o << YAML::EndSeq;
                o.docs(t.name + "." + name);
                o << YAML::EndMap;
            }
            o << YAML::EndMap;
        }
        o << YAML::EndMap;
    }
    else Type::Detail::write(o, t, f);
}

void Struct::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL == f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << "struct";
        o << YAML::Value << t.name;
        o << YAML::Key << "fields";
        o << YAML::BeginMap;
        for(auto const &[name, type]: fields)
        {
            o << YAML::Key << name;
            o << YAML::BeginMap;
            o << YAML::Key << "type";
            o << YAML::Value;
            type.lock()->write(o, Type::TYPE);
            o.docs(*t.scope + "." + t.name + "." + name);
            o << YAML::EndMap;
        }
        o << YAML::EndMap;
        o.docs(*t.scope + "." + t.name);
        o << YAML::EndMap;
    }
    else Type::Detail::write(o, t, f);
}

void Template::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL != f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << t.name;
        o << YAML::BeginSeq;
        for(auto const &arg: args)
        {
            arg.lock()->write(o, Type::TYPE);
        }
        o << YAML::EndSeq;
        o << YAML::EndMap;
    }
}

void Tail::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL != f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << t.name;
        o << YAML::BeginSeq;
        o << min;
        for(auto const &arg: args)
        {
            arg.lock()->write(o, Type::TYPE);
        }
        o << YAML::EndSeq;
        o << YAML::EndMap;
    }
}

void Validator::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL == f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << "type";
        o << YAML::Value << t.name;
        o << YAML::Key << "base";
        base.lock()->write(o, Type::TYPE);
        o.docs(*t.scope + "." + t.name);
        o << YAML::EndMap;
    }
    else Type::Detail::write(o, t, f);
}

void Module::dump(std::optional<std::filesystem::path> &&file)
{
    Output::Docs docs;
    if(file) docs = Output::parse(*file);

    Output yaml(std::move(docs));
    for(auto const &w: api)
    {
        auto const t = w.lock();
        t->write(yaml, Type::FULL);
    }
    for(auto const &[name, sign]: fns)
    {
        yaml << YAML::BeginMap;
        yaml << YAML::Key << "function";
        yaml << YAML::Value << name;
        yaml << YAML::Key << "args";
        yaml << YAML::BeginSeq;
        for(auto const &s: sign)
        {
            yaml << YAML::BeginSeq;
            for(auto const &t: s)
                t.lock()->write(yaml, Type::TYPE);
            yaml << YAML::EndSeq;
        }
        yaml << YAML::EndSeq;
        yaml.docs(name);
        yaml << YAML::EndMap;
    }
    if(file)
    {
        std::ofstream stream{*file};
        yaml.flush(stream);
    }
    else
    {
        yaml.flush(std::cout);
    }
}

std::shared_ptr<Type> Module::make(clang::QualType const &type, clang::ASTContext &ctx)
{
    if(auto const *d = type->getAsCXXRecordDecl())
    {
        if(auto const *t = type->getAs<clang::TemplateSpecializationType>())
        {
            Template *det;
            std::shared_ptr<Type> ptr;

            auto args = t->template_arguments();
            auto it = std::begin(args);

            if("bridge::Tail" == d->getQualifiedNameAsString())
            {
                std::tie(ptr, det) = make<Tail>(type.getAsString());
                if(clang::TemplateArgument::Integral == it->getKind())
                    dynamic_cast<Tail*>(det)->min = (it++)->getAsIntegral().getSExtValue();
                else if(clang::TemplateArgument::Expression == it->getKind())
                    if(clang::Expr::EvalResult result; (it++)->getAsExpr()->EvaluateAsInt(result, ctx))
                        dynamic_cast<Tail*>(det)->min = result.Val.getInt().getSExtValue();
            }
            else
            {
                std::tie(ptr, det) = make<Template>(type.getAsString());
            }
            ptr->name = Type::canonical_name(d->getQualifiedNameAsString());
            if(std::end(args) - it != det->args.size())
                for(;it != std::end(args); ++it)
                    det->args.push_back(make(it->getAsType(), ctx));
            return ptr;
        }
        else if("Impl" == d->getNameAsString())
        {
            if(auto const *p = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(d->getDeclContext()))
                if(auto const *t = p->getSpecializedTemplate(); t && "bridge::Interface" == t->getQualifiedNameAsString())
                    if(auto const &args = p->getTemplateArgs(); std::invoke([&]{
                        if(args.size() < 2) return false;
                        if(clang::TemplateArgument::Type != args[0].getKind()) return false;
                        if(clang::TemplateArgument::Type != args[1].getKind()) return false;

                        clang::QualType qt = args[1].getAsType().getCanonicalType();
                        if(const auto *pt = qt->getAs<clang::PointerType>())
                            return pt->getPointeeType().isCanonical() && pt->getPointeeType()->isVoidType();
                        return false;
                    }))
                    {
                        if("Interface" == args[0].getAsType()->getAsCXXRecordDecl()->getNameAsString())
                            if(const auto *parent = llvm::dyn_cast<clang::CXXRecordDecl>(args[0].getAsType()->getAsCXXRecordDecl()->getDeclContext()))
                                return make<>(parent->getQualifiedNameAsString()).first;
                        return make<>(args[0].getAsType()->getAsCXXRecordDecl()->getQualifiedNameAsString()).first;
                    }
        }
        return make<>(d->getQualifiedNameAsString()).first;
    }
    if(auto const pt = type->getPointeeType(); !pt.isNull())
        return make<>(pt->getAsCXXRecordDecl()->getQualifiedNameAsString() + "*").first;
    return nullptr;
}

std::string Interface::Signature::cleanup(std::string name)
{
    for(std::string::size_type pos = 0; (pos = name.find("> >", pos)) != std::string::npos;)
    {
        name.replace(pos, 3, ">>");
        pos += 2;
    }
    for(std::string::size_type pos = 0; (pos = name.find("_Bool", pos)) != std::string::npos;)
    {
        name.replace(pos, 5, "bool");
        pos += 2;
    }
    return name;
}

void Interface::Signature::write(Output &o, std::string const &docs, std::vector<Signature> const &sign)
{
    o << YAML::Key << "sign";
    o << YAML::BeginSeq;
    for(auto const &s: sign)
    {
        o << YAML::BeginMap;
        o << YAML::Key << "type" << Signature::cleanup(s.type);
        o << YAML::Key << "args";
        o << YAML::BeginSeq;
        for(auto const &a: s.args)
        o << Signature::cleanup(a);
        o << YAML::EndSeq;
        o << YAML::EndMap;
    }
    o << YAML::EndSeq;
    o.docs(docs);
}

void Interface::write(Output &o, Type const &t, Type::Format f) const
{
    if(Type::FULL == f)
    {
        o << YAML::BeginMap;
        o << YAML::Key << "interface";
        o << YAML::Value << t.name;
        o.docs(t.name);

        for(auto const &[n, m]: std::vector<std::pair<char const *, decltype(&methods)>>{
            {"methods", &methods},
            {"statics", &statics}
        })
        {
            if(!m->empty())
            {
                o << YAML::Key << n;
                o << YAML::BeginMap;
                for(auto const &[name, sign]: *m)
                {
                    o << YAML::Key << name;
                    o << YAML::BeginMap;
                    Signature::write(o, t.name + "." + name, sign);
                    o << YAML::EndMap;
                }
                o << YAML::EndMap;
            }
        }
        o << YAML::EndMap;
    }
    else Type::Detail::write(o, t, f);
}

void Header::dump(std::optional<std::filesystem::path> &&file)
{
    Output::Docs docs;
    if(file) docs = Output::parse(*file);

    Output yaml(std::move(docs));
    for(auto const &[_, t]: types)
    {
        t->write(yaml, Type::FULL);
    }
    for(auto const &[name, sign]: fns)
    {
        yaml << YAML::BeginMap;
        yaml << YAML::Key << "function" << name;
        Interface::Signature::write(yaml, name, sign);
        yaml << YAML::EndMap;
    }

    if(file)
    {
        std::ofstream stream{*file};
        yaml.flush(stream);
    }
    else
    {
        yaml.flush(std::cout);
    }
}

} // namespace notojs
