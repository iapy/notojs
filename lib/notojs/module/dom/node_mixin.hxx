struct NodeMixin
{
    using Tail = bridge::Tail<1, Node, bridge::String>;

    static JSValue childElementCount(Node const &node, JSContext *ctx)
    {
        return bridge::Number{ctx, node.ref().doc->childElementCount(node.ref())};
    }

    static JSValue children(Node const &node, JSContext *ctx)
    {
        if(auto it = node.ref().doc->child_e.find(node.ref().node); it != std::end(node.ref().doc->child_e))
            return JS_DupValue(ctx, it->second);
        return node.ref().doc->child_e[node.ref().node] = HTMLCollection::from(ctx, HTMLCollection::Wrapped{dom::Node{node.ref().doc, node.ref().node}});
    }

    static JSValue firstElementChild(Node const &node, JSContext *)
    {
        return node.ref().doc->firstElementChild(node.ref());
    }

    static JSValue lastElementChild(Node const &node, JSContext *)
    {
        return node.ref().doc->lastElementChild(node.ref());
    }

    static JSValue remove(Node const &node, JSContext *)
    {
        return node.ref().doc->removeVoid(node.ref()), JS_UNDEFINED;
    }

    static JSValue querySelector(Node const &node, JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto n = node.ref().doc->querySelector(node.ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return n;
    }

    static JSValue querySelectorAll(Node const &node, JSContext *ctx, bridge::String query)
    {
        std::optional<std::string> error;
        if(auto result = node.ref().doc->querySelectorAll(node.ref(), query, error); error)
            return DOMException::throwSyntaxError(ctx, std::move(error));
        else return NodeList::from(ctx, dom::NodeList{node.ref().doc->shared_from_this(), std::move(result)});
    }

    static BOOST_FORCEINLINE std::optional<JSValue> after_f(dom::Node const &, JSContext *, Tail &, std::size_t,
        dom::Node const &, std::optional<dom::Node> const &)
    {
        return std::nullopt;
    }

    static BOOST_FORCEINLINE std::optional<JSValue> append_f(dom::Node const &, JSContext *, Tail &, std::size_t)
    {
        return std::nullopt;
    }

    static BOOST_FORCEINLINE std::optional<JSValue> before_f(dom::Node const &, JSContext *, Tail &, std::size_t,
        dom::Node const &)
    {
        return std::nullopt;
    }

    static BOOST_FORCEINLINE std::optional<JSValue> prepare_f(
        dom::Node const &, JSContext *, Tail &, std::size_t)
    {
        return std::nullopt;
    }

    static BOOST_FORCEINLINE void *prepared_f(Tail &, std::size_t)
    {
        return nullptr;
    }

    template<typename Fallback = NodeMixin>
    static std::optional<JSValue> validate_nodes(
        dom::Node const &parent, JSContext *ctx, typename Fallback::Tail &nodes)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto node = nodes.template get<Node>(i); node)
            {
                if(parent.doc != node->ref().doc)
                    return DOMException::throwWrongDocumentError(ctx);

                if(parent.doc->contains(node->ref(), parent))
                    return DOMException::throwHierarchyRequestError(ctx);
            }
        }

        return std::nullopt;
    }

    template<typename Fallback = NodeMixin>
    static std::optional<JSValue> validate_mutation(
        dom::Node const &parent, JSContext *ctx, typename Fallback::Tail &nodes,
        dom::Node const *reference = nullptr,
        std::vector<dom::Node const *> removed = {})
    {
        if(auto error = validate_nodes<Fallback>(parent, ctx, nodes)) return error;

        for(std::size_t i = 0; i < nodes.size(); ++i)
            if(auto error = Fallback::prepare_f(parent, ctx, nodes, i)) return error;

        std::vector<dom::Node> prepared;
        prepared.reserve(nodes.size());

        dom::MutationPlan plan{parent, reference, std::move(removed), {}};
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto node = nodes.template get<Node>(i); node)
            {
                plan.inserted.push_back({
                    &node->ref(), node->ref().doc->nodeType(node->ref())
                });
            }
            else if(nodes.template get<bridge::String>(i))
            {
                plan.inserted.push_back({nullptr, 3});
            }
            else if(auto *fragment = Fallback::prepared_f(nodes, i))
            {
                prepared.emplace_back(parent.doc, fragment);
                plan.inserted.push_back({&prepared.back(), 11});
            }
        }

        if(!plan) return DOMException::throwHierarchyRequestError(ctx);
        return std::nullopt;
    }

    template<typename Fallback = NodeMixin>
    static bool moves_node(typename Fallback::Tail &nodes, dom::Node const &candidate)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto node = nodes.template get<Node>(i); node)
            {
                if(node->ref().node == candidate.node) return true;
                if(11 == node->ref().doc->nodeType(node->ref()))
                {
                    for(auto child = node->ref().doc->first(node->ref()); child;
                        child = node->ref().doc->next(*child))
                    {
                        if(child->node == candidate.node) return true;
                    }
                }
            }
        }
        return false;
    }

    template<typename Fallback = NodeMixin>
    static void detach_nodes(typename Fallback::Tail &nodes)
    {
        std::vector<void *> detached;
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto node = nodes.template get<Node>(i); node
                && 11 != node->ref().doc->nodeType(node->ref())
                && std::find(std::begin(detached), std::end(detached), node->ref().node)
                    == std::end(detached))
            {
                node->ref().doc->removeVoid(node->ref());
                detached.push_back(node->ref().node);
            }
        }
    }

    template<typename Fallback = NodeMixin>
    static std::optional<dom::Node> stable_reference(
        dom::Node const &parent, void *start, typename Fallback::Tail &nodes)
    {
        if(!start) return std::nullopt;

        bool reached = false;
        for(auto current = parent.doc->first(parent); current;
            current = parent.doc->next(*current))
        {
            if(current->node == start) reached = true;
            if(reached && !moves_node<Fallback>(nodes, *current))
                return current;
        }
        return std::nullopt;
    }

    template<typename Fallback = NodeMixin>
    static JSValue append_validated(
        dom::Node const &parent, JSContext *ctx, typename Fallback::Tail &nodes)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto node = nodes.template get<Node>(i); node)
            {
                parent.doc->appendChildVoid(parent, node->ref());
            }
            else if(auto string = nodes.template get<bridge::String>(i); string)
            {
                parent.doc->appendChildVoid(parent, parent.doc->createTextNode(*string));
            }
            else if(auto error = Fallback::append_f(parent, ctx, nodes, i))
            {
                return *error;
            }
        }

        return JS_UNDEFINED;
    }

    template<typename Fallback = NodeMixin>
    static JSValue after_validated(Node const &node, JSContext *ctx, typename Fallback::Tail &nodes,
        dom::Node const &parent, std::optional<dom::Node> const &next)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(next) parent.doc->insertBeforeVoid(parent, n->ref(), *next);
                else parent.doc->appendChildVoid(parent, n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                if(next) parent.doc->insertBeforeVoid(parent, parent.doc->createTextNode(*s), *next);
                else parent.doc->appendChildVoid(parent, parent.doc->createTextNode(*s));
            }
            else if(auto e = Fallback::after_f(node.ref(), ctx, nodes, i, parent, next)) return *e;
        }
        return JS_UNDEFINED;
    }

    template<typename Fallback = NodeMixin>
    static JSValue before_validated(JSContext *ctx, typename Fallback::Tail &nodes,
        dom::Node const &parent, std::optional<dom::Node> const &reference)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(reference) parent.doc->insertBeforeVoid(parent, n->ref(), *reference);
                else parent.doc->appendChildVoid(parent, n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                if(reference) parent.doc->insertBeforeVoid(parent, parent.doc->createTextNode(*s), *reference);
                else parent.doc->appendChildVoid(parent, parent.doc->createTextNode(*s));
            }
            else if(reference)
            {
                if(auto e = Fallback::before_f(*reference, ctx, nodes, i, parent)) return *e;
            }
            else
            {
                if(auto e = Fallback::append_f(parent, ctx, nodes, i)) return *e;
            }
        }
        return JS_UNDEFINED;
    }

    template<typename Fallback = NodeMixin>
    static JSValue prepend_validated(
        dom::Node const &parent, JSContext *ctx, typename Fallback::Tail &nodes,
        std::optional<dom::Node> const &first)
    {
        for(std::size_t i = 0; i < nodes.size(); ++i)
        {
            if(auto n = nodes.template get<Node>(i); n)
            {
                if(first) parent.doc->insertBeforeVoid(parent, n->ref(), *first);
                else parent.doc->appendChildVoid(parent, n->ref());
            }
            else if(auto s = nodes.template get<bridge::String>(i); s)
            {
                if(first) parent.doc->insertBeforeVoid(parent, parent.doc->createTextNode(*s), *first);
                else parent.doc->appendChildVoid(parent, parent.doc->createTextNode(*s));
            }
            else if(first)
            {
                if(auto e = Fallback::before_f(*first, ctx, nodes, i, parent)) return *e;
            }
            else
            {
                if(auto e = Fallback::append_f(parent, ctx, nodes, i)) return *e;
            }
        }
        return JS_UNDEFINED;
    }

    template<typename Fallback = NodeMixin>
    static JSValue append_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        if(auto error = validate_mutation<Fallback>(node.ref(), ctx, nodes)) return *error;
        detach_nodes<Fallback>(nodes);
        return append_validated<Fallback>(node.ref(), ctx, nodes);
    }

    template<typename Fallback = NodeMixin>
    static JSValue after_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        auto parent = node.ref().doc->parent(node.ref());
        if(!parent) return DOMException::throwHierarchyRequestError(ctx);

        auto next = node.ref().doc->next(node.ref());
        if(auto error = validate_mutation<Fallback>(
            *parent, ctx, nodes, next ? &*next : nullptr)) return *error;

        auto reference = stable_reference<Fallback>(
            *parent, next ? next->node : nullptr, nodes);
        detach_nodes<Fallback>(nodes);
        return after_validated<Fallback>(node, ctx, nodes, *parent, reference);
    }

    template<typename Fallback = NodeMixin>
    static JSValue before_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        auto parent = node.ref().doc->parent(node.ref());
        if(!parent) return DOMException::throwHierarchyRequestError(ctx);

        if(auto error = validate_mutation<Fallback>(
            *parent, ctx, nodes, &node.ref())) return *error;

        auto reference = stable_reference<Fallback>(*parent, node.ref().node, nodes);
        detach_nodes<Fallback>(nodes);
        return before_validated<Fallback>(ctx, nodes, *parent, reference);
    }

    template<typename Fallback = NodeMixin>
    static JSValue prepend_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        auto first = node.ref().doc->first(node.ref());
        if(auto error = validate_mutation<Fallback>(
            node.ref(), ctx, nodes, first ? &*first : nullptr)) return *error;

        auto reference = stable_reference<Fallback>(
            node.ref(), first ? first->node : nullptr, nodes);
        detach_nodes<Fallback>(nodes);
        return prepend_validated<Fallback>(node.ref(), ctx, nodes, reference);
    }

    template<typename Fallback = NodeMixin>
    static JSValue replaceChildren_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        std::vector<dom::Node> removedNodes;
        for(auto current = node.ref().doc->first(node.ref()); current;)
        {
            auto next = node.ref().doc->next(*current);
            removedNodes.push_back(std::move(*current));
            current = std::move(next);
        }

        std::vector<dom::Node const *> removed;
        removed.reserve(removedNodes.size());
        for(auto const &child: removedNodes) removed.push_back(&child);

        if(auto error = validate_mutation<Fallback>(
            node.ref(), ctx, nodes, nullptr, std::move(removed))) return *error;

        while(auto first = node.ref().doc->first(node.ref()))
            node.ref().doc->removeVoid(*first);
        detach_nodes<Fallback>(nodes);
        return append_validated<Fallback>(node.ref(), ctx, nodes);
    }

    template<typename Fallback = NodeMixin>
    static JSValue replaceWith_t(Node const &node, JSContext *ctx, typename Fallback::Tail nodes)
    {
        auto parent = node.ref().doc->parent(node.ref());
        if(!parent) return JS_UNDEFINED;

        auto next = node.ref().doc->next(node.ref());
        if(auto error = validate_mutation<Fallback>(
            *parent, ctx, nodes, next ? &*next : nullptr, {&node.ref()})) return *error;

        auto reference = stable_reference<Fallback>(
            *parent, next ? next->node : nullptr, nodes);
        detach_nodes<Fallback>(nodes);
        node.ref().doc->removeVoid(node.ref());
        return after_validated<Fallback>(node, ctx, nodes, *parent, reference);
    }
};

