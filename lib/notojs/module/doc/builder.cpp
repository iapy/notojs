#include "yaml-cpp/node/parse.h"
#include <notojs/module/doc/data.hpp>
#include <boost/program_options.hpp>
#include <string>
#include <variant>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>

struct Pad
{
    std::size_t n{0};
};

Pad pad(std::size_t n) { return Pad{n}; }

std::ostream &operator << (std::ostream &stream, Pad const &pad)
{
    for(std::size_t i = 0; i < pad.n; ++i) stream << "    ";
    return stream;
};

std::filesystem::path common_path_prefix(std::vector<std::filesystem::path> const &paths)
{
    if(paths.empty()) return {};

    auto prefix = paths.front().parent_path();
    for(auto const &path: paths)
    {
        auto const parent = path.parent_path();
        auto prefix_it = prefix.begin();
        auto parent_it = parent.begin();
        std::filesystem::path common;

        while(prefix_it != prefix.end() && parent_it != parent.end() && *prefix_it == *parent_it)
        {
            common /= *prefix_it;
            ++prefix_it;
            ++parent_it;
        }

        prefix = std::move(common);
        if(prefix.empty()) break;
    }

    return prefix;
}

void parse_type(YAML::Node const &node, std::string &sign)
{
    if(node.IsScalar())
    {
        sign.append(node.as<std::string>());
    }
    else
    {
        std::size_t i = 0;
        auto it = node.begin();
        auto const p = it->second;
        if("Tail" == it->first.as<std::string>())
        {
            int n = 0;
            std::string name;
            for(auto const &u: p)
            {
                try {
                    n = u.as<int>();
                    continue;
                } catch(YAML::BadConversion const &) {}
                if(++i > 1) name.append(", ");
                parse_type(u, name);
            }
            if(i > 1) sign.append("Any[");
            sign.append(name);
            if(i > 1) sign.append("]");

            if(n == 1)
            {
                sign.append(", ");
                if(i > 1) sign.append("Any[");
                sign.append(name);
                if(i > 1) sign.append("]");
            }
            sign.append("...");
        }
        else
        {
            sign.append(it->first.as<std::string>());
            sign.append("[");

            auto const p = it->second;
            for(auto const &u: p)
            {
                if(++i > 1) sign.append(", ");
                parse_type(u, sign);
            }
            sign.append("]");
        }
    }
};

void parse_func(YAML::Node const &node, std::string &sign)
{
    sign.append("(");
    for(std::size_t i = 0; i < node.size(); ++i)
    {
        if(sign.size() > 1) sign.append(", ");
        parse_type(node[i], sign);
        sign.append(" arg");
        sign.append(std::to_string(i));
    }
    sign.append(")");
};

void parse_docs(YAML::Node const &text, YAML::Node const &node, notojs::doc::Module::Doc &doc)
{
    doc.text = text.as<std::string>();
    if(auto a = node["also"]; a)
        for(auto const &x: a) doc.also.push_back(x.as<std::string>());
}

