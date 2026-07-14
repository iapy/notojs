#include <notojs/module/dom/backend.hpp>
#include <notojs/module/dom/lexbor.hpp>
#include <notojs/module/dom/node.hpp>
#include <notojs/module/dom/xpath.hpp>

#include <lexbor/style/attribute_steps.h>
#include <lexbor/style/element_steps.h>
#include <bridge.hpp>
#include <cstring>

#include <iostream>

namespace notojs::dom {
namespace {

constexpr std::string_view NN_CDATA    {"#cdata-section"};
constexpr std::string_view NN_COMMENT  {"#comment"};
constexpr std::string_view NN_DOCUMENT {"#document"};
constexpr std::string_view NN_TEXT     {"#text"};

bool is_equal_node(pugi::xml_node a, pugi::xml_node b)
{
    if(a == b) return true;
    if(!a || !b) return a == b;
    if(a.type() != b.type()) return false;

    if(std::strcmp(a.name(), b.name())) return false;
    if(std::strcmp(a.value(), b.value())) return false;

    std::int64_t count = 0;
    for(auto ab: b.attributes())
        ++count;

    for(auto aa: a.attributes())
    {
        --count;
        bool found = false;
        for(auto ab: b.attributes())
            if((found = !std::strcmp(aa.name(), ab.name()) && !std::strcmp(aa.value(), ab.value()))) break;

        if(!found)
            return false;
    }

    if(count) return false;

    auto ac = a.first_child();
    auto bc = b.first_child();

    for(;ac && bc; ac = ac.next_sibling(), bc = bc.next_sibling())
    {
        if(!is_equal_node(ac, bc))
            return false;
    }
    return !ac && !bc;
}

bool is_equal_node(lxb_dom_node_t *a, lxb_dom_node_t *b)
{
    if(a == b) return true;
    if(!a || !b) return a == b;
    if(a->type != b->type) return false;

    switch (a->type)
    {
    case LXB_DOM_NODE_TYPE_TEXT:
    case LXB_DOM_NODE_TYPE_COMMENT:
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        {
            auto* ca = lxb_dom_interface_character_data(a);
            auto* cb = lxb_dom_interface_character_data(b);

            if(ca->data.length != cb->data.length) return false;
            if(memcmp(ca->data.data, cb->data.data, ca->data.length))
                return false;
        }
        break;
    case LXB_DOM_NODE_TYPE_ELEMENT:
        {
            auto* ea = lxb_dom_interface_element(a);
            auto* eb = lxb_dom_interface_element(b);

            if(a->local_name != b->local_name || a->ns != b->ns)
                return false;

            std::int64_t count{0};
            for(lxb_dom_attr_t *ab = eb->first_attr; ab; ab = ab->next)
                ++count;

            for(lxb_dom_attr_t *aa = ea->first_attr; aa; aa = aa->next)
            {
                --count;
                bool found = false;
                for(lxb_dom_attr_t *ab = eb->first_attr; ab; ab = ab->next)
                    if((found = lxb_dom_attr_compare(aa, ab))) break;
                if(!found) return false;
            }

            if(count) return false;
        }
        break;
    default:
        break;
    }

    lxb_dom_node_t* ac = a->first_child;
    lxb_dom_node_t* bc = b->first_child;

    for(;ac && bc; ac = ac->next, bc = bc->next)
        if (!is_equal_node(ac, bc))
            return false;

    return !ac && !bc;
}

void normalize(pugi::xml_node node, std::unordered_map<pugi::xml_node_struct *, JSValue> &nodes)
{
    for(auto child = node.first_child(); child;)
    {
        auto next = child.next_sibling();
        if(child.type() == pugi::node_pcdata)
        {
            while(next && next.type() == pugi::node_pcdata)
            {
                std::string merged = child.value();
                merged += next.value();

                child.set_value(merged.c_str());

                auto remove = next;
                next = next.next_sibling();

                if(nodes.count(remove.internal_object())) node.root().append_move(remove);
                else node.remove_child(remove);
            }

            if(std::strlen(child.value()) == 0)
            {
                auto remove = child;
                child = next;

                if(nodes.count(node.internal_object())) node.root().append_move(node);
                else node.remove_child(remove);
                continue;
            }
        }
        else normalize(child, nodes);
        child = next;
    }
}

void normalize(lxb_dom_node_t *node, std::unordered_map<lxb_dom_node_t *, JSValue> &nodes)
{
    for(lxb_dom_node_t *child = node->first_child; child;)
    {
        lxb_dom_node_t *next = child->next;
        if(LXB_DOM_NODE_TYPE_TEXT == child->type)
        {
            auto *text = lxb_dom_interface_character_data(child);
            std::string merged((char const *)text->data.data, text->data.length);
            while(next && LXB_DOM_NODE_TYPE_TEXT == next->type)
            {
                auto *next_text = lxb_dom_interface_character_data(next);
                merged.append((char const *)next_text->data.data, next_text->data.length);

                lxb_dom_node_t *remove = next;
                next = next->next;
                if(!nodes.count(remove))
                    lxb_dom_node_destroy(remove);
                lxb_dom_node_remove(remove);
            }
            lxb_dom_node_text_content_set(child, (lxb_char_t const *)merged.data(), merged.size());
            if(text->data.length == 0)
            {
                lxb_dom_node_remove(child);
                if(!nodes.count(child))
                    lxb_dom_node_destroy(child);
            }
        }
        else normalize(child, nodes);
        child = next;
    }
}

lxb_status_t append_node(lxb_dom_node_t *node, lxb_css_selector_specificity_t, void *ctx)
{
    reinterpret_cast<std::vector<void*>*>(ctx)->push_back(node);
    return LXB_STATUS_OK;
}

lxb_status_t find_node(lxb_dom_node_t *node, lxb_css_selector_specificity_t, void *ctx)
{
    *reinterpret_cast<lxb_dom_node_t**>(ctx) = node;
    return LXB_STATUS_STOP;
}

lxb_status_t node_inserted(lxb_dom_node_t *node)
{
    if(node->owner_document && node->owner_document->user)
    {
        switch(node->type) {
        case LXB_DOM_NODE_TYPE_ELEMENT:
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(node);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub(node);
            break;
        case LXB_DOM_NODE_TYPE_CDATA_SECTION:
        case LXB_DOM_NODE_TYPE_TEXT:
            if(node->parent && LXB_TAG_STYLE == lxb_dom_node_tag_id(node->parent))
            {
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub();
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->csss.pub();
            }
        default: break;
        };
    }
    return lxb_style_element_steps_insertion(node);
}

lxb_status_t node_removed(lxb_dom_node_t *node, lxb_dom_node_t *from)
{
    if(node->owner_document && node->owner_document->user)
    {
        switch(node->type) {
        case LXB_DOM_NODE_TYPE_ELEMENT:
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(node);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(from);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub(node);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub(from);
            break;
        case LXB_DOM_NODE_TYPE_CDATA_SECTION:
        case LXB_DOM_NODE_TYPE_TEXT:
            if(LXB_TAG_STYLE == lxb_dom_node_tag_id(from))
            {
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub();
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->csss.pub();
            }
        default: break;
        };
    }
    return lxb_style_element_steps_removing(node, from);
}

lxb_status_t node_moved(lxb_dom_node_t *node, lxb_dom_node_t *from)
{
    if(node->owner_document && node->owner_document->user)
    {
        switch(node->type) {
        case LXB_DOM_NODE_TYPE_ELEMENT:
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(node);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(from);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub(node);
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub(from);
            break;
        case LXB_DOM_NODE_TYPE_CDATA_SECTION:
        case LXB_DOM_NODE_TYPE_TEXT:
            if(node->parent && LXB_TAG_STYLE == lxb_dom_node_tag_id(node->parent) || LXB_TAG_STYLE == lxb_dom_node_tag_id(from))
            {
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub();
                reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub();
            }
        default: break;
        };
    }
    return lxb_style_element_steps_moving(node, from);
}

template<auto fn>
lxb_status_t attr_change(lxb_dom_element_t *e, lxb_dom_attr_id_t a, lxb_char_t const *ov, size_t ol, lxb_char_t const *nv, size_t nl, lxb_ns_id_t id)
{
    if(auto *node = lxb_dom_interface_node(e); node->owner_document && node->owner_document->user)
    {
        switch(a) {
        case LXB_DOM_ATTR_CLASS:
            reinterpret_cast<HTMLBackend *>(node->owner_document->user)->live.pub(node);
            break;
        }
        reinterpret_cast<HTMLBackend *>(node->owner_document->user)->comp.pub();
    }
    return fn(e, a, ov, ol, nv, nl, id);
}

const lxb_dom_document_mutation_cb_t lxb_dom_document_mutation_cbs = {
    .inserted = &node_inserted,
    .removed = &node_removed,
    .moved = &node_moved,
    .destroy = &lxb_style_element_steps_destroy,
    .children_changed = lxb_style_element_steps_children_changed,
    .connected = lxb_style_element_steps_post_connection
};

const lxb_dom_document_attr_mutation_cb_t lxb_dom_document_attr_mutation_cbs = {
    .change = &attr_change<lxb_style_attribute_steps_change>,
    .append = &attr_change<lxb_style_attribute_steps_append>,
    .remove = &attr_change<lxb_style_attribute_steps_remove>,
    .replace = &attr_change<lxb_style_attribute_steps_replace>
};

} // namespace

JSValue XMLBackend::appendChild(Node const &p, Node const &c)
{
    pugi::xml_node pn{static_cast<pugi::xml_node_struct *>(p.node)};
    pugi::xml_node cn{static_cast<pugi::xml_node_struct *>(c.node)};
    return make(pn.append_move(cn).internal_object());
}

JSValue XMLBackend::cloneNode(Node const &n, bool deep)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(deep || pugi::node_element != nn.type())
    {
        pugi::xml_node clone = doc.append_copy(nn);
        return make(clone.internal_object());
    }
    else
    {
        pugi::xml_node clone = doc.append_child(nn.name());
        for(auto attr: nn.attributes())
            clone.append_attribute(attr.name()).set_value(attr.value());
        return make(clone.internal_object());
    }
}

