#include <notojs/module/dom/dom_token_list.hpp>
#include <bridge.hpp>

namespace notojs::dom {

JSValue DOMTokenList::item(std::int64_t const n, JSValue d) const
{
    std::int64_t j{0};
    if(auto c = getAttribute(attr))
    {
        bool token = false;
        for(size_t i = 0; i < c->size(); i++) switch(c->at(i))
        {
        case 0x20:
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
            token = false;
            break;
        default:
            if(!token)
            {
                if(n == j++)
                {
                    std::size_t k = i + 1;
                    for(; k < c->size(); ++k)
                        if(isspace(c->at(k))) break;
                    return bridge::String(doc->ctx, c->substr(i, k - i));
                }
                token = true;
            }
        }
    }
    return d;
}

std::uint64_t DOMTokenList::length() const
{
    std::uint64_t length{0};
    if(auto c = getAttribute(attr))
    {
        bool token = false;
        for(size_t i = 0; i < c->size(); i++) switch(c->at(i))
        {
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
        case 0x20:
            token = false;
            break;
        default: if(!token) { ++length; token = true; }
        }
    }
    return length;
}

JSValue DOMTokenList::values() const
{
    bridge::Array array{doc->ctx};
    std::int64_t j{0};
    if(auto c = getAttribute(attr))
    {
        bool token = false;
        for(size_t i = 0; i < c->size(); i++) switch(c->at(i))
        {
        case 0x20:
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
            token = false;
            break;
        default:
            if(!token)
            {
                std::size_t k = i + 1;
                for(; k < c->size(); ++k)
                    if(isspace(c->at(k))) break;
                array.append(bridge::String(doc->ctx, c->substr(i, k - i)));
                token = true;
                i = k - 1;
            }
        }
    }
    return array;
}

} // namespace notojs:dom
