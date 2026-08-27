#include <notojs/module/dom/css_style_properties.hpp>
#include <notojs/module/dom/css_style_sheet.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <lexbor/style/style.h>
#include <bridge.hpp>

#include <unordered_set>
#include <cstring>
#include <utility>
#include <set>

namespace notojs::dom {
namespace {

BOOST_FORCEINLINE char const *get_attribute(lxb_dom_element_t *el, std::size_t *len = NULL)
{
    return reinterpret_cast<char const *>(lxb_dom_element_get_attribute(el, reinterpret_cast<lxb_char_t const *>("style"), 5, len));
}

BOOST_FORCEINLINE void set_attribute(lxb_dom_element_t *el, std::string const &value)
{
    lxb_dom_element_set_attribute(el, reinterpret_cast<lxb_char_t const *>("style"), 5, reinterpret_cast<lxb_char_t const *>(value.data()), value.size());
}

void append_declaration_delimiter(std::string &value)
{
    if(value.empty()) return;

    auto const end = value.find_last_not_of(" \t\r\n\f");
    if(end == std::string::npos)
    {
        value.clear();
        return;
    }

    value.resize(end + 1);
    if(value.back() != ';') value.append(";");
    value.append(" ");
}

std::string to_snake(char const *n)
{
    std::string result;
    if(n)
    {
        for(;*n; ++n)
        {
            if(auto const c = static_cast<unsigned char>(*n); std::isupper(c))
            {
                result.append("-", 1);
                result += static_cast<char>(std::tolower(c));
            }
            else result.append(n, 1);
        }
    }
    return result == "css-float" ? "float" : result;
}

struct Match
{
    lxb_css_selector_specificity_t specificity{0};
    bool found{false};
};

lxb_status_t match(lxb_dom_node_t*, lxb_css_selector_specificity_t specificity, void *ctx)
{
    auto &result = *reinterpret_cast<Match*>(ctx);
    result.found = true;
    result.specificity = std::max(result.specificity, specificity);
    return LXB_STATUS_OK;
}

std::unordered_set<std::string_view> const html = {
    "border-collapse",
    "border-spacing",
    "caption-side",
    "color",
    "cursor",
    "direction",
    "empty-cells",
    "font",
    "font-family",
    "font-feature-settings",
    "font-kerning",
    "font-size",
    "font-stretch",
    "font-style",
    "font-variant",
    "font-weight",
    "letter-spacing",
    "line-height",
    "list-style",
    "list-style-image",
    "list-style-position",
    "list-style-type",
    "orphans",
    "quotes",
    "tab-size",
    "text-align",
    "text-indent",
    "text-transform",
    "visibility",
    "white-space",
    "widows",
    "word-break",
    "word-spacing",
    "word-wrap",
    "writing-mode"
};

std::unordered_set<std::string_view> const svg = {
    "clip-rule",
    "color",
    "color-interpolation",
    "color-rendering",
    "cursor",
    "direction",
    "fill",
    "fill-opacity",
    "fill-rule",
    "font",
    "font-family",
    "font-size",
    "font-size-adjust",
    "font-stretch",
    "font-style",
    "font-variant",
    "font-weight",
    "glyph-orientation-horizontal",
    "glyph-orientation-vertical",
    "image-rendering",
    "letter-spacing",
    "line-height",
    "marker",
    "marker-end",
    "marker-mid",
    "marker-start",
    "paint-order",
    "pointer-events",
    "shape-rendering",
    "stroke",
    "stroke-dasharray",
    "stroke-dashoffset",
    "stroke-linecap",
    "stroke-linejoin",
    "stroke-miterlimit",
    "stroke-opacity",
    "stroke-width",
    "text-anchor",
    "text-rendering",
    "visibility",
    "white-space",
    "word-spacing",
    "writing-mode"
};

std::map<std::string_view, std::string_view> const global = {
    {"accent-color", "auto"},
    {"align-content", "normal"},
    {"align-items", "normal"},
    {"align-self", "auto"},
    {"alignment-baseline", "baseline"},
    {"all", "initial"},
    {"anchor-name", "none"},
    {"anchor-scope", "none"},
    {"animation", "none 0s ease 0s 1 normal none running"},
    {"animation-delay", "0s"},
    {"animation-direction", "normal"},
    {"animation-duration", "0s"},
    {"animation-fill-mode", "none"},
    {"animation-iteration-count", "1"},
    {"animation-name", "none"},
    {"animation-play-state", "running"},
    {"animation-range", "normal"},
    {"animation-timing-function", "ease"},
    {"appearance", "none"},
    {"aspect-ratio", "auto"},
    {"backdrop-filter", "none"},
    {"backface-visibility", "visible"},
    {"background", "transparent"},
    {"background-attachment", "scroll"},
    {"background-blend-mode", "normal"},
    {"background-clip", "border-box"},
    {"background-color", "transparent"},
    {"background-image", "none"},
    {"background-origin", "padding-box"},
    {"background-position", "0% 0%"},
    {"background-repeat", "repeat"},
    {"background-size", "auto auto"},
    {"baseline-shift", "baseline"},
    {"block-size", "auto"},
    {"border", "medium none currentcolor"},
    {"border-block", "medium none currentcolor"},
    {"border-block-color", "currentcolor"},
    {"border-block-style", "none"},
    {"border-block-width", "medium"},
    {"border-bottom", "medium none currentcolor"},
    {"border-bottom-color", "currentcolor"},
    {"border-bottom-left-radius", "0"},
    {"border-bottom-right-radius", "0"},
    {"border-bottom-style", "none"},
    {"border-bottom-width", "medium"},
    {"border-collapse", "separate"},
    {"border-color", "currentcolor"},
    {"border-image", "none 100% / 1 / 0 stretch"},
    {"border-inline", "medium none currentcolor"},
    {"border-inline-color", "currentcolor"},
    {"border-inline-style", "none"},
    {"border-inline-width", "medium"},
    {"border-left", "medium none currentcolor"},
    {"border-left-color", "currentcolor"},
    {"border-left-style", "none"},
    {"border-left-width", "medium"},
    {"border-radius", "0"},
    {"border-right", "medium none currentcolor"},
    {"border-right-color", "currentcolor"},
    {"border-right-style", "none"},
    {"border-right-width", "medium"},
    {"border-spacing", "0"},
    {"border-style", "none"},
    {"border-top", "medium none currentcolor"},
    {"border-top-color", "currentcolor"},
    {"border-top-left-radius", "0"},
    {"border-top-right-radius", "0"},
    {"border-top-style", "none"},
    {"border-top-width", "medium"},
    {"border-width", "medium"},
    {"bottom", "auto"},
    {"box-shadow", "none"},
    {"box-sizing", "content-box"},
    {"break-after", "auto"},
    {"break-before", "auto"},
    {"break-inside", "auto"},
    {"caption-side", "top"},
    {"caret-color", "auto"},
    {"clear", "none"},
    {"clip", "auto"},
    {"clip-path", "none"},
    {"clip-rule", "nonzero"},
    {"color", "canvastext"},
    {"color-interpolation", "srgb"},
    {"color-rendering", "auto"},
    {"column-count", "auto"},
    {"column-gap", "normal"},
    {"column-rule", "medium none currentcolor"},
    {"column-span", "none"},
    {"column-width", "auto"},
    {"columns", "auto auto"},
    {"contain", "none"},
    {"contain-intrinsic-size", "none"},
    {"container-name", "none"},
    {"container-type", "normal"},
    {"content", "normal"},
    {"counter-increment", "none"},
    {"counter-reset", "none"},
    {"counter-set", "none"},
    {"cursor", "auto"},
    {"cx", "0"},
    {"cy", "0"},
    {"d", "none"},
    {"direction", "ltr"},
    {"display", "inline"},
    {"dominant-baseline", "auto"},
    {"empty-cells", "show"},
    {"fill", "black"},
    {"fill-opacity", "1"},
    {"fill-rule", "nonzero"},
    {"filter", "none"},
    {"flex", "0 1 auto"},
    {"flex-basis", "auto"},
    {"flex-direction", "row"},
    {"flex-grow", "0"},
    {"flex-shrink", "1"},
    {"flex-wrap", "nowrap"},
    {"float", "none"},
    {"flood-color", "black"},
    {"flood-opacity", "1"},
    {"font", "normal normal normal medium / normal serif"},
    {"font-family", "serif"},
    {"font-feature-settings", "normal"},
    {"font-kerning", "auto"},
    {"font-optical-sizing", "auto"},
    {"font-size", "16px"},
    {"font-stretch", "normal"},
    {"font-style", "normal"},
    {"font-variant", "normal"},
    {"font-weight", "normal"},
    {"gap", "normal"},
    {"grid", "none / none / none"},
    {"grid-area", "auto / auto / auto / auto"},
    {"grid-auto-columns", "auto"},
    {"grid-auto-flow", "row"},
    {"grid-auto-rows", "auto"},
    {"grid-column", "auto / auto"},
    {"grid-row", "auto / auto"},
    {"grid-template", "none"},
    {"height", "auto"},
    {"hyphens", "manual"},
    {"image-rendering", "auto"},
    {"inline-size", "auto"},
    {"inset", "auto"},
    {"isolation", "auto"},
    {"justify-content", "normal"},
    {"justify-items", "legacy"},
    {"justify-self", "auto"},
    {"left", "auto"},
    {"letter-spacing", "normal"},
    {"line-height", "normal"},
    {"list-style", "disc outside none"},
    {"margin", "0"},
    {"mask", "none"},
    {"max-height", "none"},
    {"max-width", "none"},
    {"min-height", "auto"},
    {"min-width", "auto"},
    {"mix-blend-mode", "normal"},
    {"object-fit", "fill"},
    {"object-position", "50% 50%"},
    {"opacity", "1"},
    {"order", "0"},
    {"outline", "medium none invert"},
    {"overflow", "visible"},
    {"overflow-wrap", "normal"},
    {"padding", "0"},
    {"paint-order", "normal"},
    {"perspective", "none"},
    {"pointer-events", "auto"},
    {"position", "static"},
    {"r", "0"},
    {"resize", "none"},
    {"right", "auto"},
    {"rotate", "none"},
    {"row-gap", "normal"},
    {"rx", "auto"},
    {"ry", "auto"},
    {"scale", "none"},
    {"scroll-behavior", "auto"},
    {"shape-rendering", "auto"},
    {"stop-color", "black"},
    {"stop-opacity", "1"},
    {"stroke", "none"},
    {"stroke-dasharray", "none"},
    {"stroke-dashoffset", "0"},
    {"stroke-linecap", "butt"},
    {"stroke-linejoin", "miter"},
    {"stroke-miterlimit", "4"},
    {"stroke-opacity", "1"},
    {"stroke-width", "1"},
    {"tab-size", "8"},
    {"table-layout", "auto"},
    {"text-align", "start"},
    {"text-anchor", "start"},
    {"text-decoration", "none solid currentcolor"},
    {"text-overflow", "clip"},
    {"text-rendering", "auto"},
    {"text-shadow", "none"},
    {"text-transform", "none"},
    {"top", "auto"},
    {"touch-action", "auto"},
    {"transform", "none"},
    {"transform-box", "view-box"},
    {"transform-origin", "50% 50% 0"},
    {"transform-style", "flat"},
    {"transition", "all 0s ease 0s"},
    {"translate", "none"},
    {"unicode-bidi", "normal"},
    {"user-select", "auto"},
    {"vector-effect", "none"},
    {"vertical-align", "baseline"},
    {"visibility", "visible"},
    {"white-space", "normal"},
    {"width", "auto"},
    {"will-change", "auto"},
    {"word-break", "normal"},
    {"word-spacing", "normal"},
    {"writing-mode", "horizontal-tb"},
    {"x", "0"},
    {"y", "0"},
    {"z-index", "auto"}
};

bool inherits(lxb_dom_node_t *node, std::string const &key)
{
    return (key.size() >= 2 && key[0] == '-' && key[1] == '-')
        || html.count(key)
        || (LXB_NS_SVG == node->ns && svg.count(key));
}

} // namespace

void CSSStyleProperties::Attr::free(HTMLElement &el) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    doc->styles.erase(el);
}

