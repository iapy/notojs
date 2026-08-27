struct SVGMarkerElement : bridge::Interface<SVGMarkerElement, dom::SVGElement, SVGElement>
{
    SVGMarkerElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGMarkerElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGMarkerElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGMarkerElement::funcs[] = {
    SVGLENGTH_ATTRIBUTE(refX),
    SVGLENGTH_ATTRIBUTE(refY),
    SVGLENGTH_ATTRIBUTE(markerWidth),
    SVGLENGTH_ATTRIBUTE(markerHeight),
    REFLECTING_ATTRIBUTE(markerUnits),
    REFLECTING_ATTRIBUTE(orient),
    REFLECTING_ATTRIBUTE(preserveAspectRatio),

    JS_CGETSET_DEF("viewBox", &bridge::Getter<&SVGElement::viewBox>, NULL),
};
