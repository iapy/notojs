#pragma once
#include <notojs/module/dom/css_style_sheet_state.hpp>
#include <notojs/module/dom/live.hpp>

#include <lexbor/style/style.h>
#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/dom/dom.h>
#include <lexbor/selectors/selectors.h>

#include <quickjs/quickjs.h>
#include <boost/config.hpp>
#include <pugixml.hpp>

#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cstdint>
#include <memory>
#include <vector>

namespace notojs::dom {

struct Node;
struct Backend : std::enable_shared_from_this<Backend>
{
    BOOST_FORCEINLINE Backend(JSContext *ctx, JSValue val)
    : ctx{ctx}, self{val} {}

    JSContext *ctx;
    std::optional<JSValue> self;
    std::unordered_map<void*, JSValue> child_n;
    std::unordered_map<void*, JSValue> child_e;
    std::unordered_map<void*, JSValue> attributes;

    virtual ~Backend() = default;

    // Node interface
    virtual JSValue appendChild(Node const &, Node const &) = 0;
    virtual JSValue cloneNode(Node const &, bool) = 0;
    virtual JSValue compareDocumentPosition(Node const &, Node const &) = 0;
    virtual bool contains(Node const &, Node const &) = 0;
    virtual JSValue firstChild(Node const &) = 0;
    virtual JSValue getRootNode(Node const &) = 0;
    virtual JSValue hasChildNodes(Node const &) = 0;
    virtual JSValue insertBefore(Node const &, Node const &, Node const &) = 0;
    virtual void insertBeforeVoid(Node const &, Node const &, Node const &) = 0;
    virtual JSValue replaceChild(Node const &, Node const &, Node const &) = 0;
    virtual bool isChild(Node const &, Node const &) = 0;
    virtual JSValue isConnected(Node const &) = 0;
    virtual JSValue isDefaultNamespace(Node const &, std::string_view const &) = 0;
    virtual JSValue isEqualNode(Node const &, Node const &) = 0;
    virtual JSValue lastChild(Node const &) = 0;
    virtual JSValue namespaceURI(Node const &) = 0;
    virtual JSValue nextSibling(Node const &) = 0;
    virtual JSValue nodeName(Node const &) = 0;
    virtual std::uint16_t nodeType(Node const &) = 0;
    virtual JSValue nodeValue(Node const &) = 0;
    virtual void nodeValue(Node const &, std::string_view const &) = 0;
    virtual void normalize(Node const &) = 0;
    virtual JSValue ownerDocument() = 0;
    virtual JSValue parentElement(Node const &) = 0;
    virtual JSValue parentNode(Node const &) = 0;
    virtual JSValue previousSibling(Node const &) = 0;
    virtual JSValue remove(Node const &) = 0;
    virtual JSValue textContent(Node const &) = 0;
    virtual void textContent(Node const &, std::optional<std::string_view> &&) = 0;

    // NodeList interface
    virtual JSValue childNodes(Node const &) = 0;
    virtual std::uint64_t childNodeCount(Node const &) = 0;
    virtual JSValue childNode(Node const &, std::int64_t, JSValue) = 0;

    // HTMLCollection interface
    virtual JSValue childElements(Node const &) = 0;
    virtual JSValue childElement(Node const &, std::int64_t, JSValue) = 0;
    virtual JSValue childElement(Node const &, std::string_view const &) = 0;

    // NodeList interface
    virtual JSValue collection(std::vector<void*> const &) = 0;
    virtual JSValue collection_item(std::vector<void*> const &, std::int64_t, JSValue) = 0;
    virtual JSValue collection_item(std::vector<void*> const &, std::string_view const &, JSValue) = 0;

    // Element interface
    virtual void appendChildVoid(Node const &, Node const &) = 0;
    virtual std::uint64_t childElementCount(Node const &) = 0;
    virtual JSValue closest(Node const &, std::string_view const &, std::optional<std::string> &) = 0;
    virtual Node createTextNode(std::string_view const &) = 0;
    virtual JSValue doctype() = 0;
    virtual std::optional<Node> first(Node const &) = 0;
    virtual JSValue firstElementChild(Node const &) = 0;
    virtual JSValue lastElementChild(Node const &) = 0;
    virtual JSValue *lookup(Node const &) = 0;
    virtual bool matches(Node const &, std::string_view const &, std::optional<std::string> &) = 0;
    virtual JSValue nextElementSibling(Node const &) = 0;
    virtual std::optional<Node> next(Node const &) = 0;
    virtual std::optional<Node> parent(Node const &) = 0;
    virtual JSValue previousElementSibling(Node const &) = 0;
    virtual void removeVoid(Node const &) = 0;

    // DocumentType interface
    virtual JSValue documentTypeName(Node const &) = 0;
    virtual JSValue documentTypePublicId(Node const &) = 0;
    virtual JSValue documentTypeSystemId(Node const &) = 0;

    // CharacterData interface
    virtual std::string_view characterData(Node const &) = 0;

