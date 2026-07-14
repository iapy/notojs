#include <notojs/module/dom/dom_string_map.hpp>
#include <bridge.hpp>
#include <cstring>

namespace notojs::dom {
namespace {

std::string to_snake(char const *n)
{
    std::string result;
    if(n)
    {
        result.append("data-", 5);
        for(;*n; ++n)
        {
            auto const c = static_cast<unsigned char>(*n);
            if(std::isupper(c))
            {
                result.append("-", 1);
                result += static_cast<char>(std::tolower(c));
            }
            else result.append(n, 1);
        }
    }
    return result;
}

std::string to_camel(char const *n, std::size_t len)
{
    std::string result;
    if(n && !std::strncmp(n, "data-", 5))
    {
        bool upper{false};
        for(const char *p = n + 5; p != n + len; ++p)
        {
            if(std::exchange(upper, false))
            {
                if(auto const c = static_cast<unsigned char>(*p); std::islower(c))
                    result += static_cast<char>(std::toupper(c));
                else if(!(upper = ('-' == *p)))
                    result.append(p - 1, 2);
                else
                    result.append(p, 1);
            }
            else if('-' == *p) upper = true;
            else result.append(p, 1);
        }
    }
    return result;
}

} // namespace

Attr::Name DOMStringMap::del_property(char const *name)
{
    Attr::Name n{to_snake(name)};
    removeAttribute(n);
    return n;
}

JSValue DOMStringMap::get_property(char const *name) const
{
    if(auto value = getAttribute({to_snake(name)}))
        return bridge::String{doc->ctx, *value};
    return JS_UNDEFINED;
}

bool DOMStringMap::has_property(char const *name) const
{
    return !!getAttribute({to_snake(name)});
}

std::vector<std::string> DOMStringMap::own_properties() const
{
    std::size_t nlen;
    std::vector<std::string> props;
    for(auto *attr = lxb_dom_element_first_attribute(*this); attr; attr = lxb_dom_element_next_attribute(attr))
    {
        if(attr->node.ns != LXB_NS_HTML) continue;

        const char *a = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
        if(auto name = to_camel(a, nlen); !name.empty())
        {
            props.push_back(std::move(name));
        }
    }
    return props;
}

bool DOMStringMap::own_property(char const *name, JSPropertyDescriptor *desc) const
{
    if(auto value = getAttribute({to_snake(name)}))
    {
        if(desc)
        {
            desc->flags = JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE;
            desc->value = bridge::String{doc->ctx, std::move(*value)};
            desc->getter = JS_UNDEFINED;
            desc->setter = JS_UNDEFINED;
        }
        return true;
    }
    return false;
}

void DOMStringMap::set_property(char const *name, std::string_view const &value)
{
    setAttribute(Attr::Name{to_snake(name)}, value);
}

} // namespace notojs:dom