struct HTMLNodeMixin
{
    struct Tail : bridge::Tail<1, Node, bridge::String, HTML, Image, SVG>
    {
        using Base = bridge::Tail<1, Node, bridge::String, HTML, Image, SVG>;

        BOOST_FORCEINLINE Tail(JSContext *ctx, int argc, JSValue *argv)
        : Base{ctx, argc, argv}, prepared_(static_cast<std::size_t>(argc), nullptr) {}

        BOOST_FORCEINLINE void *&prepared(std::size_t i) { return prepared_[i]; }
        BOOST_FORCEINLINE void *prepared(std::size_t i) const { return prepared_[i]; }

    private:
        std::vector<void *> prepared_;
    };

    static std::optional<JSValue> prepare_f(
        dom::Node const &parent, JSContext *ctx, Tail &nodes, std::size_t i)
    {
        std::optional<std::string> error;
        bool prepared = true;
        auto *node = static_cast<lxb_dom_node_t *>(parent.node);

        if(auto html = nodes.template get<HTML>(i); html)
        {
            prepared = dom::lexbor::prepare(node, *html, error);
            nodes.prepared(i) = html->fragment;
        }
        else if(auto image = nodes.template get<Image>(i); image)
        {
            prepared = dom::lexbor::prepare(node, *image, error);
            nodes.prepared(i) = image->fragment;
        }
        else if(auto svg = nodes.template get<SVG>(i); svg)
        {
            prepared = dom::lexbor::prepare(node, *svg, error);
            nodes.prepared(i) = svg->fragment;
        }

        if(!prepared) return DOMException::throwSyntaxError(ctx, std::move(error));
        return std::nullopt;
    }

