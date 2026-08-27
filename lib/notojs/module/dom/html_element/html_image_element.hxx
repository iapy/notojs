struct HTMLImageElement : bridge::Interface<HTMLImageElement, dom::HTMLElement, HTMLElement>
{
    HTMLImageElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    HTMLImageElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    struct I : Base::I<I, Image::Interface>
    {
        using Base::Base;
        std::string get() const override
        {
            return std::string{ref.getAttribute({"src"}).value_or("")};
        }
    };

    using Base::Base;
    using impl = bridge::Implements<I>;
    using ctor = bridge::Unconstructable<HTMLImageElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const HTMLImageElement::funcs[] = {
    REFLECTING_ATTRIBUTE(alt),
    REFLECTING_ATTRIBUTE(src)
};