JSValue XMLBackend::compareDocumentPosition(Node const &n, Node const &o)
{
    if(n.doc != o.doc) return bridge::Number{ctx, 1};
    if(n.node == o.node) return bridge::Number{ctx, 0};

    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    pugi::xml_node on{static_cast<pugi::xml_node_struct *>(o.node)};

    std::vector<pugi::xml_node> opath;
    for(pugi::xml_node x = on; x; x = x.parent()) {
        if(x == nn) return bridge::Number{ctx, 8 | 2}; // CONTAINS
        if(!is_connected(x)) break;
        opath.push_back(x);
    }

    std::vector<pugi::xml_node> npath;
    for(pugi::xml_node x = nn; x; x = x.parent()) {
        if(x == on) return bridge::Number{ctx, 16 | 4}; // CONTAINED_BY
        if(!is_connected(x)) break;
        npath.push_back(x);
    }

    auto ot = std::rbegin(opath);
    auto nt = std::rbegin(npath);

    for(;ot != std::rend(opath) && nt != std::rend(npath); ++ot, ++nt)
    {
        if(*ot == *nt) continue;
        for(pugi::xml_node x = ot->next_sibling(); x && is_connected(x); x = x.next_sibling())
            if(x == *nt) return bridge::Number{ctx, 4}; // FOLLOWING
        for(pugi::xml_node x = nt->next_sibling(); x && is_connected(x); x = x.next_sibling())
            if(x == *ot) return bridge::Number{ctx, 2}; // PRECEEDING
        break;
    }

    return bridge::Number{ctx, 1};
}

