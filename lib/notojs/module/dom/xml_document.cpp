#include <notojs/module/dom/xml_document.hpp>
#include <sstream>

namespace notojs::dom {

JSValue XMLDocument::createElement(std::string_view const &name)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    return backend->make(backend->doc.append_child(name).internal_object());
}

JSValue XMLDocument::createTextNode(std::string_view const &text)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(pugi::node_pcdata);
    node.text().set(text);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::documentElement() const
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    return backend->doc
        ? backend->make(backend->doc.document_element().internal_object())
        : JS_NULL;
}

std::string XMLDocument::toString() const
{
    static constexpr std::string_view DOCTYPE{"<?xml version=\"1.0\"?>\n"};

    std::ostringstream data;
    data << DOCTYPE;

    pugi::xml_node{*this}.first_child().print(data, " ", pugi::format_default);
    std::string str = std::move(data.str());

    size_t pos = DOCTYPE.size();
    while((pos = str.find(" />", pos)) != std::string::npos) {
        str.replace(pos, 3, "/>");
        pos += 2;
    }

    return str;
}

} // namespace notojs:dom
