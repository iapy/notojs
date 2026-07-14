struct SVGElement : bridge::Interface<SVGElement, dom::HTMLElement, HTMLElement>
{
    SVGElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGElement>;
};

struct SVGSVGElement : bridge::Interface<SVGSVGElement, dom::HTMLElement, SVGElement>
{
    SVGSVGElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGSVGElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    struct I : Base::I<I, SVG::Interface>
    {
        using Base::Base;
        std::string get() const override
        {
            return ref.toString();
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<SVGSVGElement>;
};

struct SVGGraphicsElement : bridge::Interface<SVGGraphicsElement, dom::HTMLElement, SVGElement>
{
    SVGGraphicsElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGGraphicsElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue getBBox(JSContext *ctx)
    {
        return DOMRect::from(ctx, dom::DOMRect(ref(), dom::Attr::Name{"bbox", LXB_NS_SVG}));
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGGraphicsElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGGraphicsElement::funcs[] = {
    JS_CFUNC_DEF("getBBox", 0, &bridge::Function<&SVGGraphicsElement::getBBox>::invoke)
};

struct SVGRectElement : bridge::Interface<SVGRectElement, dom::HTMLElement, SVGGraphicsElement>
{
    SVGRectElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGRectElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    ELEMENT_ATTRIBUTE_PROPERTY(x)
    ELEMENT_ATTRIBUTE_PROPERTY(y)
    ELEMENT_ATTRIBUTE_PROPERTY(rx)
    ELEMENT_ATTRIBUTE_PROPERTY(ry)
    ELEMENT_ATTRIBUTE_PROPERTY(height)
    ELEMENT_ATTRIBUTE_PROPERTY(width)

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGRectElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGRectElement::funcs[] = {
    JS_CGETSET_DEF("x", &bridge::Getter<&SVGRectElement::x>, &bridge::Setter<&SVGRectElement::set_x>),
    JS_CGETSET_DEF("y", &bridge::Getter<&SVGRectElement::y>, &bridge::Setter<&SVGRectElement::set_y>),
    JS_CGETSET_DEF("rx", &bridge::Getter<&SVGRectElement::rx>, &bridge::Setter<&SVGRectElement::set_rx>),
    JS_CGETSET_DEF("ry", &bridge::Getter<&SVGRectElement::ry>, &bridge::Setter<&SVGRectElement::set_ry>),
    JS_CGETSET_DEF("width", &bridge::Getter<&SVGRectElement::width>, &bridge::Setter<&SVGRectElement::set_width>),
    JS_CGETSET_DEF("height", &bridge::Getter<&SVGRectElement::height>, &bridge::Setter<&SVGRectElement::set_height>)
};

struct SVGTextElement : bridge::Interface<SVGTextElement, dom::HTMLElement, SVGGraphicsElement>
{
    SVGTextElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGTextElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    ELEMENT_ATTRIBUTE_PROPERTY(x)
    ELEMENT_ATTRIBUTE_PROPERTY(y)
    ELEMENT_ATTRIBUTE_PROPERTY(dx)
    ELEMENT_ATTRIBUTE_PROPERTY(dy)
    ELEMENT_ATTRIBUTE_PROPERTY(rotate)

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGTextElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGTextElement::funcs[] = {
    JS_CGETSET_DEF("x", &bridge::Getter<&SVGTextElement::x>, &bridge::Setter<&SVGTextElement::set_x>),
    JS_CGETSET_DEF("y", &bridge::Getter<&SVGTextElement::y>, &bridge::Setter<&SVGTextElement::set_y>),
    JS_CGETSET_DEF("dx", &bridge::Getter<&SVGTextElement::dx>, &bridge::Setter<&SVGTextElement::set_dx>),
    JS_CGETSET_DEF("dy", &bridge::Getter<&SVGTextElement::dy>, &bridge::Setter<&SVGTextElement::set_dy>),
    JS_CGETSET_DEF("rotate", &bridge::Getter<&SVGTextElement::rotate>, &bridge::Setter<&SVGTextElement::set_rotate>)
};
