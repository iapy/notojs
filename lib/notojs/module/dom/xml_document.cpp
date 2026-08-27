#include <notojs/module/dom/xml_document.hpp>
#include <sstream>

namespace notojs::dom {

JSValue XMLDocument::createCDATASection(std::string_view const &text)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(pugi::node_cdata);
    node.set_value(text.data(), text.size());
    backend->mark_detached(node);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::createComment(std::string_view const &text)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(pugi::node_comment);
    node.set_value(text.data(), text.size());
    backend->mark_detached(node);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::createProcessingInstruction(std::string_view const &target, std::string_view const &data)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(pugi::node_pi);
    node.set_name(target.data(), target.size());
    node.set_value(data.data(), data.size());
    backend->mark_detached(node);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::createElement(std::string_view const &name)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(name);
    backend->mark_detached(node);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::createTextNode(std::string_view const &text)
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    auto node = backend->doc.append_child(pugi::node_pcdata);
    node.text().set(text);
    backend->mark_detached(node);
    return backend->make(node.internal_object());
}

JSValue XMLDocument::documentElement() const
{
    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    for(auto child = backend->first_child(backend->doc); child;
        child = backend->next_sibling(child))
    {
        if(pugi::node_element == child.type()) return backend->make(child.internal_object());
    }
    return JS_NULL;
}

std::string XMLDocument::toString() const
{
    static constexpr std::string_view DOCTYPE{"<?xml version=\"1.0\"?>\n"};

    std::ostringstream data;
    data << DOCTYPE;

    auto *backend = dynamic_cast<XMLBackend*>(doc.get());
    for(auto child = backend->first_child(backend->doc); child;
        child = backend->next_sibling(child))
        child.print(data, " ", pugi::format_default);
    std::string str = std::move(data.str());

    size_t pos = DOCTYPE.size();
    while((pos = str.find(" />", pos)) != std::string::npos) {
        str.replace(pos, 3, "/>");
        pos += 2;
    }

    return str;
}

} // namespace notojs:dom