template<typename T>
void parse_file(std::ostream &ofs, T &mod, std::filesystem::path const &name, std::vector<YAML::Node> &&node, std::vector<std::size_t> &m)
{
    static constexpr std::string_view MDN{"https://developer.mozilla.org/"};
    auto const write_docs = [&ofs](auto p, auto const &d) -> std::ostream & {
        ofs << pad(p) << ".doc = {\n";
        ofs << pad(p + 1) << ".text = R\"MARKDOWN(" << d.text  << ")MARKDOWN\",\n";
        ofs << pad(p + 1) << ".also = {";
        for(std::size_t i = 0; i < d.also.size(); ++i)
        {
            if(i) ofs << ", ";
            ofs << "R\"TEXT(";
            if(auto const &a = d.also[i]; MDN == a.substr(0, MDN.size()))
            {
                ofs << "[MDN:" << a.substr(a.rfind('/') + 1) << "](" << a << ")";
            }
            else
            {
                ofs << "`doc('" << a << "')`";
            }
            ofs << ")TEXT\"";
        }
        ofs << "}\n";
        ofs << pad(p) << "}";
        return ofs;
    };

    for(auto const &doc: node)
    {
        if(doc.IsScalar())
        {
            mod.doc.text = doc.as<std::string>();
        }
        else if constexpr(std::is_same_v<T, notojs::doc::Module>)
        {
            auto t = doc["text"];
            if(auto e = doc["exception"]; e && t)
            {
                parse_docs(t, doc, mod.exceptions[e.as<std::string>()].doc);
            }
            else if(auto e = doc["class"]; e && t)
            {
                auto &clazz = mod.classes[e.as<std::string>()];
                parse_docs(t, doc, clazz.doc);
                if(auto constructor = doc["ctor"])
                {
                    auto t = constructor["text"];
                    auto a = constructor["args"];
                    if(t && a)
                    {
                        auto &ctor = clazz.constructor.emplace();
                        parse_docs(t, constructor, ctor.doc);
                        for(auto const &args: a)
                            parse_func(args, ctor.signatures.emplace_back());
                    }
                }
                for(auto const &i: doc["impl"])
                {
                    clazz.implements.insert(i.as<std::string>());
                }
                if(auto extends = doc["base"])
                {
                    clazz.base = extends.as<std::string>();
                }
                for(auto const &d: doc["defs"])
                {
                    auto t = d["text"];
                    if(auto u = d["type"]; t && u)
                    {
                        auto &type = clazz.types[u.as<std::string>()];
                        type.data = d["base"].as<std::string>();
                        parse_docs(t, d, type.doc);
                    }
                    else if(auto s = d["struct"]; t && s)
                    {
                        auto &type = clazz.types[s.as<std::string>()];
                        auto &fields = type.data.template emplace<1>();
                        for(auto it = d["fields"].begin(); it != d["fields"].end(); ++it)
                        {
                            auto &field = fields[it->first.as<std::string>()];
                            if(auto u = it->second["text"]; u) parse_docs(u, it->second, field.doc);
                            if(auto t = it->second["type"]; t) parse_type(t, field.type);
                        }
                        parse_docs(t, d, type.doc);
                    }
                }
                if(auto methods = doc["methods"])
                {
                    for(auto it = methods.begin(); it != methods.end(); ++it)
                    {
                        auto t = it->second["text"];
                        auto a = it->second["args"];
                        if(t && a && !t.IsNull())
                        {
                            auto &method = clazz.methods[it->first.as<std::string>()];
                            parse_docs(t, it->second, method.doc);
                            for(auto const &args: a)
                                parse_func(args, method.signatures.emplace_back());
                        }
                    }
                }
                if(auto properties = doc["properties"])
                {
                    for(auto it = properties.begin(); it != properties.end(); ++it)
                    {
                        auto &prop = clazz.properties[it->first.as<std::string>()];
                        if(auto u = it->second["text"]) parse_docs(u, it->second, prop.doc);
                        if(auto g = it->second["getter"]) prop.getter = g.as<bool>();
                        for(auto const &s: it->second["setter"])
                            parse_type(s, prop.setter.emplace_back());
                    }
                }
                if(auto methods = doc["statics"])
                {
                    for(auto it = methods.begin(); it != methods.end(); ++it)
                    {
                        auto t = it->second["text"];
                        auto a = it->second["args"];
                        if(t && a)
                        {
                            auto &method = clazz.statics[it->first.as<std::string>()];
                            parse_docs(t, it->second, method.doc);
                            for(auto const &args: a)
                                parse_func(args, method.signatures.emplace_back());
                        }
                    }
                }
            }
            else if(auto e = doc["function"]; e && t)
            {
                if(auto a = doc["args"])
                {
                    auto &function = mod.functions[e.as<std::string>()];
                    parse_docs(t, doc, function.doc);
                    for(auto const &args: a)
                        parse_func(args, function.signatures.emplace_back());
                }
            }
            else parse_docs(t, doc, mod.doc);
        }
        else
        {
            auto t = doc["text"];
            if(auto e = doc["function"]; e && t)
            {
                if(auto a = doc["args"])
                {
                    auto &function = mod.functions[e.as<std::string>()];
                    parse_docs(t, doc, function.doc);
                    for(auto const &args: a)
                        parse_func(args, function.signatures.emplace_back());
                }
            }
            else if(auto e = doc["struct"]; e && t)
            {
                auto &type = mod.types[e.as<std::string>()];
                auto &fields = type.data.template emplace<1>();
                for(auto it = doc["fields"].begin(); it != doc["fields"].end(); ++it)
                {
                    auto &field = fields[it->first.as<std::string>()];
                    if(auto u = it->second["text"]; u) parse_docs(u, it->second, field.doc);
                    if(auto t = it->second["type"]; t) parse_type(t, field.type);
                }
                parse_docs(t, doc, type.doc);
            }
            else parse_docs(t, doc, mod.doc);
        }
    }

    ofs << pad(2) << "{" << name << ", {\n";

    if constexpr(std::is_same_v<T, notojs::doc::Module>)
    {
        m.push_back(0);
        ofs << pad(3) << ".exceptions = {\n";
        for(auto const &[n, e]: mod.exceptions)
        {
            ofs << pad(4) << "{\"" << n << "\", {\n";
            write_docs(5, e.doc) << '\n';
            ofs << pad(4) << "}}";
            if(++m.back() != mod.exceptions.size()) ofs << ","; ofs << "\n";
        }
        ofs << pad(3) << "},\n";
        m.pop_back();
    }
    else
    {
        m.push_back(0);
        ofs << pad(3) << ".types = {\n";
        for(auto const &[n, d]: mod.types)
        {
            ofs << pad(4) << "{\"" << n << "\", {\n";
            write_docs(5, d.doc) << ",\n";
            if(std::holds_alternative<std::string>(d.data))
            {
                ofs << pad(6) << ".data = \"" << std::get<std::string>(d.data) << "\"\n";
            }
            else
            {
                m.push_back(0);
                ofs << pad(6) << ".data = std::map<std::string, Module::Field>{\n";
                for(auto const &[n, f]: std::get<1>(d.data))
                {
                    ofs << pad(7) << "{\"" << n << "\", {\n";
                    write_docs(8, f.doc) << ",\n";
                    ofs << pad(8) << ".type = \"" << f.type << "\"\n";
                    ofs << pad(7) << "}}";
                    if(++m.back() != std::get<1>(d.data).size()) ofs << ","; ofs << "\n";
                }
                ofs << pad(5) << "}\n";
                m.pop_back();
            }
            ofs << pad(4) << "}}";
            if(++m.back() != mod.types.size()) ofs << ","; ofs << "\n";
        }
        ofs << pad(3) << "},\n";
        m.pop_back();
    }
    m.push_back(0);
    ofs << pad(3) << ".functions = {\n";
    for(auto const &[n, f]: mod.functions)
    {
        ofs << pad(4) << "{\"" << n << "\", {\n";
        write_docs(5, f.doc) << ",\n";
        ofs << pad(5) << ".signatures = {\n";
        m.push_back(0);
        for(auto const &s: f.signatures)
        {
            ofs << pad(6) << "\"" << s << "\"";
            if(++m.back() != f.signatures.size()) ofs << ","; ofs << "\n";
        }
        m.pop_back();
        ofs << pad(5) << "}\n";
        ofs << pad(4) << "}}";
        if(++m.back() != mod.functions.size()) ofs << ","; ofs << "\n";
    }
    ofs << pad(3) << "}";
    if constexpr(std::is_same_v<T, notojs::doc::Module>) ofs << ",\n";
    m.pop_back();

    if constexpr(std::is_same_v<T, notojs::doc::Module>)
    {
        m.push_back(0);
        ofs << pad(3) << ".classes = {\n";
        for(auto const &[n, c]: mod.classes)
        {
            ofs << pad(4) << "{\"" << n << "\", {\n";
            write_docs(5, c.doc) << ",\n";
            if(c.base) ofs << pad(5) << ".base = \"" << *c.base << "\",\n";

            m.push_back(0);
            ofs << pad(5) << ".types = {\n";
            for(auto const &[n, d]: c.types)
            {
                ofs << pad(6) << "{\"" << n << "\", {\n";
                write_docs(7, d.doc) << ",\n";
                if(std::holds_alternative<std::string>(d.data))
                {
                    ofs << pad(7) << ".data = \"" << std::get<std::string>(d.data) << "\"\n";
                }
                else
                {
                    m.push_back(0);
                    ofs << pad(7) << ".data = std::map<std::string, Module::Field>{\n";
                    for(auto const &[n, f]: std::get<1>(d.data))
                    {
                        ofs << pad(8) << "{\"" << n << "\", {\n";
                        write_docs(9, f.doc) << ",\n";
                        ofs << pad(9) << ".type = \"" << f.type << "\"\n";
                        ofs << pad(8) << "}}";
                        if(++m.back() != c.types.size()) ofs << ","; ofs << "\n";
                    }
                    ofs << pad(6) << "}\n";
                    m.pop_back();
                }
                ofs << pad(6) << "}}";
                if(++m.back() != c.types.size()) ofs << ","; ofs << "\n";
            }
            ofs << pad(5) << "},\n";
            m.pop_back();

            m.push_back(0);
            ofs << pad(5) << ".implements = {\n";
            for(auto const &impl: c.implements)
            {
                ofs << pad(6) << "\"" << impl << "\"";
                if(++m.back() != c.implements.size()) ofs << ","; ofs << "\n";
            }
            ofs << pad(5) << "},\n";
            m.pop_back();

            if(c.constructor)
            {
                ofs << pad(5) << ".constructor = Module::Function{\n";
                write_docs(6, c.constructor->doc) << ",\n";
                ofs << pad(6) << ".signatures = {\n";
                m.push_back(0);
                for(auto const &s: c.constructor->signatures)
                {
                    ofs << pad(7) << "\"" << s << "\"";
                    if(++m.back() != c.constructor->signatures.size()) ofs << ","; ofs << "\n";
                }
                m.pop_back();
                ofs << pad(6) << "}\n";
                ofs << pad(5) << "},\n";
            }

            m.push_back(0);
            ofs << pad(5) << ".statics = {\n";
            for(auto const &[n, e]: c.statics)
            {
                ofs << pad(6) << "{\"" << n << "\", {\n";
                write_docs(7, e.doc) << ",\n";
                ofs << pad(7) << ".signatures = {\n";
                m.push_back(0);
                for(auto const &s: e.signatures)
                {
                    ofs << pad(8) << "\"" << s << "\"";
                    if(++m.back() != e.signatures.size()) ofs << ","; ofs << "\n";
                }
                m.pop_back();
                ofs << pad(7) << "}\n";
                ofs << pad(6) << "}}";
                if(++m.back() != c.statics.size()) ofs << ","; ofs << "\n";
            }
            ofs << pad(5) << "},\n";
            m.pop_back();

            m.push_back(0);
            ofs << pad(5) << ".methods = {\n";
            for(auto const &[n, e]: c.methods)
            {
                ofs << pad(6) << "{\"" << n << "\", {\n";
                write_docs(7, e.doc) << ",\n";
                ofs << pad(7) << ".signatures = {\n";
                m.push_back(0);
                for(auto const &s: e.signatures)
                {
                    ofs << pad(8) << "\"" << s << "\"";
                    if(++m.back() != e.signatures.size()) ofs << ","; ofs << "\n";
                }
                m.pop_back();
                ofs << pad(7) << "}\n";
                ofs << pad(6) << "}}";
                if(++m.back() != c.methods.size()) ofs << ","; ofs << "\n";
            }
            ofs << pad(5) << "},\n";
            m.pop_back();

            m.push_back(0);
            if(!c.properties.empty())
            {
                ofs << pad(5) << ".properties = {\n";
                for(auto const &[n, p]: c.properties)
                {
                    ofs << pad(6) << "{\"" << n << "\", {\n";
                    write_docs(7, p.doc) << ",\n";
                    ofs << pad(7) << ".getter = " << std::boolalpha << p.getter;
                    if(!p.setter.empty())
                    {
                        ofs << ",\n" << pad(7) << ".setter = {\n";
                        m.push_back(0);
                        for(auto const &s: p.setter)
                        {
                            ofs << pad(8) << "\"" << s << "\"";
                            if(++m.back() != p.setter.size()) ofs << ","; ofs << "\n";
                        }
                        m.pop_back();
                        ofs << pad(7) << "}";
                    }
                    ofs << "\n" << pad(6) << "}}";
                    if(++m.back() != c.properties.size()) ofs << ","; ofs << "\n";
                }
                ofs << pad(5) << "}";
                m.pop_back();
            }

            ofs << "\n" << pad(4) << "}}";
            if(++m.back() != mod.classes.size()) ofs << ","; ofs << "\n";
        }
        ofs << pad(3) << "}";
    }
    ofs << ",\n";
    write_docs(3, mod.doc) << '\n' << pad(2) << "}}";
    m.pop_back();
}

