struct KeysIterator : std::pair<std::uint64_t, std::uint64_t>
{
    using B = std::pair<std::uint64_t, std::uint64_t>;
    KeysIterator(B &&it): B{it} {}

    JSValue get(JSContext *ctx) const
    {
        return bridge::Number(ctx, first);
    }
    KeysIterator &operator ++ ()
    {
        ++first;
        return *this;
    }
};

template<typename Base, typename Computed>
struct Collection : Base
{
    using Base::Base;

    JSValue get_property(JSContext *ctx, const char *name) const
    {
        std::int64_t const i = u64(name);
        return i >= 0
            ? std::visit(boost::hana::overload_linearly(
                [i](Computed const &list) -> JSValue {
                    return list.at(i, JS_UNDEFINED);
                },
                [i](dom::Node const &node) -> JSValue {
                    return Base::collection_item(node, i, JS_UNDEFINED);
                }), Base::ref())
            : JS_UNDEFINED;
    }

    void set_property(JSContext *ctx, const char *name, JSValueConst) {}
    void del_property(JSContext *ctx, const char *name) {}

    JSValue entries(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> arr{ctx, array()};
        for(std::uint32_t i = 0; i < arr.size(); ++i)
        {
            bridge::Array sub{ctx};
            sub.set(0, bridge::Number{ctx, i});
            sub.set(1, arr[i].release());
            arr.set(i, sub);
        }
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

    JSValue each_1(JSValue self, JSContext *ctx, bridge::Lambda lambda)
    {
        bridge::Strong<bridge::Array> arr{ctx, array()};
        for(std::uint64_t i = 0; i < arr.size(); ++i)
        {
            bridge::Strong<void> j{ctx, bridge::Number{ctx, i}, false};
            if(auto result = lambda(std::array<JSValue, 3>{arr[i], j, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    JSValue each_2(JSValue self, JSContext *ctx, bridge::Lambda lambda, bridge::Value value)
    {
        bridge::Strong<bridge::Array> arr{ctx, array()};
        for(std::uint64_t i = 0; i < arr.size(); ++i)
        {
            bridge::Strong<void> j{ctx, bridge::Number{ctx, i}, false};
            if(auto result = lambda(value, std::array<JSValue, 3>{arr[i], j, self}); bridge::Error::check(ctx, +result))
                return result.release();
        }
        return JS_UNDEFINED;
    }

    using each = bridge::Function
    <
        &Collection<Base, Computed>::each_1,
        &Collection<Base, Computed>::each_2
    >;

    JSValue item(JSContext *, bridge::Number index)
    {
        std::int64_t const i = static_cast<std::int64_t>(index);
        return i >= 0
            ? std::visit(boost::hana::overload_linearly(
                [i](Computed const &list) -> JSValue {
                    return list.at(i, JS_NULL);
                },
                [i](dom::Node const &node) -> JSValue {
                    return Base::collection_item(node, i, JS_NULL);
                }), Base::ref())
            : JS_NULL;
    }

    JSValue keys(JSValue self, JSContext *ctx) const
    {
        return std::visit(boost::hana::overload_linearly(
            [&](Computed const &list) -> JSValue {
                auto const len = list.size();
                return bridge::Iterator<KeysIterator>::make(ctx, self, KeysIterator::B{0, len}, KeysIterator::B{len, len});
            },
            [&](dom::Node const &node) -> JSValue {
                auto const len = Base::collection_size(node);
                return bridge::Iterator<KeysIterator>::make(ctx, self, KeysIterator::B{0, len}, KeysIterator::B{len, len});
            }),Base::ref());
    }

    JSValue length(JSContext *ctx) const
    {
        return std::visit(boost::hana::overload_linearly(
            [ctx](Computed const &list) -> JSValue {
                return bridge::Number{ctx, list.size()};
            },
            [ctx](dom::Node const &node) -> JSValue {
                return bridge::Number{ctx, Base::collection_size(node)};
            }), Base::ref());
    }

    JSValue values(JSContext *ctx) const
    {
        bridge::Strong<bridge::Array> arr{ctx, array()};
        return bridge::Strong<bridge::Lambda>{ctx, JS_GetPropertyStr(ctx, arr, "values")}(arr).release();
    }

private:
    BOOST_FORCEINLINE JSValue array() const
    {
        return std::visit(boost::hana::overload_linearly(
            [](Computed const &list) -> JSValue {
                return static_cast<JSValue>(list);
            },
            [](dom::Node const &node) -> JSValue {
                return Base::collection(node);
            }), Base::ref());
    }
};
