#include <notojs/module/doc/data.hpp>
#include <notojs/module/doc.hpp>
#include <notojs/notojs.hpp>
#include <filesystem>
#include <functional>
#include <mutex>
#include <set>

#include <boost/spirit/home/x3.hpp>
#include <boost/hana.hpp>
#include <dlfcn.h>
#include <variant>
#include <string>

namespace notojs {
namespace {

namespace x3 = boost::spirit::x3;

template<typename T>
struct Document : std::reference_wrapper<T const>
{
    Document() = default;
    Document(T const &ref)
    : std::reference_wrapper<T const>{ref} {}
    Document(T const &ref, std::optional<std::string> &&doc)
    : std::reference_wrapper<T const>{ref}, top{doc} {}
    Document(T const &ref, std::optional<std::string> const &doc)
    : std::reference_wrapper<T const>{ref}, top{doc} {}
    Document(T const &ref, std::optional<std::string> &&doc, std::function<void()> &&ext)
    : std::reference_wrapper<T const>{ref}, top{std::move(doc)}, ext{std::move(ext)} {}
    Document(T const &ref, std::optional<std::string> const &doc, std::function<void()> &&ext)
    : std::reference_wrapper<T const>{ref}, top(doc), ext{std::move(ext)} {}

    std::optional<std::string> top;
    std::function<void()> ext;
};

template<typename T>
Document(T const &) -> Document<T>;

template<typename T>
Document(T const &, std::optional<std::string> &&) -> Document<T>;

template<typename T>
Document(T const &, std::optional<std::string> const &) -> Document<T>;

template<typename T>
Document(T const &, std::optional<std::string> &&, std::function<void()> &&) -> Document<T>;

template<typename T>
Document(T const &, std::optional<std::string> const &, std::function<void()> &&) -> Document<T>;

struct Handler
{
    struct Abstract
    {
        using Header = Document<doc::Header>;
        using Module = Document<doc::Module>;
        using Script = Document<doc::Script>;

        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(std::holds_alternative<Header>(self.target))
            {
                auto &h = std::get<Header>(self.target);
                self.suite = nullptr;
                self.target = Document(h.get().doc, std::move(h.top));
            }
            else if(std::holds_alternative<Module>(self.target))
            {
                auto &m = std::get<Module>(self.target);
                self.suite = nullptr;
                self.target = Document(m.get().doc, std::move(m.top));
            }
            else if(std::holds_alternative<Script>(self.target))
            {
                auto &s = std::get<Script>(self.target);
                self.suite = nullptr;
                self.target = Document(s.get().doc, std::move(s.top));
            }
            else self.target.template emplace<std::monostate>();
        }
    };

    struct API
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(!self.suite) return;