template<>
void parse_file<notojs::doc::Header>(std::ostream &ofs, notojs::doc::Header &hpp, std::filesystem::path const &name, std::vector<YAML::Node> &&node, std::vector<std::size_t> &m)
{
    auto const write_docs = [&ofs](auto p, auto const &d) -> std::ostream & {
        ofs << pad(p) << ".doc = {\n";
        ofs << pad(p + 1) << ".text = R\"MARKDOWN(" << d.text  << ")MARKDOWN\",\n";
        ofs << pad(p + 1) << ".also = {";
        for(std::size_t i = 0; i < d.also.size(); ++i)
        {
            if(i) ofs << ", ";
            ofs << "R\"TEXT(";
            ofs << "`doc('" << d.also[i] << "')`";
            ofs << ")TEXT\"";
        }
        ofs << "}\n";
        ofs << pad(p) << "}";
        return ofs;
    };

    auto const make_signature = [](auto const &n, auto &s, std::string &signature) {
        signature.append(s["type"].template as<std::string>());
        signature.append(" ");
        signature.append(n);
        signature.append("(");
        for(auto const &a: s["args"])
        {
            if('(' != signature.back()) signature.append(",");

            auto s = a.template as<std::string>();
            for(std::string::size_type pos = 0; (pos = s.find(',', pos)) != std::string::npos;)
            {
                s.insert(pos + 1, "\\n    ");
                pos += 6;
            }
            if(s != "JSContext *") signature.append("\\n  ");
            signature.append(s);
        }
        signature.append(")");
    };

    for(auto const &doc: node)
    {
        if(doc.IsScalar())
        {
            hpp.doc.text = doc.as<std::string>();
        }
        else
        {
            auto t = doc["text"];
            if(auto i = doc["interface"]; i && t)
            {
                auto &iface = hpp.interfaces[i.as<std::string>()];
                parse_docs(t, doc, iface.doc);
                if(auto methods = doc["methods"])
                {
                    for(auto it = methods.begin(); it != methods.end(); ++it)
                    {
                        auto &meth = iface.methods[it->first.as<std::string>()];
                        for(auto const &s: it->second["sign"])
                            make_signature(it->first.as<std::string>(), s, meth.signatures.emplace_back());
                        if(auto u = it->second["text"]) parse_docs(u, it->second, meth.doc);
                    }
                }
                auto &check = iface.statics["check"];
                check.signatures.emplace_back("bool check(JSContext *,\\n  JSValue *)");
                check.doc.text = "Returns `true` if `JSValue` holds an instance of `" + i.as<std::string>() + "`.";

                if(auto statics = doc["statics"])
                {
                    for(auto it = statics.begin(); it != statics.end(); ++it)
                    {
                        auto &meth = iface.statics[it->first.as<std::string>()];
                        for(auto const &s: it->second["sign"])
                            make_signature(it->first.as<std::string>(), s, meth.signatures.emplace_back());
                        if(auto u = it->second["text"]) parse_docs(u, it->second, meth.doc);
                    }
                }
            }
            else if(auto f = doc["function"]; f && t)
            {
                auto &func = hpp.functions[f.as<std::string>()];
                for(auto const &s: doc["sign"])
                    make_signature(f.as<std::string>(), s, func.signatures.emplace_back());

                auto t = doc["text"];
                parse_docs(t, doc, func.doc);
            }
            else parse_docs(t, doc, hpp.doc);
        }
    }

    m.push_back(0);
    ofs << pad(2) << "{" << name << ", {\n";
    if(!hpp.interfaces.empty())
    {
        m.push_back(0);
        ofs << pad(3) << ".interfaces = {\n";
        for(auto const &[n, i]: hpp.interfaces)
        {
            ofs << pad(4) << "{\"" << n << "\", {\n";
            if(!i.methods.empty())
            {
                m.push_back(0);
                ofs << pad(5) << ".methods = {\n";
                for(auto const &[n, s]: i.methods)
                {
                    ofs << pad(6) << "{\"" << n << "\", {\n";
                    ofs << pad(7) << ".signatures = {\n";
                    m.push_back(0);
                    for(auto const &q: s.signatures)
                    {
                        ofs << pad(8) << "\"" << q << "\"";
                        if(++m.back() != s.signatures.size()) ofs << ","; ofs << "\n";
                    }
                    m.pop_back();
                    ofs << pad(7) << "},\n";
                    write_docs(7, s.doc) << "\n";
                    ofs << pad(6) << "}}";
                    if(++m.back() != i.methods.size()) ofs << ","; ofs << "\n";
                }
                ofs << pad(5) << "},\n";
                m.pop_back();
            }
            if(!i.statics.empty())
            {
                m.push_back(0);
                ofs << pad(5) << ".statics = {\n";
                for(auto const &[n, s]: i.statics)
                {
                    ofs << pad(6) << "{\"" << n << "\", {\n";
                    ofs << pad(7) << ".signatures = {\n";
                    m.push_back(0);
                    for(auto const &q: s.signatures)
                    {
                        ofs << pad(8) << "\"" << q << "\"";
                        if(++m.back() != s.signatures.size()) ofs << ","; ofs << "\n";
                    }
                    m.pop_back();
                    ofs << pad(7) << "},\n";
                    write_docs(7, s.doc) << "\n";
                    ofs << pad(6) << "}}";
                    if(++m.back() != i.statics.size()) ofs << ","; ofs << "\n";
                }
                ofs << pad(5) << "},\n";
                m.pop_back();
            }
            write_docs(5, i.doc) << ",\n";
            ofs << "\n" << pad(4) << "}}";
            if(++m.back() != hpp.interfaces.size()) ofs << ","; ofs << "\n";
        }
        ofs << pad(3) << "},\n";
        m.pop_back();
    }
    if(!hpp.functions.empty())
    {
        m.push_back(0);
        ofs << pad(3) << ".functions = {\n";
        for(auto const &[n, f]: hpp.functions)
        {
            ofs << pad(4) << "{\"" << n << "\", {\n";
            ofs << pad(5) << ".signatures = {\n";
            m.push_back(0);
            for(auto const &s: f.signatures)
            {
                ofs << pad(6) << "\"" << s << "\"";
                if(++m.back() != f.signatures.size()) ofs << ","; ofs << "\n";
            }
            m.pop_back();
            ofs << pad(5) << "},\n";
            write_docs(5, f.doc) << "\n";
            ofs << pad(4) << "}}";
            if(++m.back() != hpp.functions.size()) ofs << ","; ofs << "\n";
        }
        ofs << pad(3) << "},\n";
        m.pop_back();
    }
    write_docs(3, hpp.doc) << '\n' << pad(2) << "}}";
    m.pop_back();
}