    virtual JSValue querySelector(Node const &, std::string_view const &, std::optional<std::string> &) = 0;
    virtual std::vector<void*> querySelectorAll(Node const &, std::string_view const &, std::optional<std::string> &) = 0;

    virtual void free(Node const &) = 0;
};

struct XMLBackend: Backend
{
    BOOST_FORCEINLINE XMLBackend(JSContext *ctx, JSValue val, pugi::xml_document &&doc)
    : Backend(ctx, val), doc(std::move(doc)) {}

    pugi::xml_document doc;
    std::unordered_map<pugi::xml_node_struct *, JSValue> nodes;
    std::unordered_set<pugi::xml_node_struct *> detached;

    // Node interface
    JSValue appendChild(Node const &, Node const &) override;
    JSValue cloneNode(Node const &, bool) override;
    JSValue compareDocumentPosition(Node const &, Node const &) override;
    bool contains(Node const &, Node const &) override;
    JSValue firstChild(Node const &) override;
    JSValue getRootNode(Node const &) override;
    JSValue hasChildNodes(Node const &) override;
    JSValue insertBefore(Node const &, Node const &, Node const &) override;
    void insertBeforeVoid(Node const &, Node const &, Node const &) override;
    JSValue replaceChild(Node const &, Node const &, Node const &) override;
    bool isChild(Node const &, Node const &) override;
    JSValue isConnected(Node const &) override;
    JSValue isDefaultNamespace(Node const &, std::string_view const &) override;
    JSValue isEqualNode(Node const &, Node const &) override;
    JSValue lastChild(Node const &) override;
    JSValue namespaceURI(Node const &) override;
    JSValue nextSibling(Node const &) override;
    JSValue nodeName(Node const &) override;
    std::uint16_t nodeType(Node const &) override;
    JSValue nodeValue(Node const &) override;
    void nodeValue(Node const &, std::string_view const &) override;
    void normalize(Node const &) override;
    JSValue ownerDocument() override;
    JSValue parentElement(Node const &) override;
    JSValue parentNode(Node const &) override;
    JSValue previousSibling(Node const &) override;
    JSValue remove(Node const &) override;
    JSValue textContent(Node const &) override;
    void textContent(Node const &, std::optional<std::string_view> &&) override;

    // NodeList interface
    JSValue childNodes(Node const &) override;
    std::uint64_t childNodeCount(Node const &) override;
    JSValue childNode(Node const &, std::int64_t, JSValue) override;

    // HTMLCollection interface
    JSValue childElements(Node const &) override;
    JSValue childElement(Node const &, std::int64_t, JSValue) override;
    JSValue childElement(Node const &, std::string_view const &) override;

    // NodeList interface
    JSValue collection(std::vector<void*> const &) override;
    JSValue collection_item(std::vector<void*> const &, std::int64_t, JSValue) override;
    JSValue collection_item(std::vector<void*> const &, std::string_view const &, JSValue) override;

    // Element interface
    void appendChildVoid(Node const &, Node const &) override;
    std::uint64_t childElementCount(Node const &) override;
    Node createTextNode(std::string_view const &) override;
    JSValue closest(Node const &n, std::string_view const &, std::optional<std::string> &) override;
    JSValue doctype() override;
    std::optional<Node> first(Node const &) override;
    JSValue firstElementChild(Node const &) override;
    JSValue lastElementChild(Node const &) override;
    JSValue *lookup(Node const &) override;
    bool matches(Node const &, std::string_view const &, std::optional<std::string> &) override;
    JSValue nextElementSibling(Node const &) override;
    std::optional<Node> next(Node const &) override;
    std::optional<Node> parent(Node const &) override;
    JSValue previousElementSibling(Node const &) override;
    void removeVoid(Node const &) override;

    // CharacterData interface
    std::string_view characterData(Node const &) override;

    // DocumentType interface
    JSValue documentTypeName(Node const &) override;
    JSValue documentTypePublicId(Node const &) override;
    JSValue documentTypeSystemId(Node const &) override;

    void free(Node const &) override;
    JSValue make(pugi::xml_node_struct *);

    // Detached nodes support
    bool is_detached(pugi::xml_node const &) const;
    bool is_connected(pugi::xml_node const &) const;

    pugi::xml_node logical_root(pugi::xml_node const &) const;

    void mark_detached(pugi::xml_node const &);
    void mark_attached(pugi::xml_node const &);

    pugi::xml_node first_child(pugi::xml_node const &) const;
    pugi::xml_node last_child(pugi::xml_node const &) const;
    pugi::xml_node next_sibling(pugi::xml_node const &) const;
    pugi::xml_node previous_sibling(pugi::xml_node const &) const;

    JSValue querySelector(Node const &, std::string_view const &, std::optional<std::string> &) override;
    std::vector<void*> querySelectorAll(Node const &, std::string_view const &, std::optional<std::string> &) override;
};

struct HTMLBackend: Backend
{
    HTMLBackend(JSContext *ctx, JSValue val, lxb_html_document_t *doc);

