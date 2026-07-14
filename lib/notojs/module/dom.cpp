#include <notojs/module/dom.hpp>
#include <notojs/module/dom/node.hpp>
#include <notojs/module/dom/attr.hpp>
#include <notojs/module/dom/dom_rect.hpp>
#include <notojs/module/dom/node_list.hpp>

#include <notojs/module/dom/element.hpp>

#include <notojs/module/dom/html_collection.hpp>
#include <notojs/module/dom/dom_string_map.hpp>
#include <notojs/module/dom/dom_token_list.hpp>
#include <notojs/module/dom/named_node_map.hpp>
#include <notojs/module/dom/html_element.hpp>
#include <notojs/module/dom/css_rule_list.hpp>
#include <notojs/module/dom/css_style_properties.hpp>
#include <notojs/module/dom/css_style_sheet.hpp>
#include <notojs/module/dom/html_document.hpp>
#include <notojs/module/dom/xml_document.hpp>

#include <notojs/module/dom/lexbor.hpp>

#include <notojs/global.hpp>
#include <notojs/notojs.hpp>
#include <bridge.hpp>

namespace notojs {
namespace {

class DOMException : public bridge::Exception<DOMException>
{
    int code;
    std::string_view name;
    std::optional<std::string> detail;

    DOMException() = default;

public:
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        obj.set("code", bridge::Number(ctx, code));
        obj.set("message", bridge::String(ctx, name));
        if(detail) obj.set("detail", bridge::String(ctx, *detail));
    }

#define EXCEPTION_TYPE(n, c) \
    static JSValue throw##n(JSContext *ctx, std::optional<std::string> detail = std::nullopt) { \
        DOMException e; \
        e.code = c; \
        e.name = std::string_view{#n}; \
        e.detail = std::move(detail); \
        return throw_(ctx, std::move(e)); \
    }

