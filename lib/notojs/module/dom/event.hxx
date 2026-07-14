struct Event : bridge::Interface<Event, std::monostate>
{
    Event() = default;
    Event(JSContext *ctx, JSValue val) : Base{ctx, val} {}
    Event(std::reference_wrapper<std::monostate> &&rw) : Base(std::move(rw)) {}
};

struct CustomEvent : bridge::Interface<CustomEvent, std::monostate, Event>
{
    CustomEvent(bridge::String type) {}
    CustomEvent(bridge::String type, bridge::Object opts) {}

    using ctor = bridge::Constructor
    <
        CustomEvent(bridge::String),
        CustomEvent(bridge::String, bridge::Object)
    >;
};