JSValue CSSStyleProperties::Attr::cssText(HTMLElement const &el) const
{
    std::size_t len;
    if(auto *str = get_attribute(el, &len)) return bridge::String{el.doc->ctx, std::string_view{str, len}};
    return bridge::String{el.doc->ctx};
}

std::uint32_t CSSStyleProperties::Attr::length(HTMLElement const &el) const
{
    std::uint32_t len{0};
    auto *doc = lxb_dom_interface_node(el)->owner_document;
    lexbor_avl_foreach(doc->css->styles, &static_cast<lxb_dom_element_t *>(el)->style,
        [](auto *, auto *, auto *, void *ctx) -> unsigned {
            ++*reinterpret_cast<std::int32_t*>(ctx);
            return LXB_STATUS_OK;
        }, &len);
    return len;
}

std::vector<std::pair<std::size_t, std::size_t>> CSSStyleProperties::Attr::bounds(HTMLElement const &el) const
{
    std::vector<std::pair<std::size_t, std::size_t>> context;
    auto *doc = lxb_dom_interface_node(el)->owner_document;
    lexbor_avl_foreach(doc->css->styles, &static_cast<lxb_dom_element_t *>(el)->style,
        [](auto *, auto *, lexbor_avl_node *node, void *ctx) -> unsigned {
            lxb_style_node_t *style = reinterpret_cast<lxb_style_node_t*>(node);
            lxb_css_rule_declaration_t *decl = reinterpret_cast<lxb_css_rule_declaration_t *>(style->entry.value);
            auto *context = reinterpret_cast<std::vector<std::pair<std::size_t, std::size_t>>*>(ctx);
            context->push_back({decl->offset.name_begin, decl->offset.name_end});
            return LXB_STATUS_OK;
        }, &context);
    return context;
}