bool XMLBackend::contains(Node const &n, Node const &o)
{
    if(n.doc != o.doc) return false;

    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    pugi::xml_node on{static_cast<pugi::xml_node_struct *>(o.node)};

    for(pugi::xml_node x = on; x; x = x.parent()) {
        if(x == nn) return true;
        if(!is_connected(x)) return false;
    }

    return false;
}

JSValue XMLBackend::firstChild(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto fc = nn.first_child()) return make(fc.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::getRootNode(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for(;nn.parent(); nn = nn.parent()) {
        if(!is_connected(nn)) return make(nn.internal_object());
    }
    return make(nn.internal_object());
}

JSValue XMLBackend::hasChildNodes(Node const &n)
{
    return pugi::xml_node{static_cast<pugi::xml_node_struct *>(n.node)}.children().empty() ? JS_FALSE : JS_TRUE;
}

JSValue XMLBackend::insertBefore(Node const &n, Node const &o, Node const &r)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    pugi::xml_node on{static_cast<pugi::xml_node_struct *>(o.node)};
    pugi::xml_node rn{static_cast<pugi::xml_node_struct *>(r.node)};
    return make(nn.insert_move_before(on, rn).internal_object());
}

void XMLBackend::insertBeforeVoid(Node const &n, Node const &o, Node const &r)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    pugi::xml_node on{static_cast<pugi::xml_node_struct *>(o.node)};
    pugi::xml_node rn{static_cast<pugi::xml_node_struct *>(r.node)};
    (void)nn.insert_move_before(on, rn);
}

bool XMLBackend::isChild(Node const &n, Node const &c)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    pugi::xml_node cn{static_cast<pugi::xml_node_struct *>(c.node)};
    return cn.parent() == nn;
}

JSValue XMLBackend::isConnected(Node const &n)
{
    if(n.node == doc.internal_object()) return JS_TRUE;
    return is_connected(pugi::xml_node{static_cast<pugi::xml_node_struct *>(n.node)}) ? JS_TRUE : JS_FALSE;
}

JSValue XMLBackend::isDefaultNamespace(Node const &, std::string_view const &sv)
{
    return JS_FALSE;
}

JSValue XMLBackend::isEqualNode(Node const &n, Node const &o)
{
    return is_equal_node(
        pugi::xml_node{static_cast<pugi::xml_node_struct *>(n.node)},
        pugi::xml_node{static_cast<pugi::xml_node_struct *>(o.node)}
    ) ? JS_TRUE : JS_FALSE;
}

