struct DOMStringMap : bridge::Interface<DOMStringMap, dom::DOMStringMap>
{
    DOMStringMap(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    DOMStringMap(std::reference_wrapper<dom::DOMStringMap> &&rw) : Base(std::move(rw)) {}

    JSValue get_property(JSContext *, char const *n) const
    {
        return ref().get_property(n);
    }

    bool has_property(JSContext *, char const *n) const
    {
        return ref().has_property(n);
    }

    std::vector<std::string> own_properties() const
    {
        return ref().own_properties();
    }

    bool own_property(JSContext *ctx, char const *n, JSPropertyDescriptor *desc)
    {
        return ref().own_property(n, desc);
    }

    void set_property(JSContext *ctx, char const *n, JSValueConst value)
    {
        ref().set_property(n, bridge::Value{ctx, value}.toString());
    }

    void del_property(JSContext *ctx, char const *n)
    {
        auto key = ref().del_property(n);
        if(auto it = ref().doc->attributes.find(ref().node); it != std::end(ref().doc->attributes))
            NamedNodeMap::remove(it->second, key);
    }

    BOOST_FORCEINLINE static void free(dom::DOMStringMap &self)
    {
        dynamic_cast<dom::HTMLBackend *>(self.doc.get())->datasets.erase(self);
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<DOMStringMap>;
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods DOMStringMap::exoticMethods = {
    .get_own_property = &bridge::own_property<DOMStringMap>,
    .get_own_property_names = &bridge::own_properties<DOMStringMap>,
    .delete_property = &bridge::del_property<DOMStringMap>,
    .has_property = &bridge::has_property<DOMStringMap>,
    .get_property = &bridge::get_property<DOMStringMap>,
    .set_property = &bridge::set_property<DOMStringMap>
};