JSValue CSSStyleProperties::Attr::item(HTMLElement const &el, std::int32_t x) const
{
    if(auto *attr = get_attribute(el))
    {
        if(auto bs = bounds(el); x < bs.size())
        {
            std::sort(std::begin(bs), std::end(bs), [](auto const &lhs, auto const &rhs){
                return lhs.first < rhs.first;
            });
            return bridge::String{el.doc->ctx, std::string_view{attr + bs[x].first, bs[x].second - bs[x].first}};
        }
    }
    return bridge::String{el.doc->ctx};
}

JSValue CSSStyleProperties::Attr::getPropertyPriority(HTMLElement const &el, std::string_view const &name) const
{
    if(auto *attr = get_attribute(el))
        if(auto rule = lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size());
            rule && rule->offset.important_end != rule->offset.important_begin)
        return bridge::String{el.doc->ctx, std::string_view{
            attr + rule->offset.important_begin + 1,
            rule->offset.important_end - rule->offset.important_begin - 1
        }};
    return bridge::String{el.doc->ctx};
}

JSValue CSSStyleProperties::Attr::getPropertyValue(HTMLElement const &el, std::string_view const &name) const
{
    if(auto *attr = get_attribute(el))
        if(auto rule = lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size()))
            return bridge::String{el.doc->ctx, std::string_view{
                attr + rule->offset.value_begin,
                rule->offset.value_end - rule->offset.value_begin
            }};
    return bridge::String{el.doc->ctx};
}