            std::string const attr{std::begin(x3::_attr(ctx)), std::end(x3::_attr(ctx))};
            for(auto const &[name, h]: self.suite->headers)
            {
                if(auto const it = h.interfaces.find(attr); it != std::end(h.interfaces))
                {
                    self.target = Document{it->second, attr};
                    self.defined = name;
                    return;
                }
                if(auto const it = h.functions.find(attr); it != std::end(h.functions))
                {
                    self.target = Document{it->second, attr};
                    self.defined = name;
                    return;
                }
            }
        }
    };

    struct Header
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            std::string const name = x3::_attr(ctx) + ".hpp";
            if(auto it = self.suite->headers.find(name); it != std::end(self.suite->headers))
                self.target = Document{it->second, it->first};
        }
    };

    struct Global
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(auto it = self.suite->modules.find("global"); it != std::end(self.suite->modules))
                self.target = Document{it->second, it->first};
        }
    };

    struct Module
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(auto it = self.suite->modules.find(x3::_attr(ctx)); it != std::end(self.suite->modules))
                self.target = Document{it->second, "noto:" + it->first};
        }
    };

    struct Search
    {
        using Class = Document<doc::Module::Class>;

        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(!self.suite) return;

            std::string const attr{std::begin(x3::_attr(ctx)), std::end(x3::_attr(ctx))};
            if(std::holds_alternative<std::monostate>(self.target))
            {
                if(auto it = self.suite->scripts.find(attr); it != std::end(self.suite->scripts))
                {
                    self.target = Document{it->second, attr};
                }
                else
                {
                    for(auto it = std::begin(self.suite->modules); it != std::end(self.suite->modules); ++it)
                    {
                        if(auto p = it->second.exceptions.find(attr); p != std::end(it->second.exceptions))
                        {
                            self.target = Document{p->second, attr};
                            self.defined = "noto:" + it->first;
                            break;
                        }
                        if(auto p = it->second.classes.find(attr); p != std::end(it->second.classes))
                        {
                            self.target = Document{p->second, attr};
                            self.defined = "noto:" + it->first;
                            break;
                        }
                        if(auto p = it->second.functions.find(attr); p != std::end(it->second.functions))
                        {
                            self.target = Document{p->second, attr};
                            self.defined = "noto:" + it->first;
                            break;
                        }
                    }
                }
                if(std::holds_alternative<std::monostate>(self.target))
                {
                    for(auto it = std::begin(self.suite->scripts); it != std::end(self.suite->scripts); ++it)
                    {
                        if(auto p = it->second.types.find(attr); p != std::end(it->second.types))
                        {
                            self.target = Document{p->second, attr};
                            self.defined = it->first;
                            break;
                        }
                        if(auto p = it->second.functions.find(attr); p != std::end(it->second.functions))
                        {
                            self.target = Document{p->second, attr};
                            self.defined = it->first;
                            break;
                        }
                    }
                }
            }
            else if(std::holds_alternative<Class>(self.target))
            {
                auto &c = std::get<Class>(self.target);
                if(auto z = c.get().types.find(attr); z != std::end(c.get().types))
                {
                    self.target = Document{z->second, *c.top + "." + attr};
                }
                else if(auto y = c.get().methods.find(attr); y != std::end(c.get().methods))
                {
                    self.target = Document{y->second, *c.top + "." + attr};
                }
                else if(auto x = c.get().statics.find(attr); x != std::end(c.get().statics))
                {
                    self.target = Document{x->second, *c.top + "." + attr};
                }
                else if(auto w = c.get().properties.find(attr); w != std::end(c.get().properties))
                {
                    self.target = Document{w->second, *c.top + "." + attr};
                }
                else
                {
                    self.target.template emplace<std::monostate>();
                    self.suite = nullptr;
                }
            }
            else
            {
                self.target.template emplace<std::monostate>();
                self.suite = nullptr;
            }
        }
    };

    struct Topic
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(auto it = self.suite->topics.find(x3::_attr(ctx)); it != std::end(self.suite->topics)) {
                switch(it->second.index())
                {
                case 0:
                    self.target = std::get<0>(it->second);
                    break;
                case 1:
                    self.output = it->first;
                    self.target = std::get<1>(it->second);
                    break;
                }
            }
            else if("builtins" == x3::_attr(ctx))
                Handler::append(self.suite, self.output);
        }
    };

    struct Subtopic
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            if(std::holds_alternative<std::reference_wrapper<std::map<std::string, std::string> const>>(self.target))
            {
                auto const &map = std::get<std::reference_wrapper<std::map<std::string, std::string> const>>(self.target).get();
                if(auto it = map.find(x3::_attr(ctx)); it != std::end(map))
                    self.target = it->second;
                else self.target.template emplace<std::monostate>();
            }
            else self.target.template emplace<std::monostate>();
        }
    };

    struct Flush
    {
        template<typename C>
        void operator () (C &ctx)
        {
            auto &self = x3::_val(ctx);
            std::visit(boost::hana::fix(boost::hana::overload_linearly(
                [](auto, std::monostate const &) {},
                [&self](auto, std::string const &str) {
                    self.output = str;
                },
                [&self](auto, std::map<std::string, std::string> const &map) {
                    std::string topic;
                    std::swap(topic, self.output);
                    for(auto const &[name, text]: map)
                    {
                        if(name.empty())
                        {
                            self.output.append(text);
                            if(1 != map.size()) self.output.append("\n###### See also\n");
                        }
                        else {
                            self.output.append("- `doc('topic:");
                            self.output.append(topic);
                            self.output.append(":");
                            self.output.append(name);
                            self.output.append("')`\n");
                        }
                    }
                },
                [&self](auto fn, Document<doc::Header> const &doc) {
                    if(!doc.get().doc.text.empty() || !doc.get().doc.also.empty())
                    {
                        self.output = "## Abstract{font-style=italic}\n";
                    }
                    self.output.append(doc.get().doc.text);
                    if(!doc.get().doc.also.empty())
                    {
                        self.output.append("\n###### See also\n");
                        for(auto const &a : doc.get().doc.also)
                        {
                            self.output.append("- ");
                            self.output.append(a);
                            self.output.append("\n");
                        }
                    }
                    self.brief = true;
                    if(!doc.get().interfaces.empty())
                    {
                        self.output.append("\n## Interfaces{font-style=italic}");
                        for(auto const &[name, i]: doc.get().interfaces)
                        {
                            self.output.append("\n");
                            fn(Document(i, name));
                        }
                    }
                    if(!doc.get().functions.empty())
                    {
                        self.output.append("\n## Functions{font-style=italic}");
                        for(auto const &[name, f]: doc.get().functions)
                        {
                            self.output.append("\n");
                            fn(Document(f, name));
                        }
                    }
                },
                [&self](auto fn, Document<doc::Script> const &doc) {
                    if(!doc.get().doc.text.empty() || !doc.get().doc.also.empty())
                    {
                        self.output = "## Abstract{font-style=italic}\n";
                    }
                    self.output.append(doc.get().doc.text);
                    if(!doc.get().doc.also.empty())
                    {
                        self.output.append("\n###### See also\n");
                        for(auto const &a : doc.get().doc.also)
                        {
                            self.output.append("- ");
                            self.output.append(a);
                            self.output.append("\n");
                        }
                    }
                    self.brief = true;
                    if(!doc.get().types.empty())
                    {
                        self.output.append("\n### Types{font-style=italic}");
                        for(auto const &[name, e]: doc.get().types)
                        {
                            self.output.append("\n");
                            fn(Document(e, name));
                        }
                    }
                    if(!doc.get().functions.empty())
                    {
                        self.output.append("\n### Functions{font-style=italic}");
                        for(auto const &[name, e]: doc.get().functions)
                        {
                            self.output.append("\n");
                            fn(Document(e, name));
                        }
                    }
                },
                [&self](auto fn, Document<doc::Module> const &doc) {
                    if(!doc.get().doc.text.empty() || !doc.get().doc.also.empty())
                    {
                        self.output = "## Abstract{font-style=italic}\n";
                    }
                    self.output.append(doc.get().doc.text);
                    if(!doc.get().doc.also.empty())
                    {
                        self.output.append("\n###### See also\n");
                        for(auto const &a : doc.get().doc.also)
                        {
                            self.output.append("- ");
                            self.output.append(a);
                            self.output.append("\n");
                        }
                    }
                    self.brief = true;
                    if(!doc.get().exceptions.empty())
                    {
                        self.output.append("\n## Exceptions{font-style=italic}");
                        for(auto const &[name, e]: doc.get().exceptions)
                        {
                            self.output.append("\n");
                            fn(Document(e, name));
                        }
                    }
                    if(!doc.get().classes.empty())
                    {
                        self.output.append("\n## Classes{font-style=italic}");
                        for(auto const &[name, e]: doc.get().classes)
                        {
                            self.output.append("\n");
                            fn(Document(e, name));
                        }
                    }
                    if(!doc.get().functions.empty())
                    {
                        self.output.append("\n## Functions{font-style=italic}");
                        for(auto const &[name, e]: doc.get().functions)
                        {
                            self.output.append("\n");
                            fn(Document(e, name));
                        }
                    }
                },
                [&self](auto, Document<doc::Module::Doc> const &doc) {
                    if(doc.top)
                    {
                        if(self.suite) self.output.append("#");
                        self.output.append("### ");
                        if(auto z = doc.top->find('.'); z == std::string::npos)
                        {
                            self.output.append(*doc.top);
                        }
                        else
                        {
                            self.output.append(doc.top->substr(z ? 0 : 1));
                        }
                        self.output.append("{text-decoration=underline}\n");
                        if(doc.ext && !self.brief)
                        {
                            doc.ext();
                        }
                    }
                    if(self.brief)
                    {
                        self.output.append(doc.get().text.substr(0, doc.get().text.find('\n')));
                    }
                    else
                    {
                        self.output.append(doc.get().text);
                    }
                    if((doc.top && (self.brief || !self.suite)) || !doc.get().also.empty())
                    {
                        self.output.append("\n###### See also\n");
                    }
                    for(auto const &a : doc.get().also)
                    {
                        self.output.append("- ");
                        self.output.append(a);
                        self.output.append("\n");
                    }
                    if(doc.top && (self.brief || !self.suite))
                    {
                        self.output.append("- `doc('");
                        self.output.append(*doc.top);
                        self.output.append("')`\n");
                    }
                },
                [&self](auto fn, Document<doc::Module::Type> const &t) {
                    self.defined_in();
                    fn(Document(t.get().doc, t.top));
                    if(!self.brief)
                    {
                        if(std::holds_alternative<std::string>(t.get().data))
                        {
                            self.output.append("\n\n**`type`**: `");
                            self.output.append(std::get<std::string>(t.get().data));
                            self.output.append("`\n\n");
                        }
                        else
                        {
                            self.output.append("\n\n**`type`**: `Object` with the following optional properties:\n\n");
                            for(auto &[name, type]: std::get<1>(t.get().data))
                            {
                                self.output.append("\n**`");
                                self.output.append(name);
                                self.output.append(": ");
                                self.output.append(type.type);
                                self.output.append("`**{font-size=smaller}\n\n");
                                fn(Document(type.doc));
                                self.output.append("{font-size=smaller margin-left=1em}\n\n");
                            }
                        }
                    }
                },
                [&self](auto fn, Document<doc::Module::Class> const &c) {
                    self.defined_in();
                    fn(Document(c.get().doc, c.top, [&self, &c](){
                        if(c.get().base)
                        {
                            self.output.append("**`extends`** `");
                            self.output.append(*c.get().base);
                            self.output.append("`\n\n");
                        }
                        if(!c.get().implements.empty())
                        {
                            self.output.append("**`implements`** `");
                            for(auto it = std::begin(c.get().implements); it != std::end(c.get().implements); ++it)
                            {
                                if(it != std::begin(c.get().implements)) self.output.append(", ");
                                self.output.append("`");
                                self.output.append(*it);
                                self.output.append("`");
                            }
                            self.output.append("`\n\n");
                        }
                    }));
                    if(!self.brief)
                    {
                        for(auto const &[name, type]: c.get().types)
                        {
                            self.output.append("\n");
                            fn(Document(type, *c.top + "." + name));
                        }
                        if(c.get().constructor)
                        {
                            self.output.append("\n#### constructor{font-style=italic}\n");
                            self.output.append("```\n");
                            for(auto const &sig: c.get().constructor->signatures)
                            {
                                self.output.append(*c.top);
                                self.output.append(sig);
                                self.output.append("\n");
                            }
                            self.output.append("```\n");
                            fn(Document(c.get().constructor->doc));
                        }
                        if(!c.get().properties.empty())
                        {
                            self.output.append("\n#### properties{font-style=italic}");
                            for(auto const &[name, decl]: c.get().properties)
                            {
                                self.output.append("\n");
                                fn(Document(decl, "." + name));
                            }
                        }
                        if(!c.get().methods.empty())
                        {
                            self.output.append("\n#### methods{font-style=italic}");
                            for(auto const &[name, decl]: c.get().methods)
                            {
                                self.output.append("\n");
                                fn(Document(decl, "." + name));
                            }
                        }
                        if(!c.get().statics.empty())
                        {
                            self.output.append("\n#### static methods{font-style=italic}");
                            for(auto const &[name, decl]: c.get().statics)
                            {
                                self.output.append("\n");
                                fn(Document(decl, "." + name));
                            }
                        }
                    }
                },
                [&self](auto fn, Document<doc::Module::Property> const &p) {
                    self.defined_in();
                    if(self.brief || !p.top)
                    {
                        fn(Document(p.get().doc, p.top));
                    }
                    else
                    {
                        self.output.append("#### ");
                        if(auto z = p.top->find('.'); z == std::string::npos)
                        {
                            self.output.append(*p.top);
                        }
                        else
                        {
                            self.output.append(p.top->substr(z ? 0 : 1));
                        }
                        self.output.append("{text-decoration=underline}\n");
                        auto const &f = p.get();
                        self.output.append("```\n");
                        if(f.getter) self.output.append("get()\n");
                        if(!f.setter.empty())
                        {
                            self.output.append("set(");
                            if(f.setter.size() > 1) self.output.append("Any[");
                            for(std::size_t i = 0; i < f.setter.size(); ++i)
                            {
                                self.output.append(f.setter[i]);
                                if(i < f.setter.size() - 1) self.output.append(", ");
                            }
                            if(f.setter.size() > 1) self.output.append("]");
                            self.output.append(")\n");
                        }
                        self.output.append("```\n");
                        fn(Document(f.doc));
                    }
                },
                [&self](auto fn, Document<doc::Module::Function> const &f) {
                    self.defined_in();
                    if(self.brief || !f.top)
                    {
                        fn(Document(f.get().doc, f.top));
                    }
                    else
                    {
                        self.output.append("#### ");
                        if(auto z = f.top->find('.'); z == std::string::npos)
                        {
                            self.output.append(*f.top);
                        }
                        else
                        {
                            self.output.append(f.top->substr(z ? 0 : 1));
                        }
                        self.output.append("{text-decoration=underline}\n");
                        self.output.append("```\n");
                        for(auto const &sig: f.get().signatures)
                        {
                            auto z = f.top->find('.');
                            self.output.append(f.top->substr(z ? 0 : 1));
                            self.output.append(sig);
                            self.output.append("\n");
                        }
                        self.output.append("```\n");
                        fn(Document(f.get().doc));
                    }
                },
                [&self](auto fn, Document<doc::Module::Exception> const &e) {
                    fn(Document(e.get().doc, e.top));
                },
                [&self](auto fn, Document<doc::Header::Interface> const &i) {
                    self.defined_in();
                    fn(Document(i.get().doc, i.top));
                    if(!self.brief)
                    {
                        self.output.append("\n#### constructor{font-style=italic}\n");
                        self.output.append("```\n");
                        self.output.append(*i.top);
                        self.output.append("(JSContext *\n  JSValue)\n```\n");
                        self.output.append("Constructs `");
                        self.output.append(*i.top);
                        self.output.append("` from `JSValue`. Only safe if `");
                        self.output.append(*i.top);
                        self.output.append("::check` returns `true` on that value.");
                        if(!i.get().methods.empty())
                        {
                            self.output.append("\n#### methods{font-style=italic}");
                            for(auto const &[name, decl]: i.get().methods)
                            {
                                self.output.append("\n");
                                fn(Document(decl, name));
                            }
                        }
                        if(!i.get().statics.empty())
                        {
                            self.output.append("\n#### static methods{font-style=italic}");
                            for(auto const &[name, decl]: i.get().statics)
                            {
                                self.output.append("\n");
                                fn(Document(decl, name));
                            }
                        }
                    }
                },
                [&self](auto fn, Document<doc::Header::Function> const &f) {
                    self.defined_in();
                    if(self.brief || !f.top)
                    {
                        fn(Document(f.get().doc, f.top));
                    }
                    else
                    {
                        self.output.append("#### ");
                        self.output.append(*f.top);
                        self.output.append("{text-decoration=underline}\n");
                        self.output.append("```c++\n");
                        for(std::size_t i = 0; i < f.get().signatures.size(); ++i)
                        {
                            if(i) self.output.append("\n");
                            self.output.append(f.get().signatures[i]);
                            self.output.append("\n");
                        }
                        self.output.append("```\n");
                        fn(Document(f.get().doc));
                    }
                }
            )), self.target);
        }
    };

    void defined_in()
    {
        if(!defined.empty())
        {
            if(defined.size() - 4 == defined.find(".hpp"))
                output.append("**`header`** `");
            else
                output.append("**`module`** `");
            output.append(defined);
            output.append("`\n\n");
            defined.clear();
        }
    }

    static void append(doc::Suite const *, std::string &);
    static void append(doc::Suite const *, std::string &, std::string const &);

    doc::Suite const *suite{nullptr};
    std::variant
    <
        std::monostate,
        Document<doc::Header>,
        Document<doc::Module>,
        Document<doc::Script>,
        Document<doc::Module::Doc>,
        Document<doc::Module::Type>,
        Document<doc::Module::Class>,
        Document<doc::Module::Property>,
        Document<doc::Module::Function>,
        Document<doc::Module::Exception>,
        Document<doc::Header::Interface>,
        Document<doc::Header::Function>,
        std::reference_wrapper<std::string const>,
        std::reference_wrapper<std::map<std::string, std::string> const>
    > target;
    std::string output;
    std::string defined;
    bool brief{false};
};

