struct DOMRect : bridge::Interface<DOMRect, dom::DOMRect>
{
    DOMRect(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    DOMRect(std::reference_wrapper<dom::DOMStringMap> &&rw) : Base(std::move(rw)) {}

    JSValue x(JSContext *ctx) const
    {
        std::uint32_t x{0};
        if(auto p = ref().getAttribute(ref().attr))
        {
            std::istringstream iss{std::string{p->data(), p->size()}};
            iss >> x;
        }
        return bridge::Number{ctx, x};
    }

    JSValue y(JSContext *ctx) const
    {
        std::uint32_t y{0};
        if(auto p = ref().getAttribute(ref().attr))
        {
            std::istringstream iss{std::string{p->data(), p->size()}};
            iss >> y >> y;
        }
        return bridge::Number{ctx, y};
    }

    JSValue width(JSContext *ctx) const
    {
        std::uint32_t w{0};
        if(auto p = ref().getAttribute(ref().attr))
        {
            std::istringstream iss{std::string{p->data(), p->size()}};
            iss >> w >> w >> w;
        }
        return bridge::Number{ctx, w ? w : 1};
    }

    JSValue height(JSContext *ctx) const
    {
        std::uint32_t h;
        if(auto p = ref().getAttribute(ref().attr))
        {
            std::istringstream iss{std::string{p->data(), p->size()}};
            iss >> h >> h >> h >> h;
        }
        return bridge::Number{ctx, h ? h : 1};
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<DOMRect>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const DOMRect::funcs[] = {
    JS_CGETSET_DEF("x", &bridge::Getter<&DOMRect::x>, NULL),
    JS_CGETSET_DEF("y", &bridge::Getter<&DOMRect::y>, NULL),
    JS_CGETSET_DEF("width", &bridge::Getter<&DOMRect::width>, NULL),
    JS_CGETSET_DEF("height", &bridge::Getter<&DOMRect::height>, NULL)
};