JSValue XMLBackend::lastChild(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto fc = nn.last_child()) return make(fc.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::namespaceURI(Node const &n)
{
    return JS_NULL;
}

JSValue XMLBackend::nextSibling(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto fc = nn.next_sibling(); fc && is_connected(nn) && is_connected(fc))
        return make(fc.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::nodeName(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    switch(nn.type())
    {
        case pugi::node_cdata:
            return bridge::String{ctx, NN_CDATA};
        case pugi::node_comment:
            return bridge::String{ctx, NN_COMMENT};
        case pugi::node_document:
            return bridge::String{ctx, NN_DOCUMENT};
        case pugi::node_element:
        case pugi::node_pi:
            return bridge::String{ctx, std::string(nn.name())};
        case pugi::node_pcdata:
            return bridge::String{ctx, NN_TEXT};
        default:
            break;
    }
    return JS_UNDEFINED;
}

JSValue XMLBackend::nodeType(Node const &n)
{
    constexpr int types[] = {0, 9, 1, 3, 4, 8, 7, 0, 10, 0, 0, 0, 0, 0, 0};
    return bridge::Number(ctx, types[pugi::xml_node(static_cast<pugi::xml_node_struct *>(n.node)).type()]);
}

JSValue XMLBackend::nodeValue(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    switch(nn.type())
    {
    case pugi::node_cdata:
    case pugi::node_comment:
    case pugi::node_pcdata:
    case pugi::node_pi:
        return bridge::String(ctx, std::string{nn.value()});
    default:
        break;
    }
    return JS_NULL;
}

void XMLBackend::nodeValue(Node const &n, std::string_view const &text)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    switch(nn.type())
    {
    case pugi::node_cdata:
    case pugi::node_comment:
    case pugi::node_pcdata:
    case pugi::node_pi:
        nn.set_value(text.data(), text.size());
    default:
        break;
    }
}

void XMLBackend::normalize(Node const &n)
{
    dom::normalize(pugi::xml_node{static_cast<pugi::xml_node_struct *>(n.node)}, nodes);
}

JSValue XMLBackend::ownerDocument()
{
    return make(doc.internal_object());
}

JSValue XMLBackend::parentElement(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto p = nn.parent(); p && (p != doc || nn == p.first_child()))
        return make(p.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::parentNode(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto p = nn.parent(); p && is_connected(nn))
        return make(p.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::previousSibling(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto fc = nn.previous_sibling(); fc && is_connected(nn) && is_connected(fc))
        return make(fc.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::remove(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    return make(nn.root().append_move(nn).internal_object());
}

JSValue XMLBackend::textContent(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(pugi::node_document != nn.type())
    {
        std::string result;
        std::invoke(boost::hana::fix([this, &result](auto self, pugi::xml_node nn) -> void {
            if(!is_connected(nn)) return;
            if(pugi::node_pcdata == nn.type() || pugi::node_cdata == nn.type())
                result.append(nn.value());
            else if(pugi::node_element == nn.type())
                for(auto const cn: nn.children()) self(cn);
        }), nn);
        return bridge::String{ctx, std::move(result)};
    }
    return JS_NULL;
}

void XMLBackend::textContent(Node const &n, std::optional<std::string_view> &&text)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    switch(nn.type())
    {
    case pugi::node_document:
        return;
    case pugi::node_cdata:
    case pugi::node_comment:
    case pugi::node_pcdata:
    case pugi::node_pi:
        if(text)
            nn.set_value(*text);
        else if(nodes.count(nn.internal_object()))
            nn.root().append_move(nn);
        else
            nn.parent().remove_child(nn);
        break;
    default:
        while(auto fc = nn.first_child())
        {
            if(nodes.count(fc.internal_object()))
                nn.root().append_move(fc);
            else
                nn.remove_child(fc);
        }
        if(text) nn.append_child(pugi::node_pcdata).set_value(*text);
    };
}

void XMLBackend::free(Node const &n)
{
    nodes.erase(static_cast<pugi::xml_node_struct *>(n.node));
}

JSValue XMLBackend::childNodes(Node const &n)
{
    bridge::Array arr{ctx};
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for (auto n = nn.first_child(); n; n = n.next_sibling())
        arr.append(make(n.internal_object()));
    return arr;
}

std::uint64_t XMLBackend::childNodeCount(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    return static_cast<std::uint64_t>(std::distance(nn.begin(), nn.end()));
}

JSValue XMLBackend::childNode(Node const &n, std::int64_t i, JSValue d)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for (auto ch = nn.first_child(); ch; ch = ch.next_sibling(), --i)
        if(!i) return make(ch.internal_object());
    return d;
}

JSValue XMLBackend::childElements(Node const &n)
{
    bridge::Array arr{ctx};
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for (auto n = nn.first_child(); n; n = n.next_sibling())
        if(pugi::node_element == n.type()) arr.append(make(n.internal_object()));
    return arr;
}

JSValue XMLBackend::childElement(Node const &n, std::int64_t i, JSValue d)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for (auto n = nn.first_child(); n; n = n.next_sibling())
        if(pugi::node_element == n.type() && !i--) return make(n.internal_object());
    return d;
}

JSValue XMLBackend::childElement(Node const &, std::string_view const &)
{
    return JS_NULL;
}

JSValue XMLBackend::collection(std::vector<void*> const &nodes)
{
    bridge::Array arr{ctx};
    for(void *node: nodes) arr.append(make(static_cast<pugi::xml_node_struct *>(node)));
    return arr;
}

JSValue XMLBackend::collection_item(std::vector<void*> const &nodes, std::int64_t i, JSValue def)
{
    return i < nodes.size() ? make(static_cast<pugi::xml_node_struct *>(nodes[i])) : def;
}

void XMLBackend::appendChildVoid(Node const &p, Node const &c)
{
    pugi::xml_node pn{static_cast<pugi::xml_node_struct *>(p.node)};
    pugi::xml_node cn{static_cast<pugi::xml_node_struct *>(c.node)};
    pn.append_move(cn);
}

std::uint64_t XMLBackend::childElementCount(Node const &n)
{
    std::uint64_t count{0};
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for(auto ch = nn.first_child(); ch && is_connected(ch); ch = ch.next_sibling())
        if(pugi::node_element == ch.type()) ++count;
    return count;
}

Node XMLBackend::createTextNode(std::string_view const &text)
{
    auto node = doc.append_child(pugi::node_pcdata);
    node.text().set(text);
    return Node{shared_from_this(), node.internal_object()};
}

std::optional<Node> XMLBackend::first(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto ch = nn.first_child()) return Node{shared_from_this(), ch.internal_object()};
    return std::nullopt;
}