auto const H = (+x3::alnum >> x3::lit(".hpp"))[Handler::Header()] >> -(x3::lit("#abstract")[Handler::Abstract()]);
auto const C = x3::raw[x3::lit("noto::") >> +(x3::char_ - x3::eoi)][Handler::API()];
auto const M = x3::lit("noto:") >> (+x3::alnum)[Handler::Module()] >> -(x3::lit("#abstract")[Handler::Abstract()]);
auto const T = x3::lit("topic:") >> (+x3::alnum)[Handler::Topic()] >> -(x3::lit(":") >> (+x3::alnum)[Handler::Subtopic()]);
auto const Q = (x3::lit("#abstract")[Handler::Abstract()] | -("." >> (+x3::alnum)[Handler::Search()]));
auto const S = (+x3::alnum)[Handler::Search()] >> Q;
auto const D = x3::raw[(x3::lit("$") | x3::lit("console") | x3::lit("require")) >> -("." >> +x3::alnum)][Handler::Search()] >> Q;
auto const Parser = (H | C | D | M | T | S) >> x3::eoi[Handler::Flush()];

auto const parser = std::invoke([]{
    x3::rule<class parser, Handler> const parser = "parser";
    return parser = Parser;
});

struct {
    doc::Suite const *get()
    {
        static std::once_flag flag;
        std::call_once(flag, [this](){
            if(source)
            {
                auto const path = source->string();
                if(void *handle = ::dlopen(path.c_str(), RTLD_NOW))
                    ptr = reinterpret_cast<doc::Suite const*>(::dlsym(handle, "suite"));
            }
        });
        return ptr;
    }
    BOOST_FORCEINLINE JSValue get(JSContext *ctx)
    {
        return source ? bridge::String{ctx, source->string()} : JS_NULL;
    }
    BOOST_FORCEINLINE void configure(boost::property_tree::ptree const &pt)
    {
        if(auto config = pt.get_optional<std::string>("module:doc.suite"))
            source = std::filesystem::path{*config};
    }
private:
    doc::Suite const *ptr = nullptr;
    std::optional<std::filesystem::path> source;
} docs;