    struct Deleter : CSSStyleSheetState::Deleter
    {
        BOOST_FORCEINLINE void operator () (lxb_html_document_t *obj)
        {
            lxb_style_destroy(lxb_html_interface_document(obj));
            lxb_html_document_destroy(obj);
        }
        BOOST_FORCEINLINE void operator () (lxb_selectors_t *sel) const
        {
            lxb_selectors_destroy(sel, true);
        }
    };

    std::unique_ptr<lxb_html_document_t, Deleter> doc;
    std::unordered_map<lxb_dom_node_t *, JSValue> nodes;
    std::unordered_map<lxb_html_element_t *, JSValue>
        cssrules,
        datasets,
        classes,
        styles,
        sheets,
        rels;

    std::unordered_map<lxb_html_element_t *, CSSStyleSheetState> states;
    std::unordered_map<std::uint64_t, JSValue> rules;
    std::uint64_t next_rule_id{1};
    LiveStyle::Map csss, comp;
    LiveCollection::Map live;

    // Node interface
    JSValue appendChild(Node const &, Node const &) override;
    JSValue cloneNode(Node const &, bool) override;
    JSValue compareDocumentPosition(Node const &, Node const &) override;
    bool contains(Node const &, Node const &) override;
    JSValue firstChild(Node const &) override;
    JSValue getRootNode(Node const &) override;
    JSValue hasChildNodes(Node const &) override;
    JSValue insertBefore(Node const &, Node const &, Node const &) override;
    void insertBeforeVoid(Node const &, Node const &, Node const &) override;
    JSValue replaceChild(Node const &, Node const &, Node const &) override;
    bool isChild(Node const &, Node const &) override;
    JSValue isConnected(Node const &) override;
    JSValue isDefaultNamespace(Node const &, std::string_view const &) override;
    JSValue isEqualNode(Node const &, Node const &) override;
    JSValue lastChild(Node const &) override;
    JSValue namespaceURI(Node const &) override;
    JSValue nextSibling(Node const &) override;
    JSValue nodeName(Node const &) override;
    std::uint16_t nodeType(Node const &) override;
    JSValue nodeValue(Node const &) override;
    void nodeValue(Node const &, std::string_view const &) override;
    void normalize(Node const &) override;
    JSValue ownerDocument() override;
    JSValue parentElement(Node const &) override;
    JSValue parentNode(Node const &) override;
    JSValue previousSibling(Node const &) override;
    JSValue remove(Node const &) override;
    JSValue textContent(Node const &) override;
    void textContent(Node const &, std::optional<std::string_view> &&) override;

    // NodeList interface
    JSValue childNodes(Node const &) override;
    std::uint64_t childNodeCount(Node const &) override;
    JSValue childNode(Node const &, std::int64_t, JSValue) override;

    // HTMLCollection interface
    JSValue childElements(Node const &) override;
    JSValue childElement(Node const &, std::int64_t, JSValue) override;
    JSValue childElement(Node const &, std::string_view const &) override;

    // NodeList interface
    JSValue collection(std::vector<void*> const &) override;
    JSValue collection_item(std::vector<void*> const &, std::int64_t, JSValue) override;
    JSValue collection_item(std::vector<void*> const &, std::string_view const &, JSValue) override;

    // Element interface
    void appendChildVoid(Node const &, Node const &) override;
    std::uint64_t childElementCount(Node const &) override;
    JSValue closest(Node const &n, std::string_view const &, std::optional<std::string> &) override;
    Node createTextNode(std::string_view const &) override;
    JSValue doctype() override;
    std::optional<Node> first(Node const &) override;
    JSValue firstElementChild(Node const &) override;
    JSValue lastElementChild(Node const &) override;
    JSValue *lookup(Node const &) override;
    bool matches(Node const &, std::string_view const &, std::optional<std::string> &) override;
    JSValue nextElementSibling(Node const &) override;
    std::optional<Node> next(Node const &) override;
    std::optional<Node> parent(Node const &) override;
    JSValue previousElementSibling(Node const &) override;
    void removeVoid(Node const &) override;

    // DocumentType interface
    JSValue documentTypeName(Node const &) override;
    JSValue documentTypePublicId(Node const &) override;
    JSValue documentTypeSystemId(Node const &) override;

    // CharacterData interface
    std::string_view characterData(Node const &) override;

    std::uintptr_t lookupNS(std::string_view const &) const;
    std::optional<std::string_view> lookupNS(std::uintptr_t) const;

    void free(Node const &) override;
    JSValue make(lxb_dom_node_t *);

    lxb_dom_node_t *createElement(char const *name, std::size_t size) const;

    JSValue querySelector(Node const &, std::string_view const &, std::optional<std::string> &) override;
    std::vector<void*> querySelectorAll(Node const &, std::string_view const &, std::optional<std::string> &) override;
};

} // namespace notojs:dom