JSValue XMLBackend::firstElementChild(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for(auto ch = nn.first_child(); ch && is_connected(ch); ch = ch.next_sibling())
        if(pugi::node_element == ch.type()) return make(ch.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::lastElementChild(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for(auto ch = nn.last_child(); ch && is_connected(ch); ch = ch.previous_sibling())
        if(pugi::node_element == ch.type()) return make(ch.internal_object());
    return JS_NULL;
}

JSValue *XMLBackend::lookup(Node const &n)
{
    if(auto it = nodes.find(static_cast<pugi::xml_node_struct *>(n.node)); it != std::end(nodes))
        return &it->second;
    return nullptr;
}

std::optional<Node> XMLBackend::next(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto sb = nn.next_sibling()) return Node{shared_from_this(), sb.internal_object()};
    return std::nullopt;
}

std::optional<Node> XMLBackend::parent(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto pn = nn.parent(); pn &&  pugi::node_element == pn.type()) return Node{shared_from_this(), pn.internal_object()};
    return std::nullopt;
}

JSValue XMLBackend::nextElementSibling(Node const &n)
{
    if(pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)}; !is_connected(nn))
        return JS_NULL;
    else for(auto sb = nn.next_sibling(); sb && is_connected(sb); sb = sb.next_sibling())
        if(pugi::node_element == sb.type()) return make(sb.internal_object());
    return JS_NULL;
}

JSValue XMLBackend::previousElementSibling(Node const &n)
{
    if(pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)}; !is_connected(nn))
        return JS_NULL;
    else for(auto sb = nn.previous_sibling(); sb && is_connected(sb); sb = sb.previous_sibling())
        if(pugi::node_element == sb.type()) return make(sb.internal_object());
    return JS_NULL;
}

void XMLBackend::removeVoid(Node const &n)
{
    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(is_connected(nn)) nn.root().append_move(nn).internal_object();
}

JSValue XMLBackend::querySelector(Node const &n, std::string_view const &sel, std::optional<std::string> &error)
{
    std::string xpath;
    if(!css_to_xpath(std::begin(sel), std::end(sel), xpath))
        return error = "Unsupported selector", JS_NULL;

    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    if(auto const nodes = nn.select_nodes(xpath.c_str()); nodes.begin() != nodes.end())
        return make(nodes.begin()->node().internal_object());
    return JS_NULL;
}

std::vector<void*> XMLBackend::querySelectorAll(Node const &n, std::string_view const &sel, std::optional<std::string> &error)
{
    std::string xpath;
    std::vector<void*> result;
    if(!css_to_xpath(std::begin(sel), std::end(sel), xpath))
        return error = "Unsupported selector", result;

    pugi::xml_node nn{static_cast<pugi::xml_node_struct *>(n.node)};
    for(auto const node: nn.select_nodes(xpath.c_str()))
        result.push_back(node.node().internal_object());
    return result;
}

HTMLBackend::HTMLBackend(JSContext *ctx, JSValue val, lxb_html_document_t *doc)
: Backend(ctx, val), doc{doc}
{
    if(LXB_STATUS_OK != lxb_style_init(doc)) return;

    lxb_dom_document_t *dom = lxb_dom_interface_document(doc);
    dom->attr_mutation = &lxb_dom_document_attr_mutation_cbs;
    dom->mutation = &lxb_dom_document_mutation_cbs;
    dom->user = this;
}

JSValue HTMLBackend::appendChild(Node const &p, Node const &c)
{
    lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(p.node), static_cast<lxb_dom_node_t *>(c.node));
    return make(static_cast<lxb_dom_node_t *>(c.node));
}

JSValue HTMLBackend::cloneNode(Node const &n, bool deep)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    return make(lxb_dom_node_clone(nn, deep));
}

JSValue HTMLBackend::compareDocumentPosition(Node const &n, Node const &o)
{
    if(n.doc != o.doc) return bridge::Number{ctx, 1};
    if(n.node == o.node) return bridge::Number{ctx, 0};

    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    auto *on = static_cast<lxb_dom_node_t *>(o.node);

    std::vector<lxb_dom_node_t *> opath;
    for(lxb_dom_node_t *x = on; x; x = x->parent) {
        if(x == nn) return bridge::Number{ctx, 8 | 2}; // CONTAINS
        opath.push_back(x);
    }

    std::vector<lxb_dom_node_t *> npath;
    for(lxb_dom_node_t *x = nn; x; x = x->parent) {
        if(x == on) return bridge::Number{ctx, 16 | 4}; // CONTAINED_BY
        npath.push_back(x);
    }

    auto ot = std::rbegin(opath);
    auto nt = std::rbegin(npath);

    for(;ot != std::rend(opath) && nt != std::rend(npath); ++ot, ++nt)
    {
        if(*ot == *nt) continue;
        for(lxb_dom_node_t *n = (*ot)->next; n; n = n->next)
            if(n == *nt) return bridge::Number{ctx, 4}; // FOLLOWING
        for(lxb_dom_node_t *n = (*nt)->next; n; n = n->next)
            if(n == *ot) return bridge::Number{ctx, 2}; // PRECEEDING
        break;
    }

    return bridge::Number{ctx, 1};
}