    EXCEPTION_TYPE(HierarchyRequestError, 3)
    EXCEPTION_TYPE(WrongDocumentError, 4)
    EXCEPTION_TYPE(InvalidCharacterError, 5)
    EXCEPTION_TYPE(NotFoundError, 8)
    EXCEPTION_TYPE(SyntaxError, 12)
    EXCEPTION_TYPE(NamespaceError, 14)
#undef EXCEPTION_TYPE
};

BOOST_FORCEINLINE std::int64_t u64(const char* s) {
    if(!s || *s == '\0') return -1;

    char *end;
    std::int64_t val = std::strtoull(s, &end, 10);
    if(end == s || *end != '\0' || errno == ERANGE)
        return -1;

    return val;
}

#define ELEMENT_ATTRIBUTE_PROPERTY(name)             \
JSValue name(JSContext *ctx) const                   \
{                                                    \
    if(auto value = ref().getAttribute({#name}))     \
        return bridge::String{ctx, *value};          \
    return JS_NULL;                                  \
}                                                    \
void set_##name(JSContext *ctx, bridge::Value value) \
{                                                    \
    ref().setAttribute({#name}, value.toString());   \
}

#include <notojs/module/dom/event.hxx>
#include <notojs/module/dom/collection.hxx>
#include <notojs/module/dom/node_list.hxx>
#include <notojs/module/dom/node.hxx>
#include <notojs/module/dom/attr.hxx>
#include <notojs/module/dom/dom_rect.hxx>
#include <notojs/module/dom/named_node_map.hxx>
#include <notojs/module/dom/text.hxx>
#include <notojs/module/dom/html_collection.hxx>
#include <notojs/module/dom/element.hxx>
#include <notojs/module/dom/document.hxx>
#include <notojs/module/dom/xml_document.hxx>
#include <notojs/module/dom/dom_string_map.hxx>
#include <notojs/module/dom/dom_token_list.hxx>
#include <notojs/module/dom/css_rule.hxx>
#include <notojs/module/dom/css_rule_list.hxx>
#include <notojs/module/dom/css_style_declaration.hxx>
#include <notojs/module/dom/css_style_properties.hxx>
#include <notojs/module/dom/css_style_sheet.hxx>
#include <notojs/module/dom/html_element.hxx>
#include <notojs/module/dom/svg_element.hxx>
#include <notojs/module/dom/html_document.hxx>
#include <notojs/module/dom/attr_impl.hxx>
#include <notojs/module/dom/css_rule_impl.hxx>

#undef ELEMENT_ATTRIBUTE_PROPERTY

JSValue Document::html_0(JSContext *ctx)
{
    static constexpr std::string_view BLANK{"<!DOCTYPE html><html></html>"};

    auto backend = std::make_shared<dom::HTMLBackend>(ctx, JS_NULL, ::lxb_html_document_create());
    lxb_html_document_parse(backend->doc.get(), (lxb_char_t const *)BLANK.data(), BLANK.size());
    return *(backend->self = HTMLDocument::from(ctx, dom::HTMLDocument{backend}));
}

JSValue Document::html_1(JSContext *ctx, bridge::String data)
{
    auto const &sv = static_cast<std::string_view const &>(data);
    auto backend = std::make_shared<dom::HTMLBackend>(ctx, JS_NULL, ::lxb_html_document_create());
    if(auto const result = lxb_html_document_parse(backend->doc.get(), (lxb_char_t const *)sv.data(), sv.size());
        result != LXB_STATUS_OK) return DOMException::throwSyntaxError(ctx);
    return *(backend->self = HTMLDocument::from(ctx, dom::HTMLDocument{backend}));
}

JSValue Document::xml(JSContext *ctx, bridge::String data)
{
    pugi::xml_document doc;
    auto const &sv = static_cast<std::string_view const &>(data);
    if(auto result = doc.load_buffer(sv.data(), sv.size()))
    {
        auto backend = std::make_shared<dom::XMLBackend>(ctx, JS_NULL, std::move(doc));
        return *(backend->self = XMLDocument::from(ctx, dom::XMLDocument{backend}));
    }
    else return DOMException::throwSyntaxError(ctx, result.description());
}

struct Window : bridge::Interface<Window>
{
    struct Config : bridge::Struct<Config>
    {
        BRIDGE_DEFINE_STRUCT(Config);
        static constexpr auto fields = bridge::fields(
            bridge::field<bridge::Boolean>("notebook")
        );
    };

    JSValue attach_0(JSValue self, JSContext *ctx)
    {
        JSValue glob = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, glob, "window", JS_DupValue(ctx, self));
        JS_SetPropertyStr(ctx, glob, "document", Document::html_0(ctx));
        JS_FreeValue(ctx, glob);
        Global::Context::ptr(ctx)->cleanup.insert("window");
        Global::Context::ptr(ctx)->cleanup.insert("document");
        return JS_GetPropertyStr(ctx, glob, "document");
    }

    JSValue attach_1(JSValue self, JSContext *ctx, Config config)
    {
        JSValue glob = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, glob, "window", JS_DupValue(ctx, self));
        JS_SetPropertyStr(ctx, glob, "document", Document::html_0(ctx));
        JS_FreeValue(ctx, glob);
        if(auto notebook = config.get<bridge::Boolean>("notebook"); !notebook || !*notebook)
        {
            Global::Context::ptr(ctx)->cleanup.insert("window");
            Global::Context::ptr(ctx)->cleanup.insert("document");
        }
        return JS_GetPropertyStr(ctx, glob, "document");
    }

    JSValue getComputedStyle(JSContext *ctx, HTMLElement element)
    {
        return CSSStyleProperties::from(ctx, dom::CSSStyleProperties{
            element.ref(), std::in_place_type<dom::CSSStyleProperties::Comp>
        }, bridge::Strong<void>(ctx, element.style(ctx), false));
    }

    using attach = bridge::Function
    <
        &Window::attach_0,
        &Window::attach_1
    >;

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Window::funcs[] = {
    JS_CFUNC_DEF("attach", 1, &Window::attach::invoke),
    JS_CFUNC_DEF("getComputedStyle", 1, &bridge::Function<&Window::getComputedStyle>::invoke)
};

struct DOMParser : bridge::Interface<DOMParser>
{
    JSValue parseFromString(JSContext *ctx, bridge::String string, bridge::String type) const
    {
        auto const &t = static_cast<std::string_view const &>(type);
        if(t == "text/xml" || t == "application/xml")
        {
            return Document::xml(ctx, string);
        }
        if(t == "text/html" || t == "application/xhtml+xml")
        {
            return Document::html_1(ctx, string);
        }
        return JS_ThrowTypeError(ctx, "Unsupported mime type %s", t.data());
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const DOMParser::funcs[] = {
    JS_CFUNC_DEF("parseFromString", 1, &bridge::Function<&DOMParser::parseFromString>::invoke),
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Window::init(ctx);
    DOMException::init(ctx);
    DOMParser::init(ctx, m);

    JSValue window = Window::ctor(ctx);
    JS_SetModuleExport(ctx, m, "window", window);

    Event::init(ctx, window);
    CustomEvent::init(ctx, window);

    NodeList::init(ctx, window);
    Node::init(ctx, window);
    Attr::init(ctx, window);
    Text::init(ctx, window);
    DOMRect::init(ctx, window);
    NamedNodeMap::init(ctx, window);
    HTMLNamedNodeMap::init(ctx);
    HTMLCollection::init(ctx, window);
    Element::init(ctx, window);
    Document::init(ctx, m);
    DOMStringMap::init(ctx, window);
    DOMTokenList::init(ctx, window);
    CSSStyleDeclaration::init(ctx, window);
    CSSStyleProperties::init(ctx, window);
    CSSRule::init(ctx, window);
    CSSRuleList::init(ctx, window);
    CSSStyleSheet::init(ctx, window);
    HTMLElement::init(ctx, window);
    HTMLAnchorElement::init(ctx, window);
    HTMLImageElement::init(ctx, window);
    HTMLLinkElement::init(ctx, window);
    HTMLStyleElement::init(ctx, window);
    SVGElement::init(ctx, window);
    SVGSVGElement::init(ctx, window);
    SVGGraphicsElement::init(ctx, window);
    SVGRectElement::init(ctx, window);
    SVGTextElement::init(ctx, window);
    HTMLDocument::init(ctx, window);
    XMLDocument::init(ctx, window);

    Document::alias(ctx, window, "Document");
    DOMRect::alias(ctx, window, "SVGRect");
    return 0;
}

BOOST_FORCEINLINE JSValue elementFactory(dom::XMLBackend &backend, pugi::xml_node_struct *node)
{
    return Element::from(backend.ctx, dom::Element{backend.shared_from_this(), node});
}

BOOST_FORCEINLINE JSValue elementFactory(dom::HTMLBackend &backend, lxb_dom_node_t *node)
{
    auto element = [&]() {
        return dom::HTMLElement{backend.shared_from_this(), lxb_html_interface_element(node)};
    };
    switch(node->ns)
    {
    case LXB_NS_SVG:
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_SVG:   return SVGSVGElement::from(backend.ctx, element());
        case LXB_TAG_STYLE: return HTMLStyleElement::from(backend.ctx, element());
        default:
            if(auto name = dom::lexbor::get_name(lxb_dom_interface_element(node)); "rect" == name)
                return SVGRectElement::from(backend.ctx, element());
            else if("text" == name)
                return SVGTextElement::from(backend.ctx, element());
            return SVGGraphicsElement::from(backend.ctx, element());
        }
    case LXB_NS_HTML:
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_A:     return HTMLAnchorElement::from(backend.ctx, element());
        case LXB_TAG_LINK:  return HTMLLinkElement::from(backend.ctx, element());
        case LXB_TAG_IMG:   return HTMLImageElement::from(backend.ctx, element());
        case LXB_TAG_STYLE: return HTMLStyleElement::from(backend.ctx, element());
        }
    default:
        return HTMLElement::from(backend.ctx, element());
    }
}

template<typename B, typename P>
BOOST_FORCEINLINE JSValue nodeFactory(B &backend, P *node)
{
    using namespace boost::hana;
    constexpr auto Config = make_map(
        make_pair(type_c<dom::XMLBackend>,
            make_tuple(
                type_c<XMLDocument>,
                type_c<dom::XMLDocument>,
                make_map(
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT>, int_c<pugi::node_document>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_ELEMENT>, int_c<pugi::node_element>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_TEXT>, int_c<pugi::node_pcdata>))
            )),
        make_pair(type_c<dom::HTMLBackend>,
            make_tuple(
                type_c<HTMLDocument>,
                type_c<dom::HTMLDocument>,
                make_map(
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT>, int_c<LXB_DOM_NODE_TYPE_DOCUMENT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_ELEMENT>, int_c<LXB_DOM_NODE_TYPE_ELEMENT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_TEXT>, int_c<LXB_DOM_NODE_TYPE_TEXT>))
            ))
    );

    auto const type = std::invoke([node]{
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            return node->type;
        else
            return pugi::xml_node(node).type();
    });

    constexpr auto C = Config[type_c<B>];
    constexpr auto T = at_c<2>(C);

    using X = typename decltype(+at_c<0>(C))::type;
    using Y = typename decltype(+at_c<1>(C))::type;

    switch(type)
    {
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_DOCUMENT>):
        if(backend.self)
            return JS_DupValue(backend.ctx, *backend.self);
        return *(backend.self = X::from(backend.ctx, Y{std::dynamic_pointer_cast<B>(backend.shared_from_this())}));
    default:
        if(auto it = backend.nodes.find(node); it != std::end(backend.nodes))
            return JS_DupValue(backend.ctx, it->second);
    };

    switch(type)
    {
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_ELEMENT>):
        return backend.nodes[node] = elementFactory(backend, node);
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_TEXT>):
        return backend.nodes[node] = Text::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    default:
        return backend.nodes[node] = Node::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    };
}

} // namespace

