struct Text : bridge::Interface<Text, dom::Node, Node>
{
    Text(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    Text(std::reference_wrapper<dom::Node> &&rw) : Base(std::move(rw)) {}

    using ctor = bridge::Unconstructable<Text>;
};