JSValue CSSStyleProperties::Attr::removeProperty(HTMLElement &el, std::string_view const &name)
{
    std::size_t len;
    JSValue result = JS_NULL;
    if(auto *attr = get_attribute(el, &len))
    {
        if(auto rule = lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size()))
        {
            result = bridge::String{el.doc->ctx, std::string_view{
                attr + rule->offset.value_begin,
                rule->offset.value_end - rule->offset.value_begin
            }};

            std::size_t e = rule->offset.name_begin;
            while(e > 0 && std::isspace(attr[e - 1])) --e;
            std::string value{attr, e};

            e = rule->offset.important_end ? rule->offset.important_end : rule->offset.value_end;
            while(e < len && attr[e] != '-' && !std::isalpha(attr[e])) ++e;
            value.append(attr + e, len - e);

            if(value.empty()) lxb_dom_element_remove_attribute(el, reinterpret_cast<lxb_char_t const *>("style"), 5);
            else lxb_dom_element_set_attribute(el, reinterpret_cast<lxb_char_t const *>("style"), 5, reinterpret_cast<lxb_char_t const *>(value.data()), value.size());
        }
    }
    return JS_IsNull(result) ? bridge::String{el.doc->ctx} : result;
}

void CSSStyleProperties::Attr::setProperty(HTMLElement &el, std::string_view const &name, std::string_view const &v)
{
    std::size_t len;
    std::string value;
    if(auto *attr = get_attribute(el, &len))
    {
        if(auto rule = lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size()))
        {
            std::size_t e = rule->offset.name_begin;
            while(e > 0 && std::isspace(attr[e - 1])) --e;
            value.append(attr, e);

            e = rule->offset.important_end ? rule->offset.important_end : rule->offset.value_end;
            while(e < len && attr[e] != '-' && !std::isalpha(attr[e])) ++e;
            value.append(attr + e, len - e);
        }
        else value.append(attr, len);
    }

    append_declaration_delimiter(value);
    value.append(name);
    value.append(": ");
    value.append(v);
    value.append(";");

    set_attribute(el, value);
}