int main(int argc, char **argv)
{
    namespace po = boost::program_options;
    po::options_description desc{"Options"};

    std::filesystem::path output;
    std::vector<std::filesystem::path> input;

    desc.add_options()
        ("output", po::value<std::filesystem::path>(&output)->required())
        ("input",  po::value<std::vector<std::filesystem::path>>(&input));
    ;

    po::positional_options_description p;
    p.add("input", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
        .options(desc)
        .positional(p)
        .run(), vm);
    po::notify(vm);

    std::ofstream ofs(output);
    ofs << "#include <notojs/module/doc/data.hpp>\n\n";
    ofs << "using namespace notojs::doc;\n\n";
    ofs << "extern \"C\" Suite const suite {\n";
    ofs << pad(1) << ".modules = {";

    bool first{true};
    std::vector<std::size_t> m;
    for(auto it = std::begin(input); it != std::end(input);)
    {
        if(it->extension() != ".yml" || (it->parent_path().filename() != "module" && it->parent_path().filename() != "notojs"))
        {
            ++it;
            continue;
        }
        std::cout << "doc:module noto:" << it->stem().string() << '\n';

        if(!std::exchange(first, false)) ofs << ",";
        ofs << "\n";

        notojs::doc::Module mod;
        parse_file(ofs, mod, it->stem(), YAML::LoadAllFromFile(*it), m);
        it = input.erase(it);
    }
    ofs << "\n" << pad(1) << "},\n";
    ofs << pad(1) << ".scripts = {";

    first = true;
    for(auto it = std::begin(input); it != std::end(input);)
    {
        if(it->extension() != ".yml" || it->parent_path().filename() != "script")
        {
            ++it;
            continue;
        }
        std::cout << "doc:script " << it->stem().string() << '\n';

        if(!std::exchange(first, false)) ofs << ",";
        ofs << "\n";

        notojs::doc::Script mod;
        parse_file(ofs, mod, it->stem(), YAML::LoadAllFromFile(*it), m);
        it = input.erase(it);
    }

    ofs << "\n" << pad(1) << "},\n";
    ofs << pad(1) << ".headers = {";

    first = true;
    for(auto it = std::begin(input); it != std::end(input);)
    {
        if(it->extension() != ".yml")
        {
            ++it;
            continue;
        }
        std::cout << "doc:header " << it->stem().replace_extension(".hpp").string() << '\n';

        if(!std::exchange(first, false)) ofs << ",";
        ofs << "\n";

        notojs::doc::Header hpp;
        parse_file(ofs, hpp, it->stem().replace_extension(".hpp"), YAML::LoadAllFromFile(*it), m);
        it = input.erase(it);
    }

    ofs << "\n" << pad(1) << "},\n";
    ofs << pad(1) << ".topics = {";

    first = true;
    decltype(notojs::doc::Suite::topics) topics;
    for(auto const &file: input)
    {
        if(file.extension() != ".md") continue;

        auto const stem = file.stem().string();
        if(auto p = stem.find("-"); p != std::string::npos)
        {
            auto group = stem.substr(0, p);
            auto title = stem.substr(p + 1);
            if(auto it = topics.find(group); it != topics.end())
            {
                if(!it->second.index())
                {
                    auto const old = std::move(std::get<std::string>(it->second));
                    it->second.emplace<1>()[""] = std::move(old);
                }
                std::get<1>(it->second)[title] = file.string();
            }
        }
        else if(auto it = topics.find(stem); it != topics.end())
        {
            std::get<1>(it->second)[""] = file.string();
        }
        else
        {
            topics[stem] = file.string();
        }
    }

    for(auto const &[group, topic]: topics)
    {
        if(topic.index())
        {
            if(!std::exchange(first, false)) ofs << ",";
            ofs << "\n";
            ofs << pad(2) << "{\"" << group << "\", std::map<std::string, std::string>{";

            bool first{true};
            for(auto const &[name, file]: std::get<1>(topic))
            {
                auto const p = std::filesystem::path{file};
                std::cout << "doc:topic  " << p.filename().string() << '\n';
                if(!std::exchange(first, false)) ofs << ",";
                ofs << "\n";
                ofs << pad(3) << "{\"" << name << "\", R\"MARKDOWN(";
                if(0 != std::filesystem::file_size(p))
                    ofs << std::ifstream(p, std::ios::binary).rdbuf();
                ofs << ")MARKDOWN\"}";
            }
            ofs << '\n' << pad(2) << "}}";
        }
        else
        {
            auto const p = std::filesystem::path{std::get<0>(topic)};
            std::cout << "doc:topic  " << p.filename().string() << '\n';

            if(!std::exchange(first, false)) ofs << ",";
            ofs << "\n";
            ofs << pad(2) << "{" << p.stem() << ", R\"MARKDOWN(";
            if(0 != std::filesystem::file_size(p))
                ofs << std::ifstream(p, std::ios::binary).rdbuf();
            ofs << ")MARKDOWN\"}";
        }
    }

    ofs << "\n" << pad(1) << "}";
    ofs << "\n};\n";
    return 0;
}
