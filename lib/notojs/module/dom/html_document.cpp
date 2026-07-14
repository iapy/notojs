#include <notojs/module/dom/html_document.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <iostream>

namespace notojs::dom {

JSValue HTMLDocument::body() const
{
    return dynamic_cast<HTMLBackend*>(doc.get())->make(lxb_dom_interface_node(
        static_cast<lxb_html_document_t *>(*this)->body));
}

JSValue HTMLDocument::createElement(std::string_view const &name)
{
    return dynamic_cast<HTMLBackend*>(doc.get())->make(
        lxb_dom_interface_node(lxb_dom_document_create_element(*this,
            (lxb_char_t const *)name.data(), name.size(), NULL)));
}

JSValue HTMLDocument::createElement(std::string_view const &name, uintptr_t ns)
{
    auto *node = lxb_dom_document_create_element(*this,
        (lxb_char_t const *)name.data(), name.size(), NULL);
    node->node.ns = ns;
    return dynamic_cast<HTMLBackend*>(doc.get())->make(
        lxb_dom_interface_node(node));
}

JSValue HTMLDocument::createTextNode(std::string_view const &text)
{
    return dynamic_cast<HTMLBackend*>(doc.get())->make(
        lxb_dom_interface_node(lxb_dom_document_create_text_node(*this,
            (lxb_char_t const *)text.data(), text.size())));
}

JSValue HTMLDocument::documentElement() const
{
    lxb_dom_document_t *d = *this;
    return dynamic_cast<HTMLBackend*>(doc.get())->make(lxb_dom_interface_node(d->element));
}

JSValue HTMLDocument::getElementById(std::string_view const &value) const
{
    auto *el = lxb_dom_element_by_id(*this, reinterpret_cast<lxb_char_t const *>(value.data()), value.size());
    return el ? dynamic_cast<HTMLBackend*>(doc.get())->make(lxb_dom_interface_node(el)) : JS_NULL;
}

std::uintptr_t HTMLDocument::lookupNS(std::string_view const &n) const
{
    return dynamic_cast<dom::HTMLBackend *>(doc.get())->lookupNS(n);
}

JSValue HTMLDocument::head() const
{
    if(auto *head = lexbor::head(*this))
        return dynamic_cast<HTMLBackend*>(doc.get())->make(head);
    return JS_NULL;
}

void HTMLDocument::title(std::optional<std::string_view> const &value)
{
    if(auto *h = lexbor::head(*this))
    {
        if(value) lexbor::set_text(lexbor::title(h, true), *value);
        else if(auto *t = lexbor::title(h, false)) lexbor::del_text(t);
    }
}

std::optional<std::string> HTMLDocument::title() const
{
    if(auto *h = lexbor::head(*this))
    if(auto *t = lexbor::title(h, false))
        return lexbor::get_text(t);
    return std::nullopt;
}

std::string HTMLDocument::toString() const
{
    std::string data;
    lxb_html_serialize_tree_cb(*this, [](const lxb_char_t* data, size_t len, void* ctx) -> lxb_status_t {
        static_cast<std::string*>(ctx)->append(reinterpret_cast<const char*>(data), len);
        return LXB_STATUS_OK;
    }, &data);
    return data;
}

} // namespace notojs:dom
