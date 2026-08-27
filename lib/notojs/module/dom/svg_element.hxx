struct SVGLength : bridge::Interface<SVGLength, dom::SVGLength>
{
    static void free(dom::SVGLength &self);

    JSValue baseVal(JSContext *ctx, JSValue self)
    {
        return JS_DupValue(ctx, self);
    }

    JSValue value(JSContext *ctx) const
    {
        auto attr = ref().getAttribute(ref().name);
        return JS_NewFloat64(ctx, attr ? std::atof(attr->data()) : 0);
    }

    void set_value(JSContext *ctx, bridge::Number n)
    {
        auto const s = std::to_string(n.as_double());
        ref().setAttribute(ref().name, {s.c_str(), s.size()});
    }

    JSValue toPrimitive(JSContext *ctx, bridge::String hint)
    {
        auto attr = ref().getAttribute(ref().name);
        if(auto const &hv = static_cast<std::string_view const &>(hint); "default" == hv || "number" == hv)
            return JS_NewFloat64(ctx, attr ? std::atof(attr->data()) : 0);
        else if("string" == hv)
            return bridge::String{ctx, attr ? *attr : std::string_view{""}};
        return JS_UNDEFINED;
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGLength>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGLength::funcs[] = {
    JS_CGETSET_DEF("baseVal", &bridge::Getter<&SVGLength::baseVal>, NULL),
    JS_CGETSET_DEF("value", &bridge::Getter<&SVGLength::value>, &bridge::Setter<&SVGLength::set_value>),
    JS_CFUNC_DEF("[Symbol.toPrimitive]", 0, &bridge::Function<&SVGLength::toPrimitive>::invoke),
};

struct SVGElement : bridge::Interface<SVGElement, dom::SVGElement, HTMLElement>
{
    SVGElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    struct attributes
    {
        static constexpr char x[] = "x";
        static constexpr char y[] = "y";
        static constexpr char width[] = "width";
        static constexpr char height[] = "height";
        static constexpr char cx[] = "cx";
        static constexpr char cy[] = "cy";
        static constexpr char r[] = "r";
        static constexpr char rx[] = "rx";
        static constexpr char ry[] = "ry";
        static constexpr char x1[] = "x1";
        static constexpr char x2[] = "x2";
        static constexpr char y1[] = "y1";
        static constexpr char y2[] = "y2";
        static constexpr char dx[] = "dx";
        static constexpr char dy[] = "dy";
        static constexpr char rotate[] = "rotate";
        static constexpr char textLength[] = "textLength";
        static constexpr char startOffset[] = "startOffset";
        static constexpr char fx[] = "fx";
        static constexpr char fy[] = "fy";
        static constexpr char fr[] = "fr";
        static constexpr char refX[] = "refX";
        static constexpr char refY[] = "refY";
        static constexpr char markerWidth[] = "markerWidth";
        static constexpr char markerHeight[] = "markerHeight";
    };

    template<char const *Name>
    JSValue attribute(JSContext *ctx, JSValue self)
    {
        if(auto it = ref().attributes.find({Name}); it != std::end(ref().attributes))
            return JS_DupValue(ctx, it->second);
        return ref().attributes[{Name}] = SVGLength::from(ctx, dom::SVGLength{ref().doc, ref(), Name}, self);
    }

    JSValue ownerSVGElement(JSContext *ctx) const
    {
        if(lxb_dom_node_t *node = ref(); LXB_TAG_SVG == lxb_dom_node_tag_id(node))
            return JS_NULL;

        if(auto *node = ref().closest(LXB_TAG_SVG); node)
            return  dynamic_cast<dom::HTMLBackend *>(ref().doc.get())->make(node);
        return JS_NULL;
    }

    JSValue href(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"href"}))
            return bridge::String{ctx, *value};
        if(auto value = ref().getAttribute({"href", LXB_NS_XLINK}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_href(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"href"}, value.toString());
    }

    JSValue get_in(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"in"}))
            return bridge::String{ctx, *value};
        return bridge::String{ctx};
    }

    void set_in(JSContext *, bridge::Value value)
    {
        ref().setAttribute({"in"}, value.toString());
    }

    JSValue viewBox(JSContext *ctx) const
    {
        return DOMRect::from(ctx, dom::DOMRect(ref(), dom::Attr::Name{"viewBox", LXB_NS_SVG}));
    }

    friend class SVGLength;
    friend class Window;

    using Base::Base;
    using esab = Element;
    using ctor = bridge::Unconstructable<SVGElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGElement::funcs[] = {
    REFLECTING_ATTRIBUTE(id),

    JS_CGETSET_DEF("attributes", &bridge::Getter<&HTMLElement::attributes>, NULL),
    JS_CGETSET_DEF("classList", &bridge::Getter<&HTMLElement::classList>, NULL),
    JS_CGETSET_DEF("className", &bridge::Getter<&HTMLElement::className>, &bridge::Setter<&HTMLElement::set_className>),
    JS_CGETSET_DEF("dataset", &bridge::Getter<&HTMLElement::dataset>, NULL),
    JS_CGETSET_DEF("innerHTML", &bridge::Getter<&HTMLElement::innerHTML>, &HTMLElement::set_innerHTML::invoke),
    JS_CGETSET_DEF("outerHTML", &bridge::Getter<&HTMLElement::toString>, &HTMLElement::set_outerHTML::invoke),
    JS_CGETSET_DEF("ownerSVGElement", &bridge::Getter<&SVGElement::ownerSVGElement>, NULL),
    JS_CGETSET_DEF("style", &bridge::Getter<&HTMLElement::style>, NULL),

    JS_CFUNC_DEF("after", 1, &bridge::Function<&NodeMixin::after_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("append", 1,  &bridge::Function<&NodeMixin::append_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("appendChild", 1, &HTMLElement::appendChild::invoke),
    JS_CFUNC_DEF("before", 1,  &bridge::Function<&NodeMixin::before_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("getAttributeNode", 1, &bridge::Function<&HTMLElement::getAttributeNode>::invoke),
    JS_CFUNC_DEF("getAttributeNodeNS", 2, &HTMLElement::getAttributeNodeNS::invoke),
    JS_CFUNC_DEF("getAttributeNS", 2, &HTMLElement::getAttributeNS::invoke),
    JS_CFUNC_DEF("getElementsByClassName", 1, &bridge::Function<&HTMLElement::getElementsByClassName>::invoke),
    JS_CFUNC_DEF("getElementsByTagName", 1, &bridge::Function<&HTMLElement::getElementsByTagName>::invoke),
    JS_CFUNC_DEF("getElementsByTagNameNS", 2, &HTMLElement::getElementsByTagNameNS::invoke),
    JS_CFUNC_DEF("getHTML", 0, &bridge::Function<&HTMLElement::innerHTML>::invoke),
    JS_CFUNC_DEF("hasAttributeNS", 2, &HTMLElement::hasAttributeNS::invoke),
    JS_CFUNC_DEF("insertAdjacentElement", 2, &HTMLElement::insertAdjacentElement::invoke),
    JS_CFUNC_DEF("insertAdjacentHTML", 2, &HTMLElement::insertAdjacentHTML::invoke),
    JS_CFUNC_DEF("insertBefore", 2, &HTMLElement::insertBefore::invoke),
    JS_CFUNC_DEF("moveBefore", 2, &HTMLElement::insertBefore::invoke),
    JS_CFUNC_DEF("prepend", 1, &bridge::Function<&NodeMixin::prepend_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("removeAttributeNS", 2, &HTMLElement::removeAttributeNS::invoke),
    JS_CFUNC_DEF("removeAttributeNode", 1, &bridge::Function<&HTMLElement::removeAttributeNode>::invoke),
    JS_CFUNC_DEF("replaceChildren", 1, &bridge::Function<&NodeMixin::replaceChildren_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("replaceChild", 2, &HTMLElement::replaceChild::invoke),
    JS_CFUNC_DEF("replaceWith", 1,  &bridge::Function<&NodeMixin::replaceWith_t<HTMLNodeMixin>>::invoke),
    JS_CFUNC_DEF("setAttributeNS", 3, &HTMLElement::setAttributeNS::invoke),
    JS_CFUNC_DEF("setAttributeNode", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),
    JS_CFUNC_DEF("setAttributeNodeNS", 1, &bridge::Function<&HTMLElement::setAttributeNode>::invoke),

    // Integration interface
    JS_CFUNC_DEF("toString", 0, &bridge::Function<&HTMLElement::toString>::invoke),
    JS_CFUNC_DEF("toJSON", 0, &bridge::JSON<HTMLElement>::toJSON)
};

#define SVGLENGTH_ATTRIBUTE(name) JS_CGETSET_DEF(#name, &bridge::Getter<(&SVGElement::attribute<SVGElement::attributes::name>)>, NULL)

struct SVGSVGElement : bridge::Interface<SVGSVGElement, dom::SVGElement, SVGElement>
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
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGSVGElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(x),
    SVGLENGTH_ATTRIBUTE(y),
    SVGLENGTH_ATTRIBUTE(width),
    SVGLENGTH_ATTRIBUTE(height),
    REFLECTING_ATTRIBUTE(preserveAspectRatio),

    JS_CGETSET_DEF("viewBox", &bridge::Getter<&SVGElement::viewBox>, NULL),
};

struct SVGGraphicsElement : bridge::Interface<SVGGraphicsElement, dom::SVGElement, SVGElement>
{
    SVGGraphicsElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGGraphicsElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue getBBox(JSContext *ctx)
    {
        return DOMRect::from(ctx, dom::DOMRect(ref(), dom::Attr::Name{"bbox"}));
    }

    friend class SVGLength;

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGGraphicsElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGGraphicsElement::funcs[] = {
    JS_CFUNC_DEF("getBBox", 0, &bridge::Function<&SVGGraphicsElement::getBBox>::invoke),
};

struct SVGAnimationElement : bridge::Interface<SVGAnimationElement, dom::SVGElement, SVGElement>
{
    SVGAnimationElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGAnimationElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGAnimationElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGAnimationElement::funcs[] = {
    REFLECTING_ATTRIBUTE(attributeName),
    REFLECTING_ATTRIBUTE(begin),
    REFLECTING_ATTRIBUTE(by),
    REFLECTING_ATTRIBUTE(dur),
    REFLECTING_ATTRIBUTE(end),
    REFLECTING_ATTRIBUTE(fill),
    REFLECTING_ATTRIBUTE(from),
    REFLECTING_ATTRIBUTE(repeatCount),
    REFLECTING_ATTRIBUTE(to),
    REFLECTING_ATTRIBUTE(values),
};

void SVGLength::free(dom::SVGLength &self)
{
    auto *doc = dynamic_cast<dom::HTMLBackend *>(self.doc.get());
    if(auto it = doc->nodes.find(self); it != std::end(doc->nodes))
    {
        auto &el = SVGElement::get(it->second);
        if(auto jt = el.attributes.find(self.name); jt != std::end(el.attributes))
            el.attributes.erase(jt);
    }
}

#define SVG_ELEMENT_STUB(...) BOOST_PP_OVERLOAD(SVG_ELEMENT_STUB_, __VA_ARGS__)(__VA_ARGS__)
#define SVG_ELEMENT_STUB_1(name) SVG_ELEMENT_STUB_2(name, SVGElement)
#define SVG_ELEMENT_STUB_2(name, base) \
struct name : bridge::Interface<name, dom::SVGElement, base> \
{ \
    name(JSContext *ctx, JSValue self) : Base{ctx, self} {} \
    name(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {} \
 \
    using Base::Base; \
    using ctor = bridge::Unconstructable<name>; \
};

SVG_ELEMENT_STUB(SVGAnimateElement, SVGAnimationElement)
SVG_ELEMENT_STUB(SVGAnimateMotionElement, SVGAnimationElement)
SVG_ELEMENT_STUB(SVGDescElement)
SVG_ELEMENT_STUB(SVGMetadataElement)
SVG_ELEMENT_STUB(SVGScriptElement)
SVG_ELEMENT_STUB(SVGSolidColorElement)
SVG_ELEMENT_STUB(SVGTitleElement)
SVG_ELEMENT_STUB(SVGDefsElement, SVGGraphicsElement)
SVG_ELEMENT_STUB(SVGGElement, SVGGraphicsElement)
SVG_ELEMENT_STUB(SVGSwitchElement, SVGGraphicsElement)
SVG_ELEMENT_STUB(SVGSetElement, SVGAnimationElement)

#undef SVG_ELEMENT_STUB_2
#undef SVG_ELEMENT_STUB_1
#undef SVG_ELEMENT_STUB

#define SVG_FILTER_PRIMITIVE_ATTRIBUTES \
    SVGLENGTH_ATTRIBUTE(x), \
    SVGLENGTH_ATTRIBUTE(y), \
    SVGLENGTH_ATTRIBUTE(width), \
    SVGLENGTH_ATTRIBUTE(height), \
    REFLECTING_ATTRIBUTE(result)

#define SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN \
    SVG_FILTER_PRIMITIVE_ATTRIBUTES, \
    JS_CGETSET_DEF("in", &bridge::Getter<&SVGElement::get_in>, &bridge::Setter<&SVGElement::set_in>)

#include <notojs/module/dom/svg_element/svg_a_element.hxx>
#include <notojs/module/dom/svg_element/svg_animate_transform_element.hxx>
#include <notojs/module/dom/svg_element/svg_circle_element.hxx>
#include <notojs/module/dom/svg_element/svg_clip_path_element.hxx>
#include <notojs/module/dom/svg_element/svg_ellipse_element.hxx>
#include <notojs/module/dom/svg_element/svg_filter_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_blend_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_color_matrix_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_component_transfer_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_composite_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_convolve_matrix_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_diffuse_lighting_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_displacement_map_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_distant_light_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_drop_shadow_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_flood_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_func_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_gaussian_blur_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_image_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_merge_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_merge_node_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_morphology_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_offset_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_point_light_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_specular_lighting_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_spot_light_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_tile_element.hxx>
#include <notojs/module/dom/svg_element/svg_fe_turbulence_element.hxx>
#include <notojs/module/dom/svg_element/svg_foreign_object_element.hxx>
#include <notojs/module/dom/svg_element/svg_line_element.hxx>
#include <notojs/module/dom/svg_element/svg_marker_element.hxx>
#include <notojs/module/dom/svg_element/svg_mask_element.hxx>
#include <notojs/module/dom/svg_element/svg_m_path_element.hxx>
#include <notojs/module/dom/svg_element/svg_path_element.hxx>
#include <notojs/module/dom/svg_element/svg_pattern_element.hxx>
#include <notojs/module/dom/svg_element/svg_polygon_element.hxx>
#include <notojs/module/dom/svg_element/svg_polyline_element.hxx>
#include <notojs/module/dom/svg_element/svg_rect_element.hxx>
#include <notojs/module/dom/svg_element/svg_image_element.hxx>
#include <notojs/module/dom/svg_element/svg_linear_gradient_element.hxx>
#include <notojs/module/dom/svg_element/svg_radial_gradient_element.hxx>
#include <notojs/module/dom/svg_element/svg_stop_element.hxx>
#include <notojs/module/dom/svg_element/svg_style_element.hxx>
#include <notojs/module/dom/svg_element/svg_text_content_element.hxx>
#include <notojs/module/dom/svg_element/svg_text_element.hxx>
#include <notojs/module/dom/svg_element/svg_t_span_element.hxx>
#include <notojs/module/dom/svg_element/svg_text_path_element.hxx>
#include <notojs/module/dom/svg_element/svg_symbol_element.hxx>
#include <notojs/module/dom/svg_element/svg_use_element.hxx>
#include <notojs/module/dom/svg_element/svg_view_element.hxx>

#undef SVG_FILTER_PRIMITIVE_ATTRIBUTES_IN
#undef SVG_FILTER_PRIMITIVE_ATTRIBUTES
#undef SVGLENGTH_ATTRIBUTE