void CSSStyleProperties::Attr::setProperty(HTMLElement &el, std::string_view const &name, std::string_view const &v, std::string_view const &p)
{
    std::size_t len;
    std::string value;
    if(auto *attr = get_attribute(el, &len))
    {
        if(auto rule = lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size()))
        {
            value.append(attr, rule->offset.name_begin);
            std::size_t e = rule->offset.important_end ? rule->offset.important_end : rule->offset.value_end;
            while(e < len && attr[e] != '-' && !std::isalpha(attr[e])) ++e;
            value.append(attr + e, len - e);
        }
        else value.append(attr, len);
    }

    append_declaration_delimiter(value);
    value.append(name);
    value.append(": ");
    value.append(v);
    value.append(" !");
    value.append(p);
    value.append(";");

    set_attribute(el, value);
}

JSValue CSSStyleProperties::Attr::get_property(HTMLElement const &el, char const *name) const
{
    auto const n = to_snake(name);
    if(global.count(n) || lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(n.c_str()), n.size()))
        return getPropertyValue(el, n);
    return JS_UNDEFINED;
}

bool CSSStyleProperties::Attr::has_property(HTMLElement const &el, char const *name) const
{
    auto const n = to_snake(name);
    return global.count(n) || static_cast<bool>(lxb_html_element_style_by_name(el, reinterpret_cast<lxb_char_t const *>(n.c_str()), n.size()));
}

void CSSStyleProperties::Attr::set_property(HTMLElement &el, char const *name, std::string_view const &v)
{
    setProperty(el, to_snake(name), v);
}

JSValue CSSStyleProperties::Attr::names(HTMLElement const &el) const
{
    bridge::Array arr{el.doc->ctx};
    if(auto *attr = get_attribute(el))
    {
        for(auto const &[b, e]: bounds(el))
            arr.append(bridge::String{el.doc->ctx, std::string_view{attr + b, e - b}});
    }
    return arr;
}

void CSSStyleProperties::Comp::free(HTMLElement &el) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    doc->comp.erase(static_cast<lxb_dom_node_t *>(el));
}

void CSSStyleProperties::Comp::init(HTMLElement const &el) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    doc->comp.reg(static_cast<lxb_dom_node_t *>(el));
    update(el);
}

std::uint32_t CSSStyleProperties::Comp::length(HTMLElement const &el, Attr const &a) const
{
    std::unordered_set<std::string> exotic;
    if(auto *attr = get_attribute(el))
        for(auto const &[b, e]: a.bounds(el))
            if(std::string name{attr + b, e - b}; !global.count(name))
                exotic.insert(std::move(name));

    update(el);
    for(auto const &[k, _]: style)
        if(!global.count(k))
            exotic.insert(k);
    return global.size() + exotic.size();
}