    static void *prepared_f(Tail &nodes, std::size_t i)
    {
        return nodes.prepared(i);
    }

    static std::optional<JSValue> after_f(dom::Node const &, JSContext *, Tail &nodes, std::size_t j,
        dom::Node const &parent, std::optional<dom::Node> const &next)
    {
        auto *node = static_cast<lxb_dom_node_t *>(parent.node);
        auto *reference = next ? static_cast<lxb_dom_node_t *>(next->node) : nullptr;

        if(auto html = nodes.template get<HTML>(j); html)
        {
            html->fragment = nodes.prepared(j);
            if(reference) (void)dom::lexbor::insertPrepared(node, *html, reference);
            else (void)dom::lexbor::appendPrepared(node, *html);
        }
        else if(auto image = nodes.template get<Image>(j); image)
        {
            image->fragment = nodes.prepared(j);
            if(reference) (void)dom::lexbor::insertPrepared(node, *image, reference);
            else (void)dom::lexbor::appendPrepared(node, *image);
        }
        else if(auto svg = nodes.template get<SVG>(j); svg)
        {
            svg->fragment = nodes.prepared(j);
            if(reference) (void)dom::lexbor::insertPrepared(node, *svg, reference);
            else (void)dom::lexbor::appendPrepared(node, *svg);
        }
        return std::nullopt;
    }

