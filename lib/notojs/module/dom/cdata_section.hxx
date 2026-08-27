struct CDATASection : bridge::Interface<CDATASection, dom::Node, Text>
{
    CDATASection(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    CDATASection(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<CDATASection>;
};