JSValue CSSStyleProperties::Comp::item(HTMLElement const &el, Attr const &a, std::int32_t x) const
{
    if(x < 0) return bridge::String{el.doc->ctx};
    if(x < global.size())
    {
        auto it = std::begin(global);
        std::advance(it, x);
        return bridge::String{el.doc->ctx, it->first};
    }
    x -= global.size();

    std::set<std::string> exotic;
    if(auto *attr = get_attribute(el))
        for(auto const &[b, e]: a.bounds(el))
            if(std::string name{attr + b, e - b}; !global.count(name))
                exotic.insert(std::move(name));

    update(el);
    for(auto const &[k, _]: style)
        if(!global.count(k))
            exotic.insert(k);

    if(x < exotic.size())
    {
        auto it = std::begin(exotic);
        std::advance(it, x);
        return bridge::String{el.doc->ctx, *it};
    }
    return bridge::String{el.doc->ctx};
}

JSValue CSSStyleProperties::Comp::names(HTMLElement const &el, Attr const &a) const
{
    update(el);
    bridge::Array arr{el.doc->ctx};
    for(auto const &[k, _]: global)
        arr.append(bridge::String{el.doc->ctx, k});

    std::set<std::string> exotic;
    if(auto *attr = get_attribute(el))
        for(auto const &[b, e]: a.bounds(el))
            if(std::string name{attr + b, e - b}; !global.count(name))
                exotic.insert(std::move(name));

    for(auto const &[k, _]: style)
        if(!global.count(k))
            exotic.insert(k);

    for(auto const &n: exotic)
        arr.append(bridge::String{el.doc->ctx, n});

    return arr;
}

void CSSStyleProperties::Comp::cascade(std::string name, std::string value,
                                        lxb_css_selector_specificity_t specificity,
                                        std::uint64_t order, std::uint32_t depth,
                                        bool important) const
{
    Candidate candidate{std::move(value), specificity, order, depth, important};
    auto it = style.find(name);
    if(it == std::end(style))
    {
        style.emplace(std::move(name), std::move(candidate));
        return;
    }

    auto const &current = it->second;
    bool const wins = candidate.depth < current.depth
        || (candidate.depth == current.depth && (
            candidate.important > current.important
            || (candidate.important == current.important && (
                candidate.specificity > current.specificity
                || (candidate.specificity == current.specificity
                    && candidate.order >= current.order)))));

    if(wins) it->second = std::move(candidate);
}

