struct CSSStyleDeclaration : bridge::Interface<CSSStyleDeclaration, dom::CSSStyleDeclaration>
{
    CSSStyleDeclaration(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CSSStyleDeclaration(std::reference_wrapper<dom::CSSStyleDeclaration> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<CSSStyleDeclaration>;
};