void Handler::append(doc::Suite const *suite, std::string &output)
{
    output.append("## Built-in packages\n");
    for(auto const &[k, _]: suite->scripts) append(suite, output, k + "#abstract");
    for(auto const &[k, _]: suite->modules) append(suite, output, "noto:" + k + "#abstract");
}

void Handler::append(doc::Suite const *suite, std::string &output, std::string const &q)
{
    Handler handler{suite};
    x3::parse(std::begin(q), std::end(q), parser, handler);
    output.append(std::begin(handler.output), std::end(handler.output));
}

JSValue doc_0(JSContext *ctx)
{
    std::string output;
    if(auto const *suite = docs.get())
    {
        Handler::append(suite, output, "topic:notojs");
        Handler::append(suite, output, "topic:builtins");
    }

    return output.empty()
        ? __Markdown::data(ctx, bridge::String(ctx, std::string{"#### Suite not installed\n"}))
        : __Markdown::data(ctx, bridge::String(ctx, std::move(output)));
}

JSValue doc_1(JSContext *ctx, bridge::String str)
{
    std::string output;
    if(Handler handler{docs.get()}; handler.suite)
    {
        if(auto const &s = static_cast<std::string_view const &>(str);
            x3::parse(std::begin(s), std::end(s), parser, handler))
                handler.output.swap(output);
    }

    return output.empty()
        ? __Markdown::data(ctx, bridge::String(ctx, std::string{"#### Not found\n"}))
        : __Markdown::data(ctx, bridge::String(ctx, std::move(output)));
}

using docf = bridge::Function<&doc_0, &doc_1>;

JSValue suite(JSContext *ctx)
{
    return docs.get(ctx);
}

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("doc", 1, &docf::invoke),
    JS_CFUNC_DEF("suite", 0, &bridge::Function<suite>::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExport(ctx, m, "default", JS_NewCFunction(ctx, docf::invoke, "doc", 1));
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_doc() {}
void notojs_init_doc(JSRuntime *) {}

void notojs_init_doc(boost::property_tree::ptree const &pt)
{
    docs.configure(pt);
}

JSModuleDef *notojs_init_doc(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "default");
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // namespace notojs
