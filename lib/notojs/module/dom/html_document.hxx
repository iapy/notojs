struct HTMLDocument : bridge::Interface<HTMLDocument, dom::HTMLDocument, Document>
{
    HTMLDocument(JSContext *ctx, JSValue self, std::monostate)
    : Base{dom::HTMLDocument{std::make_shared<dom::HTMLBackend>(ctx, self, ::lxb_html_document_create())}} {}

    JSValue _live(JSContext *ctx) const
    {
        return bridge::Number{ctx, static_cast<std::uint64_t>(dynamic_cast<dom::HTMLBackend*>(ref().doc.get())->live.size())};
    }

    JSValue body(JSContext *) const
    {
        return ref().body();
    }

    template<bool lower>
    JSValue createAttribute(JSContext *ctx, bridge::String name)
    {
        auto n = static_cast<std::string>(name);
        if(n == "*" || !dom::Element::isTagName(n))
            return DOMException::throwInvalidCharacterError(ctx);
        if constexpr (lower) std::transform(std::begin(n), std::end(n), std::begin(n),
            [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
        return Attr::from(ctx, dom::Attr{ref().doc->shared_from_this(), dom::Attr::Name{std::move(n)}, std::string{}});
    }

    JSValue createAttributeNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return createAttribute<false>(ctx, name);
    }

    JSValue createAttributeNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = ref().lookupNS(nsv); LXB_NS__UNDEF != nsid)
        {
            auto n = static_cast<std::string>(name);
            if(n == "*" || !dom::Element::isTagName(n))
                return DOMException::throwInvalidCharacterError(ctx);
            return Attr::from(ctx, dom::Attr{ref().doc->shared_from_this(), dom::Attr::Name{std::move(n), nsid}, std::string{}});
        }
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using createAttributeNS = bridge::Function
    <
        &HTMLDocument::createAttributeNS_0,
        &HTMLDocument::createAttributeNS_1
    >;

    JSValue createCDATASection(JSContext *ctx, bridge::String)
    {
        return DOMException::throwNotSupportedError(ctx);
    }

    JSValue createComment(JSContext *, bridge::String text)
    {
        return ref().createComment(text);
    }

    JSValue createProcessingInstruction(JSContext *ctx, bridge::String target, bridge::String data)
    {
        auto const &t = static_cast<std::string_view const &>(target);
        auto const &d = static_cast<std::string_view const &>(data);
        if(!dom::Element::isTagName(t) || d.find("?>") != std::string_view::npos)
            return DOMException::throwInvalidCharacterError(ctx);
        return ref().createProcessingInstruction(t, d);
    }

    JSValue createElement(JSContext *ctx, bridge::String name)
    {
        return ref().createElement(name);
    }

    JSValue createDocumentFragment(JSContext *)
    {
        return ref().createDocumentFragment();
    }

    JSValue createElementNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return createElement(ctx, name);
    }

    JSValue createElementNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = ref().lookupNS(nsv); LXB_NS__UNDEF != nsid)
            return ref().createElement(name, nsid);
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using createElementNS = bridge::Function
    <
        &HTMLDocument::createElementNS_0,
        &HTMLDocument::createElementNS_1
    >;

    JSValue createTextNode(JSContext *ctx, bridge::String text)
    {
        return ref().createTextNode(text);
    }

    JSValue documentElement(JSContext *) const
    {
        return ref().documentElement();
    }

    JSValue getElementById(JSContext *, bridge::String value)
    {
        return ref().getElementById(value);
    }

    JSValue getElementsByTagName(JSContext *ctx, bridge::String name)
    {
        return dom::Element::isTagName(name)
            ? HTMLCollection::from(ctx, dom::HTMLCollection{
                dom::Node{ref().doc, ref().node}, name
            })
            : HTMLCollection::from(ctx, dom::HTMLCollection{
                dom::Node{ref().doc, ref().node}, std::vector<void*>{}
            });
    }

    JSValue getElementsByTagNameNS_0(JSContext *ctx, bridge::Null, bridge::String name)
    {
        return getElementsByTagName(ctx, name);
    }

    JSValue getElementsByTagNameNS_1(JSContext *ctx, bridge::String ns, bridge::String name)
    {
        auto const &nsv = static_cast<std::string_view const &>(ns);
        if(auto nsid = ref().lookupNS(nsv); LXB_NS__UNDEF != nsid)
            return dom::Element::isTagName(name)
                ? HTMLCollection::from(ctx, dom::HTMLCollection{
                    dom::Node{ref().doc, ref().node}, name, nsid
                })
                : HTMLCollection::from(ctx, dom::HTMLCollection{
                    dom::Node{ref().doc, ref().node}, std::vector<void*>{}
                });
        return DOMException::throwNamespaceError(ctx, "Unsupported namespace: " + std::string(nsv.data(), nsv.size()));
    }

    using getElementsByTagNameNS = bridge::Function
    <
        &HTMLDocument::getElementsByTagNameNS_0,
        &HTMLDocument::getElementsByTagNameNS_1
    >;

    JSValue getElementsByClassName(JSContext *ctx, bridge::String name)
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{
            dom::Node{ref().doc, ref().node}, dom::HTMLElement::className(name)
        });
    }

    JSValue forms(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{
            dom::Node{ref().doc, ref().node}, "form"
        });
    }

    JSValue head(JSContext *) const
    {
        return ref().head();
    }

    JSValue namespaceURI(JSContext *) const
    {
        return JS_NULL;
    }

    JSValue title(JSContext *ctx) const
    {
        if(auto title = ref().title())
            return bridge::String{ctx, std::move(*title)};
        return bridge::String{ctx, std::string_view{""}};
    }

    void set_title_0(JSContext *ctx, bridge::Null)
    {
        ref().title(std::nullopt);
    }

    void set_title_1(JSContext *ctx, bridge::String data)
    {
        ref().title(data);
    }

    using set_title = bridge::Setters
    <
        &HTMLDocument::set_title_0,
        &HTMLDocument::set_title_1
    >;

    JSValue toJSON(JSContext *ctx) const
    {
        return JS_ThrowTypeError(ctx, "HTMLDocument cannot be serialized");
    }

    JSValue toString(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().toString()};
    }

    BOOST_FORCEINLINE static void free(dom::HTMLDocument &self)
    {
        self.doc->self.reset();
    }

    struct I : Base::I<I, HTML::Interface>
    {
        using Base::Base;
        bool json() const override
        {
            return false;
        }
        std::string get() const override
        {
            return ref.toString();
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<HTMLDocument>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLDocument::funcs[] = {
    // Documented interface
    JS_CGETSET_DEF("body", &bridge::Getter<&HTMLDocument::body>, NULL),
    JS_CGETSET_DEF("documentElement", &bridge::Getter<&HTMLDocument::documentElement>, NULL),
    JS_CGETSET_DEF("head", &bridge::Getter<&HTMLDocument::head>, NULL),
    JS_CGETSET_DEF("forms", &bridge::Getter<&HTMLDocument::forms>, NULL),
    JS_CGETSET_DEF("namespaceURI", &bridge::Getter<&HTMLDocument::namespaceURI>, NULL),
    JS_CGETSET_DEF("title", &bridge::Getter<&HTMLDocument::title>, &HTMLDocument::set_title::invoke),

    JS_CFUNC_DEF("createAttribute", 1, &bridge::Function<&HTMLDocument::createAttribute<true>>::invoke),
    JS_CFUNC_DEF("createAttributeNS", 2, &HTMLDocument::createAttributeNS::invoke),
    JS_CFUNC_DEF("createCDATASection", 1, &bridge::Function<&HTMLDocument::createCDATASection>::invoke),
    JS_CFUNC_DEF("createComment", 1, &bridge::Function<&HTMLDocument::createComment>::invoke),
    JS_CFUNC_DEF("createProcessingInstruction", 2, &bridge::Function<&HTMLDocument::createProcessingInstruction>::invoke),
    JS_CFUNC_DEF("createElement", 1, &bridge::Function<&HTMLDocument::createElement>::invoke),
    JS_CFUNC_DEF("createElementNS", 2, &HTMLDocument::createElementNS::invoke),
    JS_CFUNC_DEF("createDocumentFragment", 0, &bridge::Function<&HTMLDocument::createDocumentFragment>::invoke),
    JS_CFUNC_DEF("createTextNode", 1, &bridge::Function<&HTMLDocument::createTextNode>::invoke),
    JS_CFUNC_DEF("getElementById", 1, &bridge::Function<&HTMLDocument::getElementById>::invoke),
    JS_CFUNC_DEF("getElementsByClassName", 1, &bridge::Function<&HTMLDocument::getElementsByClassName>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&HTMLDocument::getElementsByTagName>::invoke),
    JS_CFUNC_DEF("getElementsByTagNameNS", 2, &HTMLDocument::getElementsByTagNameNS::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 1, &bridge::Function<&HTMLDocument::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<HTMLDocument>::toJSON),
    JS_CGETSET_DEF("_live", &bridge::Getter<&HTMLDocument::_live>, NULL)
};
