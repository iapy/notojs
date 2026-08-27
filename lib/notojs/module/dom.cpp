#include <notojs/module/dom.hpp>
#include <notojs/module/dom/node.hpp>
#include <notojs/module/dom/attr.hpp>
#include <notojs/module/dom/dom_rect.hpp>
#include <notojs/module/dom/node_list.hpp>
#include <notojs/module/dom/svg_length.hpp>

#include <notojs/module/dom/element.hpp>
#include <notojs/module/dom/document_fragment.hpp>

#include <notojs/module/dom/html_collection.hpp>
#include <notojs/module/dom/dom_string_map.hpp>
#include <notojs/module/dom/dom_token_list.hpp>
#include <notojs/module/dom/named_node_map.hpp>
#include <notojs/module/dom/html_element.hpp>
#include <notojs/module/dom/svg_element.hpp>
#include <notojs/module/dom/css_rule.hpp>
#include <notojs/module/dom/css_rule_list.hpp>
#include <notojs/module/dom/css_style_properties.hpp>
#include <notojs/module/dom/css_style_sheet.hpp>
#include <notojs/module/dom/html_document.hpp>
#include <notojs/module/dom/xml_document.hpp>

#include <notojs/module/dom/lexbor.hpp>

#include <notojs/global.hpp>
#include <notojs/notojs.hpp>
#include <bridge.hpp>
#include <cmath>

namespace notojs {
namespace {

class DOMException : public bridge::Exception<DOMException>
{
    int code;
    std::string_view message;
    std::optional<std::string> detail;

    DOMException() = default;

public:
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        obj.set("code", bridge::Number(ctx, code));
        obj.set("message", bridge::String(ctx, message));
        if(detail) obj.set("detail", bridge::String(ctx, *detail));
    }

#define EXCEPTION_TYPE(n, c) \
    static JSValue throw##n(JSContext *ctx, std::optional<std::string> detail = std::nullopt) { \
        DOMException e; \
        e.code = c; \
        e.message = std::string_view{#n}; \
        e.detail = std::move(detail); \
        return throw_(ctx, std::move(e)); \
    }