void CSSStyleProperties::Comp::update(HTMLElement const &el) const
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(el.doc.get());
    auto *target = static_cast<lxb_dom_node_t *>(el);
    if(doc->comp.upd(target, gen))
    {
        auto *root = target;
        while(root->parent != nullptr) root = root->parent;

        auto *css = lxb_dom_interface_document(doc->doc.get())->css;
        auto *lst = lxb_css_selectors_parse(css->parser, (lxb_char_t const *)"style", 5);

        struct Output
        {
            lxb_dom_node_t *target;
            CSSStyleProperties::Comp const *computed;
            std::shared_ptr<Backend> document;
            std::uint64_t order{0};
        };
        Output parsed{target, this, el.doc};

        style.clear();
        lxb_selectors_find(css->selectors, root, lst, [](lxb_dom_node_t *node, lxb_css_selector_specificity_t, void *ctx) -> lxb_status_t {
            auto &output = *reinterpret_cast<Output*>(ctx);
            HTMLElement element{output.document, lxb_html_interface_element(node)};
            auto state = CSSStyleSheet::getState(element);
            if(!state->update(element)) return LXB_STATUS_OK;

            auto const *source = state->source();
            if(!source) return LXB_STATUS_OK;
            auto const &data = *source;
            auto *css = lxb_dom_interface_document(node->owner_document)->css;

            std::unique_ptr<lxb_selectors_t, HTMLBackend::Deleter> sel{nullptr};
            for(auto *list = state->root(); list; list = list->next)
            {
                if(LXB_CSS_RULE_LIST != list->type) continue;
                for(auto *rule = lxb_css_rule_list(list)->first; rule; rule = rule->next)
                {
                    if(LXB_CSS_RULE_STYLE != rule->type) continue;

                    if(!sel)
                    {
                        sel.reset(lxb_selectors_create());
                        if(LXB_STATUS_OK != lxb_selectors_init(sel.get())) break;
                    }

                    auto *s = lxb_css_rule_style(rule);
                    std::uint32_t depth{0};
                    for(auto *ancestor = output.target; ancestor; ancestor = ancestor->parent, ++depth)
                    {
                        Match matched;
                        lxb_selectors_match_node(sel.get(), ancestor, s->selector, match, &matched);
                        if(!matched.found) continue;

                        for(auto *declaration = s->declarations->first; declaration; declaration = declaration->next)
                        {
                            if(LXB_CSS_RULE_DECLARATION != declaration->type) continue;
                            auto *decl = lxb_css_rule_declaration(declaration);

                            std::string name{data.c_str() + decl->offset.name_begin,
                                             decl->offset.name_end - decl->offset.name_begin};
                            auto const order = output.order++;
                            if(ancestor != output.target && !inherits(output.target, name)) continue;

                            output.computed->cascade(std::move(name), std::string{
                                data.c_str() + decl->offset.value_begin,
                                decl->offset.value_end - decl->offset.value_begin
                            }, matched.specificity, order, depth, decl->important);
                        }
                    }
                }
            }
            return LXB_STATUS_OK;
        }, &parsed);
        lxb_css_selector_list_destroy(lst);

        lxb_css_selector_specificity_t inline_specificity{0};
        lxb_css_selector_sp_set_s(inline_specificity, 1);
        std::uint32_t depth{0};
        for(auto *ancestor = target; ancestor; ancestor = ancestor->parent, ++depth)
        {
            if(LXB_DOM_NODE_TYPE_ELEMENT != ancestor->type) continue;

            HTMLElement element{el.doc, lxb_html_interface_element(ancestor)};
            auto *attribute = get_attribute(element);
            if(!attribute) continue;

            for(auto const &[begin, end]: Attr{}.bounds(element))
            {
                std::string name{attribute + begin, end - begin};
                if(ancestor != target && !inherits(target, name)) continue;

                auto *decl = lxb_html_element_style_by_name(element,
                    reinterpret_cast<lxb_char_t const *>(name.data()), name.size());
                if(!decl) continue;

                cascade(std::move(name), std::string{
                    attribute + decl->offset.value_begin,
                    decl->offset.value_end - decl->offset.value_begin
                }, inline_specificity, parsed.order++, depth, decl->important);
            }
        }
    }
}

JSValue CSSStyleProperties::Comp::getPropertyValue(HTMLElement const &el, Attr const &, std::string_view const &name) const
{
    update(el);

    if(auto it = style.find(std::string{name.data(), name.size()}); it != std::end(style))
        return bridge::String{el.doc->ctx, it->second.value};

    if(auto *node = static_cast<lxb_dom_node_t *>(el); LXB_NS_SVG == node->ns && LXB_TAG_SVG != lxb_dom_node_tag_id(node))
    {
        std::size_t len;
        if(auto *val = lxb_dom_element_get_attribute(el, reinterpret_cast<lxb_char_t const *>(name.data()), name.size(), &len); val)
            return bridge::String{el.doc->ctx, std::string_view{reinterpret_cast<char const *>(val), len}};
    }

    auto it = global.find(std::string{name.data(), name.size()});
    return it == std::end(global) ? bridge::String{el.doc->ctx} : bridge::String{el.doc->ctx, it->second};
}

JSValue CSSStyleProperties::Comp::get_property(HTMLElement const &el, Attr const &attr, char const *name) const
{
    auto const n = to_snake(name);
    if(has_property(el, attr, name)) return getPropertyValue(el, attr, n);
    return JS_UNDEFINED;
}

bool CSSStyleProperties::Comp::has_property(HTMLElement const &el, Attr const &attr, char const *name) const
{
    auto const n = to_snake(name);
    update(el);
    return global.count(n) || attr.has_property(el, name) || style.find(n) != std::end(style);
}

} // namespace notojs:dom
