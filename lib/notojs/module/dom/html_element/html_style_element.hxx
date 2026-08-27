struct HTMLStyleElement : bridge::Interface<HTMLStyleElement, dom::HTMLElement, HTMLElement>
{
    HTMLStyleElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLStyleElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue sheet(JSContext *ctx) const
    {
        auto doc = dynamic_cast<dom::HTMLBackend *>(ref().doc.get());
        auto ptr = static_cast<lxb_html_element_t *>(ref());

        if(auto it = doc->sheets.find(ptr); it != std::end(doc->sheets))
            return JS_DupValue(ctx, it->second);
        return doc->sheets[ptr] = CSSStyleSheet::from(ctx, dom::CSSStyleSheet{ref()});
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLStyleElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLStyleElement::funcs[] = {
    JS_CGETSET_DEF("sheet", &bridge::Getter<&HTMLStyleElement::sheet>, NULL)
};