JSValue dom::HTMLBackend::make(lxb_dom_node_t *node)
{
    return nodeFactory(*this, node);
}

JSValue dom::XMLBackend::make(pugi::xml_node_struct *node)
{
    return nodeFactory(*this, node);
}

void notojs_init_dom()
{
    Window::init();
    DOMException::init();
    DOMParser::init();
    Event::init();
    CustomEvent::init();

    NodeList::init();
    Node::init();
    Attr::init();
    Text::init();
    DOMRect::init();
    NamedNodeMap::init();
    HTMLNamedNodeMap::init();
    HTMLCollection::init();
    Element::init();
    Document::init();
    DOMStringMap::init();
    DOMTokenList::init();
    CSSStyleDeclaration::init();
    CSSStyleProperties::init();
    CSSRule::init();
    CSSRuleList::init();
    CSSStyleSheet::init();
    HTMLElement::init();
    HTMLAnchorElement::init();
    HTMLImageElement::init();
    HTMLLinkElement::init();
    HTMLStyleElement::init();
    SVGElement::init();
    SVGSVGElement::init();
    SVGGraphicsElement::init();
    SVGRectElement::init();
    SVGTextElement::init();
    HTMLDocument::init();
    XMLDocument::init();
}

void notojs_init_dom(JSRuntime *rt)
{
    Window::init(rt);
    DOMException::init(rt);
    DOMParser::init(rt);
    Event::init(rt);
    CustomEvent::init(rt);

    NodeList::init(rt);
    Node::init(rt);
    Attr::init(rt);
    Text::init(rt);
    DOMRect::init(rt);
    NamedNodeMap::init(rt);
    HTMLNamedNodeMap::init(rt);
    HTMLCollection::init(rt);
    Element::init(rt);
    Document::init(rt);
    DOMStringMap::init(rt);
    DOMTokenList::init(rt);
    CSSStyleDeclaration::init(rt);
    CSSStyleProperties::init(rt);
    CSSRule::init(rt);
    CSSRuleList::init(rt);
    CSSStyleSheet::init(rt);
    HTMLElement::init(rt);
    HTMLAnchorElement::init(rt);
    HTMLImageElement::init(rt);
    HTMLLinkElement::init(rt);
    HTMLStyleElement::init(rt);
    SVGElement::init(rt);
    SVGSVGElement::init(rt);
    SVGGraphicsElement::init(rt);
    SVGRectElement::init(rt);
    SVGTextElement::init(rt);
    HTMLDocument::init(rt);
    XMLDocument::init(rt);
}

void notojs_init_dom(boost::property_tree::ptree const &) {}

JSModuleDef *notojs_init_dom(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "window");
    JS_AddModuleExport(ctx, mod, Document::name());
    JS_AddModuleExport(ctx, mod, DOMParser::name());
    return mod;
}

} // namespace notojs
