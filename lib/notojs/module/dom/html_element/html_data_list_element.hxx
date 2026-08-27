struct HTMLDataListElement : bridge::Interface<HTMLDataListElement, dom::HTMLElement, HTMLElement>
{
    HTMLDataListElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLDataListElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    JSValue options(JSContext *ctx) const
    {
        return HTMLCollection::from(ctx, dom::HTMLCollection{dom::Node{ref().doc, ref().node}, "option"});
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<HTMLDataListElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLDataListElement::funcs[] = {
    JS_CGETSET_DEF("options", &bridge::Getter<&HTMLDataListElement::options>, NULL)
};
