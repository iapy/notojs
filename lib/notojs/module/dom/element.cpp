#include <notojs/module/dom/element.hpp>
#include <sstream>

namespace notojs::dom {

std::vector<Attr::Name> Element::attrs() const
{
    pugi::xml_node nn{*this};
    std::vector<Attr::Name> result;
    for(auto it = std::begin(nn.attributes()); it != std::end(nn.attributes()); ++it)
        result.emplace_back(Attr::Name{it->name()});
    return result;
}

std::uint64_t Element::attributesCount() const
{
    pugi::xml_node nn{*this};
    return std::distance(std::begin(nn.attributes()), std::end(nn.attributes()));
}

void Element::clear()
{
    pugi::xml_node nn{*this};
    while(nn.first_child()) nn.root().append_move(nn.first_child());
}

std::optional<std::string_view> Element::lookupNS(uintptr_t) const
{
    return std::nullopt;
}

std::optional<Attr::Name> Element::getAttributeName(std::int64_t i) const
{
    pugi::xml_node nn{*this};
    for(auto it = std::begin(nn.attributes()); it != std::end(nn.attributes()); ++it)
        if(!i--) return Attr::Name{it->name()};
    return std::nullopt;
}

void Element::removeAttribute(Attr::Name::View const &a)
{
    pugi::xml_node{*this}.remove_attribute(a.name);
}

bool Element::toggleAttribute(Attr::Name::View const &a)
{
    if(hasAttribute(a)) return removeAttribute(a), false;
    else return setAttribute(a, ""), true;
}

bool Element::toggleAttribute(Attr::Name::View const &a, bool force)
{
    if(force) return setAttribute(a, ""), true;
    else return removeAttribute(a), false;
}

std::optional<std::string_view> Element::getAttribute(Attr::Name::View const &a) const
{
    if(auto e = pugi::xml_node{*this}.attribute(a.name); !e) return std::nullopt;
    else return std::string_view{e.value()};
}

bool Element::hasAttribute(Attr::Name::View const &a) const
{
    return !!pugi::xml_node{*this}.attribute(a.name);
}

void Element::setAttribute(Attr::Name::View const &a, std::string_view const &v)
{
    pugi::xml_node nn{*this};
    if(auto e = nn.attribute(a.name); e) e.set_value(v);
    else nn.append_attribute(a.name).set_value(v);
}

std::string Element::toString() const
{
    std::ostringstream data;
    pugi::xml_node{*this}.print(data, " ", pugi::format_default);
    std::string str = std::move(data.str());

    size_t pos = 0;
    while((pos = str.find(" />", pos)) != std::string::npos) {
        str.replace(pos, 3, "/>");
        pos += 2;
    }

    return str;
}

bool Element::isTagName(std::string_view const &sv)
{
    if(sv.empty()) return false;
    if(sv == "*") return true;

    for(std::size_t i = 0; i < sv.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(sv[i]);

        if(c == ' ') return false;
        if(c == ':' && i == 0) return false;

        if(!std::isalnum(c) && c != ':' && c != '-' && c != '_')
            return false;
    }

    return true;
}

} // namespace notojs:dom
