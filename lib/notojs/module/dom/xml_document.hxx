struct XMLDocument : bridge::Interface<XMLDocument, dom::XMLDocument, Document>
{
    XMLDocument(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    XMLDocument(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    BOOST_FORCEINLINE static void free(dom::XMLDocument &self)
    {
        self.doc->self.reset();
    }

    JSValue createElement(JSContext *ctx, bridge::String name)
    {
        return ref().createElement(name);
    }

    JSValue createTextNode(JSContext *ctx, bridge::String text)
    {
        return ref().createTextNode(text);
    }

    JSValue documentElement(JSContext *) const
    {
        return ref().documentElement();
    }

    JSValue getElementsByTagName(JSContext *ctx, bridge::String name)
    {
        std::optional<std::string> error;
        return HTMLCollection::from(ctx, dom::HTMLCollection{
            dom::Node{ref().doc, ref().node},
            dom::Element::isTagName(name)
                ? ref().doc->querySelectorAll(ref(), name, error)
                : std::vector<void*>()
        });
    }

    JSValue toJSON(JSContext *ctx) const
    {
        return XML{ctx, bridge::Strong<void>{ctx, XML::data(ctx, toString(ctx)), false}}.toJSON(ctx);
    }

    JSValue toString(JSContext *ctx) const
    {
        return bridge::String{ctx, ref().toString()};
    }

    struct I : Base::I<I, XML::Interface>
    {
        using Base::Base;
        std::string get() const override
        {
            return ref.toString();
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<XMLDocument>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const XMLDocument::funcs[] = {
    // XMLDocumented interface
    JS_CGETSET_DEF("documentElement", &bridge::Getter<&XMLDocument::documentElement>, NULL),

    JS_CFUNC_DEF("createElement", 1, &bridge::Function<&XMLDocument::createElement>::invoke),
    JS_CFUNC_DEF("createTextNode", 1, &bridge::Function<&XMLDocument::createTextNode>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&XMLDocument::getElementsByTagName>::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 1, &bridge::Function<&XMLDocument::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<XMLDocument>::toJSON)
};