bool HTMLBackend::contains(Node const &n, Node const &o)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    auto *on = static_cast<lxb_dom_node_t *>(o.node);

    for(lxb_dom_node_t *x = on; x; x = x->parent)
        if(x == nn) return true;
    return false;
}

JSValue HTMLBackend::firstChild(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *fc = lxb_dom_node_first_child(nn);
    return fc ? make(fc) : JS_NULL;
}

JSValue HTMLBackend::getRootNode(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    while(nn->parent != nullptr) nn = nn->parent;
    return make(nn);
}

JSValue HTMLBackend::hasChildNodes(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    return lxb_dom_node_first_child(nn) ? JS_TRUE : JS_FALSE;
}

JSValue HTMLBackend::insertBefore(Node const &n, Node const &o, Node const &r)
{
    lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *on = static_cast<lxb_dom_node_t *>(o.node);
    lxb_dom_node_t *rn = static_cast<lxb_dom_node_t *>(r.node);

    lxb_dom_node_insert_before_spec(nn, on, rn);
    return make(on);
}

void HTMLBackend::insertBeforeVoid(Node const &n, Node const &o, Node const &r)
{
    lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *on = static_cast<lxb_dom_node_t *>(o.node);
    lxb_dom_node_t *rn = static_cast<lxb_dom_node_t *>(r.node);

    lxb_dom_node_insert_before_spec(nn, on, rn);
}

bool HTMLBackend::isChild(Node const &n, Node const &c)
{
    lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *cn = static_cast<lxb_dom_node_t *>(c.node);
    return cn->parent == nn;
}

JSValue HTMLBackend::isDefaultNamespace(Node const &, std::string_view const &sv)
{
    return sv == "http://www.w3.org/1999/xhtml" ? JS_TRUE : JS_FALSE;
}

JSValue HTMLBackend::isConnected(Node const &n)
{
    auto const *node = static_cast<lxb_dom_node_t *>(n.node);
    return node->type == LXB_DOM_NODE_TYPE_DOCUMENT || node->parent ? JS_TRUE : JS_FALSE;
}

JSValue HTMLBackend::isEqualNode(Node const &n, Node const &o)
{
    return is_equal_node(
        static_cast<lxb_dom_node_t *>(n.node),
        static_cast<lxb_dom_node_t *>(o.node)
    ) ? JS_TRUE : JS_FALSE;
}

JSValue HTMLBackend::lastChild(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *lc = lxb_dom_node_last_child(nn);
    return lc ? make(lc) : JS_NULL;
}

JSValue HTMLBackend::namespaceURI(Node const &n)
{
    size_t len;
    auto const *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_char_t const *uri = lxb_ns_by_id(doc->dom_document.ns, nn->ns, &len);
    return uri ? bridge::String{ctx, std::string_view{(char const *)uri, len}} : JS_NULL;
}

JSValue HTMLBackend::nextSibling(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    return nn->next ? make(nn->next) : JS_NULL;
}

JSValue HTMLBackend::nodeName(Node const &n)
{
    auto const *nn = static_cast<lxb_dom_node_t *>(n.node);
    switch(nn->type)
    {
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
        return bridge::String{ctx, NN_CDATA};
    case LXB_DOM_NODE_TYPE_COMMENT:
        return bridge::String{ctx, NN_COMMENT};
    case LXB_DOM_NODE_TYPE_DOCUMENT:
        return bridge::String{ctx, NN_DOCUMENT};
    case LXB_DOM_NODE_TYPE_ELEMENT:
        return bridge::String{ctx, lexbor::get_name(lxb_dom_interface_element(nn))};
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        return bridge::String{ctx, lexbor::get_name(nn->local_name)};
    case LXB_DOM_NODE_TYPE_TEXT:
        return bridge::String{ctx, NN_TEXT};
    case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE:
        return bridge::String{ctx, lexbor::get_name(lxb_dom_interface_document_type(nn))};
    default:
        break;
    }
    return JS_NULL;
}

JSValue HTMLBackend::nodeType(Node const &n)
{
    auto const *node = static_cast<lxb_dom_node_t *>(n.node);
    return bridge::Number{ctx, node->type};
}

JSValue HTMLBackend::nodeValue(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    switch(nn->type)
    {
    case LXB_DOM_NODE_TYPE_COMMENT:
    case LXB_DOM_NODE_TYPE_TEXT:
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        return bridge::String{ctx, lexbor::get_text(nn)};
    default:
        break;
    }
    return JS_NULL;
}