    static std::optional<JSValue> append_f(dom::Node const &self, JSContext *, Tail &nodes, std::size_t j)
    {
        auto *node = static_cast<lxb_dom_node_t *>(self.node);
        if(auto html = nodes.template get<HTML>(j); html)
        {
            html->fragment = nodes.prepared(j);
            (void)dom::lexbor::appendPrepared(node, *html);
        }
        else if(auto image = nodes.template get<Image>(j); image)
        {
            image->fragment = nodes.prepared(j);
            (void)dom::lexbor::appendPrepared(node, *image);
        }
        else if(auto svg = nodes.template get<SVG>(j); svg)
        {
            svg->fragment = nodes.prepared(j);
            (void)dom::lexbor::appendPrepared(node, *svg);
        }
        return std::nullopt;
    }

    static std::optional<JSValue> before_f(dom::Node const &self, JSContext *, Tail &nodes, std::size_t j,
        dom::Node const &parent)
    {
        auto *node = static_cast<lxb_dom_node_t *>(parent.node);
        auto *reference = static_cast<lxb_dom_node_t *>(self.node);
        if(auto html = nodes.template get<HTML>(j); html)
        {
            html->fragment = nodes.prepared(j);
            (void)dom::lexbor::insertPrepared(node, *html, reference);
        }
        else if(auto image = nodes.template get<Image>(j); image)
        {
            image->fragment = nodes.prepared(j);
            (void)dom::lexbor::insertPrepared(node, *image, reference);
        }
        else if(auto svg = nodes.template get<SVG>(j); svg)
        {
            svg->fragment = nodes.prepared(j);
            (void)dom::lexbor::insertPrepared(node, *svg, reference);
        }
        return std::nullopt;
    }

