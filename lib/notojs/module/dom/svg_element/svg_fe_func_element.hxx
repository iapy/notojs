#define SVG_FE_FUNC_ELEMENT(name) \
struct name : bridge::Interface<name, dom::SVGElement, SVGElement> \
{ \
    name(JSContext *ctx, JSValue self) : Base{ctx, self} {} \
    name(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {} \
 \
 \
    using Base::Base; \
    using ctor = bridge::Unconstructable<name>; \
    static JSCFunctionListEntry const funcs[]; \
}; \
JSCFunctionListEntry const name::funcs[] = { \
    REFLECTING_ATTRIBUTE(type), \
    REFLECTING_ATTRIBUTE(tableValues), \
    REFLECTING_ATTRIBUTE(slope), \
    REFLECTING_ATTRIBUTE(intercept), \
    REFLECTING_ATTRIBUTE(amplitude), \
    REFLECTING_ATTRIBUTE(exponent), \
    REFLECTING_ATTRIBUTE(offset) \
};

SVG_FE_FUNC_ELEMENT(SVGFEFuncAElement)
SVG_FE_FUNC_ELEMENT(SVGFEFuncBElement)
SVG_FE_FUNC_ELEMENT(SVGFEFuncGElement)
SVG_FE_FUNC_ELEMENT(SVGFEFuncRElement)

#undef SVG_FE_FUNC_ELEMENT