void HTMLBackend::nodeValue(Node const &n, std::string_view const &text)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    switch(nn->type)
    {
    case LXB_DOM_NODE_TYPE_COMMENT:
    case LXB_DOM_NODE_TYPE_TEXT:
    case LXB_DOM_NODE_TYPE_CDATA_SECTION:
    case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
        lxb_dom_node_text_content_set(nn, (lxb_char_t const *)text.data(), text.size());
    default:
        break;
    }
}

void HTMLBackend::normalize(Node const &n)
{
    dom::normalize(static_cast<lxb_dom_node_t *>(n.node), nodes);
}

JSValue HTMLBackend::ownerDocument()
{
    return make(lxb_dom_interface_node(doc.get()));
}

JSValue HTMLBackend::parentElement(Node const &n)
{
    if(lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(n.node); nn->parent && (
        LXB_DOM_NODE_TYPE_ELEMENT == nn->parent->type
        || LXB_DOM_NODE_TYPE_DOCUMENT == nn->parent->type
        || LXB_DOM_NODE_TYPE_DOCUMENT_FRAGMENT == nn->parent->type
    )) return make(nn->parent);
    return JS_NULL;
}

JSValue HTMLBackend::parentNode(Node const &n)
{
    if(lxb_dom_node_t *nn = static_cast<lxb_dom_node_t *>(n.node); nn->parent)
        return make(nn->parent);
    return JS_NULL;
}

JSValue HTMLBackend::previousSibling(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    return nn->prev ? make(nn->prev) : JS_NULL;
}

JSValue HTMLBackend::remove(Node const &o)
{
    auto *on = static_cast<lxb_dom_node_t *>(o.node);
    lxb_dom_node_remove(on);
    return make(on);
}

JSValue HTMLBackend::textContent(Node const &n)
{
    if(auto *nn = static_cast<lxb_dom_node_t *>(n.node);
        LXB_DOM_NODE_TYPE_DOCUMENT != nn->type && LXB_DOM_NODE_TYPE_DOCUMENT_TYPE != nn->type)
        return bridge::String{ctx, lexbor::get_text(nn)};
    return JS_NULL;
}

void HTMLBackend::textContent(Node const &n, std::optional<std::string_view> &&text)
{
    if(auto *nn = static_cast<lxb_dom_node_t *>(n.node);
        LXB_DOM_NODE_TYPE_DOCUMENT != nn->type && LXB_DOM_NODE_TYPE_DOCUMENT_TYPE != nn->type)
        switch(nn->type)
        {
        case LXB_DOM_NODE_TYPE_COMMENT:
        case LXB_DOM_NODE_TYPE_TEXT:
        case LXB_DOM_NODE_TYPE_CDATA_SECTION:
        case LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION:
            if(text) lxb_dom_node_text_content_set(nn, (lxb_char_t const *)text->data(), text->size());
            else lxb_dom_node_remove(nn);
            break;
        default:
            if(text) lexbor::set_text(nn, *text);
            else lexbor::del_text(nn);
        }
}

void HTMLBackend::free(Node const &n)
{
    nodes.erase(static_cast<lxb_dom_node_t *>(n.node));
}

JSValue HTMLBackend::childNodes(Node const &n)
{
    bridge::Array arr{ctx};
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t* n = nn->first_child; n; n = n->next)
        arr.append(make(n));
    return arr;
}

std::uint64_t HTMLBackend::childNodeCount(Node const &n)
{
    std::uint64_t count = 0;
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t* n = nn->first_child; n; n = n->next)
        ++count;
    return count;
}

JSValue HTMLBackend::childNode(Node const &n, std::int64_t i, JSValue d)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t* n = nn->first_child; n; n = n->next, --i)
        if(!i) return make(n);
    return d;
}

JSValue HTMLBackend::childElements(Node const &n)
{
    bridge::Array arr{ctx};
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t* n = nn->first_child; n; n = n->next)
        if(LXB_DOM_NODE_TYPE_ELEMENT == n->type) arr.append(make(n));
    return arr;
}

JSValue HTMLBackend::childElement(Node const &n, std::int64_t i, JSValue d)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t* n = nn->first_child; n; n = n->next)
        if(LXB_DOM_NODE_TYPE_ELEMENT == n->type && !i--) return make(n);
    return d;
}

JSValue HTMLBackend::childElement(Node const &n, std::string_view const &name)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    lxb_dom_node_t *candidate{nullptr};
    std::size_t nlen;

    for(lxb_dom_node_t* ch = nn->first_child; ch; ch = ch->next)
    {
        if(LXB_DOM_NODE_TYPE_ELEMENT != ch->type) continue;
        for(auto *attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(ch)); attr; attr = lxb_dom_element_next_attribute(attr))
        {
            if(attr->node.ns != LXB_NS_HTML) continue;
            if(char const *nstr = reinterpret_cast<char const *>(lxb_dom_attr_qualified_name(attr, &nlen));
                std::string_view{nstr, nlen} == "id")
            {
                char const *vstr = reinterpret_cast<char const *>(lxb_dom_attr_value(attr, &nlen));
                if(name == std::string_view{vstr, nlen}) return make(ch);
            }
            else if(std::string_view{nstr, nlen} == "name")
            {
                char const *vstr = reinterpret_cast<char const *>(lxb_dom_attr_value(attr, &nlen));
                if(name == std::string_view{vstr, nlen}) candidate = ch;
            }
        }
    }
    return candidate ? make(candidate) : JS_NULL;
}