    template<typename T>
    static JSValue appendChild_t(Node const &self, JSContext *ctx, T object)
    {
        std::optional<std::string> error;
        if(auto *node = dom::lexbor::appendChild(static_cast<lxb_dom_node_t *>(self.ref().node), object, error))
            return dynamic_cast<dom::HTMLBackend *>(self.ref().doc.get())->make(node);
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }

    template<typename T>
    static JSValue insertBefore_t0(Node const &self, JSContext *ctx, T object, Node r)
    {
        if(self.ref().doc != r.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!self.ref().doc->isChild(self.ref(), r.ref())) return DOMException::throwNotFoundError(ctx);

        std::optional<std::string> error;
        if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(self.ref().node), object, static_cast<lxb_dom_node_t *>(r.ref().node), error))
            return dynamic_cast<dom::HTMLBackend *>(self.ref().doc.get())->make(node);
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }

    template<typename T>
    static JSValue insertBefore_t1(Node const &self, JSContext *ctx, T object, bridge::Null)
    {
        return appendChild_t<T>(self, ctx, object);
    }

    template<typename T>
    static JSValue replaceChild_t(Node const &self, JSContext *ctx, T object, Node o)
    {
        if(self.ref().doc != o.ref().doc) return DOMException::throwWrongDocumentError(ctx);
        if(!self.ref().doc->isChild(self.ref(), o.ref())) return DOMException::throwNotFoundError(ctx);

        std::optional<std::string> error;
        if(auto *node = dom::lexbor::insertBefore(static_cast<lxb_dom_node_t *>(self.ref().node), object, static_cast<lxb_dom_node_t *>(o.ref().node), error))
            return self.ref().doc->remove(o.ref());
        return DOMException::throwSyntaxError(ctx, std::move(error));
    }
};