    EXCEPTION_TYPE(IndexSizeError, 1)
    EXCEPTION_TYPE(HierarchyRequestError, 3)
    EXCEPTION_TYPE(WrongDocumentError, 4)
    EXCEPTION_TYPE(InvalidCharacterError, 5)
    EXCEPTION_TYPE(NoModificationAllowedError, 7)
    EXCEPTION_TYPE(NotFoundError, 8)
    EXCEPTION_TYPE(NotSupportedError, 9)
    EXCEPTION_TYPE(InUseAttributeError, 10)
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

std::uint32_t css_rule_index(bridge::Number const &index)
{
    double value = index.as_double();
    if(!std::isfinite(value) || value == 0) return 0;

    value = std::fmod(std::trunc(value), 4294967296.0);
    if(value < 0) value += 4294967296.0;
    return static_cast<std::uint32_t>(value);
}

#define REFLECTING_ATTRIBUTE(...) BOOST_PP_OVERLOAD(REFLECTING_ATTRIBUTE_, __VA_ARGS__)(__VA_ARGS__)
#define REFLECTING_ATTRIBUTE_1(name) REFLECTING_ATTRIBUTE_2(name, name)
#define REFLECTING_ATTRIBUTE_2(prop, name) JS_CGETSET_DEF(#prop, &bridge::Getter<(&HTMLElement::attribute<HTMLElement::attributes::name>)>, &bridge::Setter<(&HTMLElement::set_attribute<HTMLElement::attributes::name>)>)

#include <notojs/module/dom/event.hxx>
#include <notojs/module/dom/collection.hxx>
#include <notojs/module/dom/node_list.hxx>
#include <notojs/module/dom/node.hxx>
#include <notojs/module/dom/dom_rect.hxx>
#include <notojs/module/dom/attr.hxx>
#include <notojs/module/dom/named_node_map.hxx>
#include <notojs/module/dom/html_collection.hxx>
#include <notojs/module/dom/node_mixin.hxx>
#include <notojs/module/dom/character_data.hxx>
#include <notojs/module/dom/text.hxx>
#include <notojs/module/dom/cdata_section.hxx>
#include <notojs/module/dom/comment.hxx>
#include <notojs/module/dom/processing_instruction.hxx>
#include <notojs/module/dom/document_type.hxx>
#include <notojs/module/dom/document_fragment.hxx>
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

#undef REFLECTING_ATTRIBUTE_2
#undef REFLECTING_ATTRIBUTE_1
#undef REFLECTING_ATTRIBUTE

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

JSValue Document::xml_0(JSContext *ctx, bridge::String data)
{
    pugi::xml_document doc;
    auto const &sv = static_cast<std::string_view const &>(data);
    if(auto result = doc.load_buffer(sv.data(), sv.size(), pugi::parse_default | pugi::parse_comments | pugi::parse_doctype | pugi::parse_pi))
    {
        auto backend = std::make_shared<dom::XMLBackend>(ctx, JS_NULL, std::move(doc));
        return *(backend->self = XMLDocument::from(ctx, dom::XMLDocument{backend}));
    }
    else return DOMException::throwSyntaxError(ctx, result.description());
}

JSValue Document::xml_1(JSContext *ctx, XML xml)
{
    pugi::xml_document doc;
    if(auto data = xml.get<bridge::String>("data"); data)
        return xml_0(ctx, *data);
    return DOMException::throwSyntaxError(ctx, "Invalid SVG fragment");
}

struct Window : bridge::Interface<Window>
{
    JSValue getComputedStyle(JSContext *ctx, HTMLElement element)
    {
        return CSSStyleProperties::from(ctx, dom::CSSStyleProperties{
            element.ref(), std::in_place_type<dom::CSSStyleProperties::Comp>
        }, bridge::Strong<void>(ctx, element.style(ctx), false));
    }

    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const Window::funcs[] = {
    JS_CFUNC_DEF("getComputedStyle", 1, &bridge::Function<&Window::getComputedStyle>::invoke)
};

struct DOMParser : bridge::Interface<DOMParser>
{
    JSValue parseFromString(JSContext *ctx, bridge::String string, bridge::String type) const
    {
        auto const &t = static_cast<std::string_view const &>(type);
        if(t == "text/xml" || t == "application/xml")
        {
            return Document::xml_0(ctx, string);
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

enum class DOMTypeAlias : std::uint8_t
{
  anonym,
  module,
  window
};

template<typename T, DOMTypeAlias A>
struct DOMType
{
    static void init()
    {
        T::init();
    }

    static void init(JSRuntime *rt)
    {
        T::init(rt);
    }

    static void init(JSContext *ctx, JSModuleDef *m, JSValue window)
    {
        if constexpr (A == DOMTypeAlias::anonym)
            T::init(ctx);
        else if constexpr (A == DOMTypeAlias::module)
            T::init(ctx, m);
        else if constexpr (A == DOMTypeAlias::window)
            T::init(ctx, window);
    }
};

template<typename ...Ts>
struct DOMTypes_
{
    static void init()
    {
        (Ts::init(), ...);
    }

    static void init(JSRuntime *rt)
    {
        (Ts::init(rt), ...);
    }

    static void init(JSContext *ctx, JSModuleDef *m, JSValue window)
    {
        (Ts::init(ctx, m, window), ...);
    }
};

using DOMTypes = DOMTypes_<
    DOMType<Event                        , DOMTypeAlias::window>,
    DOMType<CustomEvent                  , DOMTypeAlias::window>,
    DOMType<DOMParser                    , DOMTypeAlias::module>,
    DOMType<NodeList                     , DOMTypeAlias::window>,
    DOMType<Node                         , DOMTypeAlias::window>,
    DOMType<Attr                         , DOMTypeAlias::window>,
    DOMType<CharacterData                , DOMTypeAlias::window>,
    DOMType<Text                         , DOMTypeAlias::window>,
    DOMType<HTMLText                     , DOMTypeAlias::anonym>,
    DOMType<CDATASection                 , DOMTypeAlias::window>,
    DOMType<Comment                      , DOMTypeAlias::window>,
    DOMType<HTMLComment                  , DOMTypeAlias::anonym>,
    DOMType<ProcessingInstruction        , DOMTypeAlias::window>,
    DOMType<HTMLProcessingInstruction    , DOMTypeAlias::anonym>,
    DOMType<DocumentType                 , DOMTypeAlias::window>,
    DOMType<HTMLDocumentType             , DOMTypeAlias::anonym>,
    DOMType<DocumentFragment             , DOMTypeAlias::window>,
    DOMType<DOMRect                      , DOMTypeAlias::window>,
    DOMType<NamedNodeMap                 , DOMTypeAlias::window>,
    DOMType<HTMLNamedNodeMap             , DOMTypeAlias::anonym>,
    DOMType<HTMLCollection               , DOMTypeAlias::window>,
    DOMType<Element                      , DOMTypeAlias::window>,
    DOMType<Document                     , DOMTypeAlias::module>,
    DOMType<DOMStringMap                 , DOMTypeAlias::window>,
    DOMType<DOMTokenList                 , DOMTypeAlias::window>,
    DOMType<CSSStyleDeclaration          , DOMTypeAlias::window>,
    DOMType<CSSStyleProperties           , DOMTypeAlias::window>,
    DOMType<CSSRule                      , DOMTypeAlias::window>,
    DOMType<CSSRuleList                  , DOMTypeAlias::window>,
    DOMType<CSSStyleSheet                , DOMTypeAlias::window>,
    DOMType<HTMLElement                  , DOMTypeAlias::window>,
    DOMType<HTMLAnchorElement            , DOMTypeAlias::window>,
    DOMType<HTMLAreaElement              , DOMTypeAlias::window>,
    DOMType<HTMLBaseElement              , DOMTypeAlias::window>,
    DOMType<HTMLCanvasElement            , DOMTypeAlias::window>,
    DOMType<HTMLDataElement              , DOMTypeAlias::window>,
    DOMType<HTMLDataListElement          , DOMTypeAlias::window>,
    DOMType<HTMLDetailsElement           , DOMTypeAlias::window>,
    DOMType<HTMLDialogElement            , DOMTypeAlias::window>,
    DOMType<HTMLEmbedElement             , DOMTypeAlias::window>,
    DOMType<HTMLFieldSetElement          , DOMTypeAlias::window>,
    DOMType<HTMLHtmlElement              , DOMTypeAlias::window>,
    DOMType<HTMLIFrameElement            , DOMTypeAlias::window>,
    DOMType<HTMLLabelElement             , DOMTypeAlias::window>,
    DOMType<HTMLLegendElement            , DOMTypeAlias::window>,
    DOMType<HTMLLIElement                , DOMTypeAlias::window>,
    DOMType<HTMLMapElement               , DOMTypeAlias::window>,
    DOMType<HTMLMediaElement             , DOMTypeAlias::window>,
    DOMType<HTMLAudioElement             , DOMTypeAlias::window>,
    DOMType<HTMLMenuElement              , DOMTypeAlias::window>,
    DOMType<HTMLMetaElement              , DOMTypeAlias::window>,
    DOMType<HTMLMeterElement             , DOMTypeAlias::window>,
    DOMType<HTMLModElement               , DOMTypeAlias::window>,
    DOMType<HTMLOListElement             , DOMTypeAlias::window>,
    DOMType<HTMLObjectElement            , DOMTypeAlias::window>,
    DOMType<HTMLOptGroupElement          , DOMTypeAlias::window>,
    DOMType<HTMLOptionElement            , DOMTypeAlias::window>,
    DOMType<HTMLOutputElement            , DOMTypeAlias::window>,
    DOMType<HTMLParamElement             , DOMTypeAlias::window>,
    DOMType<HTMLPictureElement           , DOMTypeAlias::window>,
    DOMType<HTMLProgressElement          , DOMTypeAlias::window>,
    DOMType<HTMLQuoteElement             , DOMTypeAlias::window>,
    DOMType<HTMLScriptElement            , DOMTypeAlias::window>,
    DOMType<HTMLSelectElement            , DOMTypeAlias::window>,
    DOMType<HTMLSourceElement            , DOMTypeAlias::window>,
    DOMType<HTMLTableCaptionElement      , DOMTypeAlias::window>,
    DOMType<HTMLTableColElement          , DOMTypeAlias::window>,
    DOMType<HTMLTimeElement              , DOMTypeAlias::window>,
    DOMType<HTMLTitleElement             , DOMTypeAlias::window>,
    DOMType<HTMLTrackElement             , DOMTypeAlias::window>,
    DOMType<HTMLUListElement             , DOMTypeAlias::window>,
    DOMType<HTMLVideoElement             , DOMTypeAlias::window>,
    DOMType<HTMLBRElement                , DOMTypeAlias::window>,
    DOMType<HTMLBodyElement              , DOMTypeAlias::window>,
    DOMType<HTMLButtonElement            , DOMTypeAlias::window>,
    DOMType<HTMLDivElement               , DOMTypeAlias::window>,
    DOMType<HTMLFormElement              , DOMTypeAlias::window>,
    DOMType<HTMLHeadElement              , DOMTypeAlias::window>,
    DOMType<HTMLHeadingElement           , DOMTypeAlias::window>,
    DOMType<HTMLHRElement                , DOMTypeAlias::window>,
    DOMType<HTMLImageElement             , DOMTypeAlias::window>,
    DOMType<HTMLInputElement             , DOMTypeAlias::window>,
    DOMType<HTMLLinkElement              , DOMTypeAlias::window>,
    DOMType<HTMLParagraphElement         , DOMTypeAlias::window>,
    DOMType<HTMLPreElement               , DOMTypeAlias::window>,
    DOMType<HTMLSpanElement              , DOMTypeAlias::window>,
    DOMType<HTMLStyleElement             , DOMTypeAlias::window>,
    DOMType<HTMLTableElement             , DOMTypeAlias::window>,
    DOMType<HTMLTableCellElement         , DOMTypeAlias::window>,
    DOMType<HTMLTableRowElement          , DOMTypeAlias::window>,
    DOMType<HTMLTableSectionElement      , DOMTypeAlias::window>,
    DOMType<HTMLTextAreaElement          , DOMTypeAlias::window>,
    DOMType<SVGElement                   , DOMTypeAlias::window>,
    DOMType<SVGSVGElement                , DOMTypeAlias::window>,
    DOMType<SVGGraphicsElement           , DOMTypeAlias::window>,
    DOMType<SVGAElement                  , DOMTypeAlias::window>,
    DOMType<SVGAnimationElement          , DOMTypeAlias::window>,
    DOMType<SVGAnimateElement            , DOMTypeAlias::window>,
    DOMType<SVGAnimateMotionElement      , DOMTypeAlias::window>,
    DOMType<SVGAnimateTransformElement   , DOMTypeAlias::window>,
    DOMType<SVGCircleElement             , DOMTypeAlias::window>,
    DOMType<SVGClipPathElement           , DOMTypeAlias::window>,
    DOMType<SVGDefsElement               , DOMTypeAlias::window>,
    DOMType<SVGDescElement               , DOMTypeAlias::window>,
    DOMType<SVGEllipseElement            , DOMTypeAlias::window>,
    DOMType<SVGFEBlendElement            , DOMTypeAlias::window>,
    DOMType<SVGFEColorMatrixElement      , DOMTypeAlias::window>,
    DOMType<SVGFEComponentTransferElement, DOMTypeAlias::window>,
    DOMType<SVGFECompositeElement        , DOMTypeAlias::window>,
    DOMType<SVGFEConvolveMatrixElement   , DOMTypeAlias::window>,
    DOMType<SVGFEDiffuseLightingElement  , DOMTypeAlias::window>,
    DOMType<SVGFEDisplacementMapElement  , DOMTypeAlias::window>,
    DOMType<SVGFEDistantLightElement     , DOMTypeAlias::window>,
    DOMType<SVGFEDropShadowElement       , DOMTypeAlias::window>,
    DOMType<SVGFEFloodElement            , DOMTypeAlias::window>,
    DOMType<SVGFEFuncAElement            , DOMTypeAlias::window>,
    DOMType<SVGFEFuncBElement            , DOMTypeAlias::window>,
    DOMType<SVGFEFuncGElement            , DOMTypeAlias::window>,
    DOMType<SVGFEFuncRElement            , DOMTypeAlias::window>,
    DOMType<SVGFEGaussianBlurElement     , DOMTypeAlias::window>,
    DOMType<SVGFEImageElement            , DOMTypeAlias::window>,
    DOMType<SVGFEMergeElement            , DOMTypeAlias::window>,
    DOMType<SVGFEMergeNodeElement        , DOMTypeAlias::window>,
    DOMType<SVGFEMorphologyElement       , DOMTypeAlias::window>,
    DOMType<SVGFEOffsetElement           , DOMTypeAlias::window>,
    DOMType<SVGFEPointLightElement       , DOMTypeAlias::window>,
    DOMType<SVGFESpecularLightingElement , DOMTypeAlias::window>,
    DOMType<SVGFESpotLightElement        , DOMTypeAlias::window>,
    DOMType<SVGFETileElement             , DOMTypeAlias::window>,
    DOMType<SVGFETurbulenceElement       , DOMTypeAlias::window>,
    DOMType<SVGFilterElement             , DOMTypeAlias::window>,
    DOMType<SVGForeignObjectElement      , DOMTypeAlias::window>,
    DOMType<SVGGElement                  , DOMTypeAlias::window>,
    DOMType<SVGImageElement              , DOMTypeAlias::window>,
    DOMType<SVGLength                    , DOMTypeAlias::window>,
    DOMType<SVGLinearGradientElement     , DOMTypeAlias::window>,
    DOMType<SVGLineElement               , DOMTypeAlias::window>,
    DOMType<SVGMarkerElement             , DOMTypeAlias::window>,
    DOMType<SVGMaskElement               , DOMTypeAlias::window>,
    DOMType<SVGMetadataElement           , DOMTypeAlias::window>,
    DOMType<SVGMPathElement              , DOMTypeAlias::window>,
    DOMType<SVGPathElement               , DOMTypeAlias::window>,
    DOMType<SVGPatternElement            , DOMTypeAlias::window>,
    DOMType<SVGPolygonElement            , DOMTypeAlias::window>,
    DOMType<SVGPolylineElement           , DOMTypeAlias::window>,
    DOMType<SVGRadialGradientElement     , DOMTypeAlias::window>,
    DOMType<SVGRectElement               , DOMTypeAlias::window>,
    DOMType<SVGScriptElement             , DOMTypeAlias::window>,
    DOMType<SVGSetElement                , DOMTypeAlias::window>,
    DOMType<SVGSolidColorElement         , DOMTypeAlias::window>,
    DOMType<SVGStopElement               , DOMTypeAlias::window>,
    DOMType<SVGStyleElement              , DOMTypeAlias::window>,
    DOMType<SVGSwitchElement             , DOMTypeAlias::window>,
    DOMType<SVGSymbolElement             , DOMTypeAlias::window>,
    DOMType<SVGTextContentElement        , DOMTypeAlias::window>,
    DOMType<SVGTextElement               , DOMTypeAlias::window>,
    DOMType<SVGTextPathElement           , DOMTypeAlias::window>,
    DOMType<SVGTitleElement              , DOMTypeAlias::window>,
    DOMType<SVGTSpanElement              , DOMTypeAlias::window>,
    DOMType<SVGUseElement                , DOMTypeAlias::window>,
    DOMType<SVGViewElement               , DOMTypeAlias::window>,
    DOMType<HTMLDocument                 , DOMTypeAlias::window>,
    DOMType<XMLDocument                  , DOMTypeAlias::window>
>;

int init(JSContext *ctx, JSModuleDef *m)
{
    Window::init(ctx);
    DOMException::init(ctx, m);

    JSValue window = Window::ctor(ctx);
    JS_SetModuleExport(ctx, m, "window", window);

    DOMTypes::init(ctx, m, window);
    Document::alias(ctx, window, "Document");
    DOMRect::alias(ctx, window, "SVGRect");
    return 0;
}

namespace factory {

BOOST_FORCEINLINE dom::HTMLElement html_element(dom::HTMLBackend &backend, lxb_dom_node_t *node)
{
    return dom::HTMLElement{backend.shared_from_this(), lxb_html_interface_element(node)};
}

template<typename T>
JSValue svg_element(dom::HTMLBackend &backend, lxb_dom_node_t *node)
{
    return T::from(backend.ctx, html_element(backend, node));
}

BOOST_FORCEINLINE JSValue element(dom::HTMLBackend &backend, lxb_dom_node_t *node)
{
    // Lexbor doesn't have enum for SVG elements
    static const auto svg = std::unordered_map<std::string_view, JSValue(*)(dom::HTMLBackend &, lxb_dom_node_t *node)>{
        {"a",                   &svg_element<SVGAElement>},
        {"animate",             &svg_element<SVGAnimateElement>},
        {"animateMotion",       &svg_element<SVGAnimateMotionElement>},
        {"animatemotion",       &svg_element<SVGAnimateMotionElement>},
        {"animateTransform",    &svg_element<SVGAnimateTransformElement>},
        {"animatetransform",    &svg_element<SVGAnimateTransformElement>},
        {"circle",              &svg_element<SVGCircleElement>},
        {"clipPath",            &svg_element<SVGClipPathElement>},
        {"clippath",            &svg_element<SVGClipPathElement>},
        {"defs",                &svg_element<SVGDefsElement>},
        {"desc",                &svg_element<SVGDescElement>},
        {"ellipse",             &svg_element<SVGEllipseElement>},
        {"feBlend",             &svg_element<SVGFEBlendElement>},
        {"feblend",             &svg_element<SVGFEBlendElement>},
        {"feColorMatrix",       &svg_element<SVGFEColorMatrixElement>},
        {"fecolormatrix",       &svg_element<SVGFEColorMatrixElement>},
        {"feComponentTransfer", &svg_element<SVGFEComponentTransferElement>},
        {"fecomponenttransfer", &svg_element<SVGFEComponentTransferElement>},
        {"feComposite",         &svg_element<SVGFECompositeElement>},
        {"fecomposite",         &svg_element<SVGFECompositeElement>},
        {"feConvolveMatrix",    &svg_element<SVGFEConvolveMatrixElement>},
        {"feconvolvematrix",    &svg_element<SVGFEConvolveMatrixElement>},
        {"feDiffuseLighting",   &svg_element<SVGFEDiffuseLightingElement>},
        {"fediffuselighting",   &svg_element<SVGFEDiffuseLightingElement>},
        {"feDisplacementMap",   &svg_element<SVGFEDisplacementMapElement>},
        {"fedisplacementmap",   &svg_element<SVGFEDisplacementMapElement>},
        {"feDistantLight",      &svg_element<SVGFEDistantLightElement>},
        {"fedistantlight",      &svg_element<SVGFEDistantLightElement>},
        {"feDropShadow",        &svg_element<SVGFEDropShadowElement>},
        {"fedropshadow",        &svg_element<SVGFEDropShadowElement>},
        {"feFlood",             &svg_element<SVGFEFloodElement>},
        {"feflood",             &svg_element<SVGFEFloodElement>},
        {"feFuncA",             &svg_element<SVGFEFuncAElement>},
        {"fefunca",             &svg_element<SVGFEFuncAElement>},
        {"feFuncB",             &svg_element<SVGFEFuncBElement>},
        {"fefuncb",             &svg_element<SVGFEFuncBElement>},
        {"feFuncG",             &svg_element<SVGFEFuncGElement>},
        {"fefuncg",             &svg_element<SVGFEFuncGElement>},
        {"feFuncR",             &svg_element<SVGFEFuncRElement>},
        {"fefuncr",             &svg_element<SVGFEFuncRElement>},
        {"feGaussianBlur",      &svg_element<SVGFEGaussianBlurElement>},
        {"fegaussianblur",      &svg_element<SVGFEGaussianBlurElement>},
        {"feImage",             &svg_element<SVGFEImageElement>},
        {"feimage",             &svg_element<SVGFEImageElement>},
        {"feMerge",             &svg_element<SVGFEMergeElement>},
        {"femerge",             &svg_element<SVGFEMergeElement>},
        {"feMergeNode",         &svg_element<SVGFEMergeNodeElement>},
        {"femergenode",         &svg_element<SVGFEMergeNodeElement>},
        {"feMorphology",        &svg_element<SVGFEMorphologyElement>},
        {"femorphology",        &svg_element<SVGFEMorphologyElement>},
        {"feOffset",            &svg_element<SVGFEOffsetElement>},
        {"feoffset",            &svg_element<SVGFEOffsetElement>},
        {"fePointLight",        &svg_element<SVGFEPointLightElement>},
        {"fepointlight",        &svg_element<SVGFEPointLightElement>},
        {"feSpecularLighting",  &svg_element<SVGFESpecularLightingElement>},
        {"fespecularlighting",  &svg_element<SVGFESpecularLightingElement>},
        {"feSpotLight",         &svg_element<SVGFESpotLightElement>},
        {"fespotlight",         &svg_element<SVGFESpotLightElement>},
        {"feTile",              &svg_element<SVGFETileElement>},
        {"fetile",              &svg_element<SVGFETileElement>},
        {"feTurbulence",        &svg_element<SVGFETurbulenceElement>},
        {"feturbulence",        &svg_element<SVGFETurbulenceElement>},
        {"filter",              &svg_element<SVGFilterElement>},
        {"foreignObject",       &svg_element<SVGForeignObjectElement>},
        {"foreignobject",       &svg_element<SVGForeignObjectElement>},
        {"g",                   &svg_element<SVGGElement>},
        {"image",               &svg_element<SVGImageElement>},
        {"line",                &svg_element<SVGLineElement>},
        {"linearGradient",      &svg_element<SVGLinearGradientElement>},
        {"lineargradient",      &svg_element<SVGLinearGradientElement>},
        {"marker",              &svg_element<SVGMarkerElement>},
        {"mask",                &svg_element<SVGMaskElement>},
        {"metadata",            &svg_element<SVGMetadataElement>},
        {"mpath",               &svg_element<SVGMPathElement>},
        {"path",                &svg_element<SVGPathElement>},
        {"pattern",             &svg_element<SVGPatternElement>},
        {"polygon",             &svg_element<SVGPolygonElement>},
        {"polyline",            &svg_element<SVGPolylineElement>},
        {"radialGradient",      &svg_element<SVGRadialGradientElement>},
        {"radialgradient",      &svg_element<SVGRadialGradientElement>},
        {"rect",                &svg_element<SVGRectElement>},
        {"script",              &svg_element<SVGScriptElement>},
        {"set",                 &svg_element<SVGSetElement>},
        {"solidColor",          &svg_element<SVGSolidColorElement>},
        {"solidcolor",          &svg_element<SVGSolidColorElement>},
        {"stop",                &svg_element<SVGStopElement>},
        {"style",               &svg_element<SVGStyleElement>},
        {"switch",              &svg_element<SVGSwitchElement>},
        {"symbol",              &svg_element<SVGSymbolElement>},
        {"text",                &svg_element<SVGTextElement>},
        {"textPath",            &svg_element<SVGTextPathElement>},
        {"textpath",            &svg_element<SVGTextPathElement>},
        {"title",               &svg_element<SVGTitleElement>},
        {"tspan",               &svg_element<SVGTSpanElement>},
        {"use",                 &svg_element<SVGUseElement>},
        {"view",                &svg_element<SVGViewElement>},
    };
    switch(node->ns)
    {
    case LXB_NS_SVG:
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_SVG: return SVGSVGElement::from(backend.ctx, html_element(backend, node));
        default:
            if(auto it = svg.find(dom::lexbor::get_name(lxb_dom_interface_element(node))); it != std::end(svg))
                return it->second(backend, node);
            return SVGElement::from(backend.ctx, html_element(backend, node));
        }
    case LXB_NS_HTML:
        switch(lxb_dom_node_tag_id(node))
        {
        case LXB_TAG_A:        return HTMLAnchorElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_AREA:     return HTMLAreaElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_AUDIO:    return HTMLAudioElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_BASE:     return HTMLBaseElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_BR:       return HTMLBRElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_BODY:     return HTMLBodyElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_BUTTON:   return HTMLButtonElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_CANVAS:   return HTMLCanvasElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_CAPTION:  return HTMLTableCaptionElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_COL:
        case LXB_TAG_COLGROUP: return HTMLTableColElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DATA:     return HTMLDataElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DATALIST: return HTMLDataListElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DEL:
        case LXB_TAG_INS:      return HTMLModElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DETAILS:  return HTMLDetailsElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DIALOG:   return HTMLDialogElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_DIV:      return HTMLDivElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_EMBED:    return HTMLEmbedElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_FIELDSET: return HTMLFieldSetElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_FORM:     return HTMLFormElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_H1:
        case LXB_TAG_H2:
        case LXB_TAG_H3:
        case LXB_TAG_H4:
        case LXB_TAG_H5:
        case LXB_TAG_H6:       return HTMLHeadingElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_HEAD:     return HTMLHeadElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_HR:       return HTMLHRElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_HTML:     return HTMLHtmlElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_IFRAME:   return HTMLIFrameElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_IMG:      return HTMLImageElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_INPUT:    return HTMLInputElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_LABEL:    return HTMLLabelElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_LEGEND:   return HTMLLegendElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_LI:       return HTMLLIElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_LINK:     return HTMLLinkElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_MAP:      return HTMLMapElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_MENU:     return HTMLMenuElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_META:     return HTMLMetaElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_METER:    return HTMLMeterElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_OBJECT:   return HTMLObjectElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_OL:       return HTMLOListElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_OPTGROUP: return HTMLOptGroupElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_OPTION:   return HTMLOptionElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_OUTPUT:   return HTMLOutputElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_P:        return HTMLParagraphElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_PARAM:    return HTMLParamElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_PICTURE:  return HTMLPictureElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_PRE:      return HTMLPreElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_PROGRESS: return HTMLProgressElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_BLOCKQUOTE:
        case LXB_TAG_Q:        return HTMLQuoteElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_SCRIPT:   return HTMLScriptElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_SELECT:   return HTMLSelectElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_SOURCE:   return HTMLSourceElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_SPAN:     return HTMLSpanElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_STYLE:    return HTMLStyleElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TABLE:    return HTMLTableElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TD:
        case LXB_TAG_TH:       return HTMLTableCellElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TBODY:
        case LXB_TAG_TFOOT:
        case LXB_TAG_THEAD:    return HTMLTableSectionElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TEXTAREA: return HTMLTextAreaElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TIME:     return HTMLTimeElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TITLE:    return HTMLTitleElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TR:       return HTMLTableRowElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_TRACK:    return HTMLTrackElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_UL:       return HTMLUListElement::from(backend.ctx, html_element(backend, node));
        case LXB_TAG_VIDEO:    return HTMLVideoElement::from(backend.ctx, html_element(backend, node));
        }
    default:
        return HTMLElement::from(backend.ctx, html_element(backend, node));
    }
}

BOOST_FORCEINLINE JSValue element(dom::XMLBackend &backend, pugi::xml_node_struct *node)
{
    return Element::from(backend.ctx, dom::Element{backend.shared_from_this(), node});
}

template<typename B, typename P>
BOOST_FORCEINLINE JSValue node(B &backend, P *node)
{
    using namespace boost::hana;
    constexpr auto Config = make_map(
        make_pair(type_c<dom::XMLBackend>,
            make_tuple(
                type_c<XMLDocument>,
                type_c<dom::XMLDocument>,
                make_map(
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT>, int_c<pugi::node_document>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT_TYPE>, int_c<pugi::node_doctype>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_ELEMENT>, int_c<pugi::node_element>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_TEXT>, int_c<pugi::node_pcdata>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_CDATA_SECTION>, int_c<pugi::node_cdata>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_COMMENT>, int_c<pugi::node_comment>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION>, int_c<pugi::node_pi>))
            )),
        make_pair(type_c<dom::HTMLBackend>,
            make_tuple(
                type_c<HTMLDocument>,
                type_c<dom::HTMLDocument>,
                make_map(
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT>, int_c<LXB_DOM_NODE_TYPE_DOCUMENT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_DOCUMENT_TYPE>, int_c<LXB_DOM_NODE_TYPE_DOCUMENT_TYPE>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_ELEMENT>, int_c<LXB_DOM_NODE_TYPE_ELEMENT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_TEXT>, int_c<LXB_DOM_NODE_TYPE_TEXT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_CDATA_SECTION>, int_c<LXB_DOM_NODE_TYPE_CDATA_SECTION>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_COMMENT>, int_c<LXB_DOM_NODE_TYPE_COMMENT>),
                    make_pair(int_c<LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION>, int_c<LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION>))
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
        return backend.nodes[node] = element(backend, node);
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_TEXT>):
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            return backend.nodes[node] = HTMLText::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
        else
            return backend.nodes[node] = Text::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_CDATA_SECTION>):
        return backend.nodes[node] = CDATASection::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_DOCUMENT_TYPE>):
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            return backend.nodes[node] = HTMLDocumentType::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
        else
            return backend.nodes[node] = DocumentType::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_COMMENT>):
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            return backend.nodes[node] = HTMLComment::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
        else
            return backend.nodes[node] = Comment::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    case at_key(T, int_c<LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION>):
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            return backend.nodes[node] = HTMLProcessingInstruction::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
        else
            return backend.nodes[node] = ProcessingInstruction::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    default:
        if constexpr (std::is_same_v<B, dom::HTMLBackend>)
            if(type == LXB_DOM_NODE_TYPE_DOCUMENT_FRAGMENT)
                return backend.nodes[node] = DocumentFragment::from(backend.ctx, dom::DocumentFragment{backend.shared_from_this(), node});
        return backend.nodes[node] = Node::from(backend.ctx, dom::Node{backend.shared_from_this(), node});
    };
}

} // namespace factory
} // namespace

JSValue dom::HTMLBackend::make(lxb_dom_node_t *node)
{
    return factory::node(*this, node);
}

JSValue dom::XMLBackend::make(pugi::xml_node_struct *node)
{
    return factory::node(*this, node);
}

void notojs_init_dom()
{
    Window::init();
    DOMException::init();
    DOMTypes::init();
}

void notojs_init_dom(JSRuntime *rt)
{
    Window::init(rt);
    DOMException::init(rt);
    DOMTypes::init(rt);
}

void notojs_init_dom(detail::Config const &) {}

JSModuleDef *notojs_init_dom(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "window");
    JS_AddModuleExport(ctx, mod, Document::name());
    JS_AddModuleExport(ctx, mod, DOMParser::name());
    JS_AddModuleExport(ctx, mod, DOMException::name());
    return mod;
}

} // namespace notojs