JSValue HTMLBackend::collection(std::vector<void*> const &nodes)
{
    bridge::Array arr{ctx};
    for(void *node: nodes) arr.append(make(static_cast<lxb_dom_node_t *>(node)));
    return arr;
}

JSValue HTMLBackend::collection_item(std::vector<void*> const &nodes, std::int64_t i, JSValue def)
{
    return i < nodes.size() ? make(static_cast<lxb_dom_node_t *>(nodes[i])) : def;
}

void HTMLBackend::appendChildVoid(Node const &p, Node const &c)
{
    lxb_dom_node_append_child(static_cast<lxb_dom_node_t *>(p.node), static_cast<lxb_dom_node_t *>(c.node));
}

std::uint64_t HTMLBackend::childElementCount(Node const &n)
{
    std::uint64_t count{0};
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t *ch = nn->first_child; ch; ch = ch->next)
        if(LXB_DOM_NODE_TYPE_ELEMENT == ch->type) ++count;
    return count;
}

Node HTMLBackend::createTextNode(std::string_view const &text)
{
    return Node{shared_from_this(), lxb_dom_interface_node(lxb_dom_document_create_text_node(
        lxb_dom_interface_document(doc.get()),
        (lxb_char_t const *)text.data(), text.size()
    ))};
}

std::optional<Node> HTMLBackend::first(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    if(lxb_dom_node_t *ch = nn->first_child) return Node{shared_from_this(), ch};
    return std::nullopt;
}

JSValue HTMLBackend::firstElementChild(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t *ch = nn->first_child; ch; ch = ch->next)
        if(LXB_DOM_NODE_TYPE_ELEMENT == ch->type)
            return make(ch);
    return JS_NULL;
}

JSValue HTMLBackend::lastElementChild(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(lxb_dom_node_t *ch = nn->last_child; ch; ch = ch->prev)
        if(LXB_DOM_NODE_TYPE_ELEMENT == ch->type)
            return make(ch);
    return JS_NULL;
}

JSValue *HTMLBackend::lookup(Node const &n)
{
    if(auto it = nodes.find(static_cast<lxb_dom_node_t *>(n.node)); it != std::end(nodes))
        return &it->second;
    return nullptr;
}

JSValue HTMLBackend::nextElementSibling(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(auto *sb = nn->next; sb; sb = sb->next)
        if(LXB_DOM_NODE_TYPE_ELEMENT == sb->type)
            return make(sb);
    return JS_NULL;
}

std::optional<Node> HTMLBackend::next(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    if(auto *sb = nn->next) return Node{shared_from_this(), sb};
    return std::nullopt;
}

std::optional<Node> HTMLBackend::parent(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    if(auto pn = nn->parent; pn && LXB_DOM_NODE_TYPE_ELEMENT == pn->type) return Node{shared_from_this(), pn};
    return std::nullopt;
}

JSValue HTMLBackend::previousElementSibling(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    for(auto *sb = nn->prev; sb; sb = sb->prev)
        if(LXB_DOM_NODE_TYPE_ELEMENT == sb->type)
            return make(sb);
    return JS_NULL;
}

void HTMLBackend::removeVoid(Node const &n)
{
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);
    if(nn->parent) lxb_dom_node_remove(nn);
}

JSValue HTMLBackend::querySelector(Node const &n, std::string_view const &s, std::optional<std::string> &err)
{
    lxb_dom_node_t* result{nullptr};
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);

    auto *css = lxb_dom_interface_document(doc.get())->css;
    if(auto *lst = lxb_css_selectors_parse(css->parser, (const lxb_char_t *)s.data(), s.size()))
    {
        lxb_selectors_find(css->selectors, nn, lst, find_node, &result);
        lxb_css_selector_list_destroy(lst);
    }
    return result ? make(result) : JS_NULL;
}

std::vector<void*> HTMLBackend::querySelectorAll(Node const &n, std::string_view const &s, std::optional<std::string> &err)
{
    std::vector<void*> result;
    auto *nn = static_cast<lxb_dom_node_t *>(n.node);

    auto *css = lxb_dom_interface_document(doc.get())->css;
    if(auto *lst = lxb_css_selectors_parse(css->parser, (const lxb_char_t *)s.data(), s.size()))
    {
        lxb_selectors_find(css->selectors, nn, lst, append_node, &result);
        lxb_css_selector_list_destroy(lst);
    }
    return result;
}

std::uintptr_t HTMLBackend::lookupNS(std::string_view const &n) const
{
    const lxb_ns_data_t *data = lxb_ns_data_by_link(doc->dom_document.ns, (lxb_char_t const *)n.data(), n.size());
    return data ? data->ns_id : LXB_NS__UNDEF;
}

std::optional<std::string_view> HTMLBackend::lookupNS(uintptr_t ns) const
{
    size_t len;
    std::optional<std::string_view> result;
    if(const lxb_char_t *uri = lxb_ns_by_id(doc->dom_document.ns, ns, &len); uri)
        result.emplace((char const *)uri, len);
    return result;
}

} // namespace notojs:dom
