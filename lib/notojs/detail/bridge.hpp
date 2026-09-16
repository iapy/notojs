#pragma once
#include <bridge.hpp>

namespace bridge {

template<typename Impl>
struct Content : Object
{
    using Object::Object;
    using Base = Content<Impl>;

    BOOST_FORCEINLINE static bool check(JSContext *ctx, JSValue *value)
    {
        return JS_IsObject(*value) && cid == JS_GetClassID(*value);
    }

    static JSCFunctionListEntry const funcs[];

    static JSClassID cid;
    static JSClassDef def;
    static thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> upcast;

    static void init();
    static void init(JSRuntime *);
    static void init(JSContext *);
    static void init(JSContext *, JSValue);
    static void alias(JSContext *, JSModuleDef *, const char *name = nullptr);

    static char const *name();
    static JSValue ctor(JSContext *);
    static JSValue data(JSContext *, JSValue data, bool json = true);

    static bool ctor(JSContext *ctx, JSValue);

private:
    Content() = delete;
    static thread_local JSValue proto; // bound to context

private:
    static JSValue make(JSContext *);
};

template<typename Impl>
JSClassID Content<Impl>::cid;

template<typename Impl>
thread_local JSValue Content<Impl>::proto = JS_UNDEFINED;

template<typename Impl>
thread_local std::unordered_map<JSClassID, std::function<Impl(void*)>> Content<Impl>::upcast;

template<typename Impl>
JSClassDef Content<Impl>::def = {
    .class_name = Content<Impl>::name(),
    .finalizer = NULL
};

template<typename Impl>
JSCFunctionListEntry const Content<Impl>::funcs[] = {};

template<typename Impl>
char const *Content<Impl>::name()
{
    static std::string name = boost::core::demangle(typeid(Impl).name());
    return name.c_str() + name.rfind(':') + 1;
}

template<typename Impl>
void Content<Impl>::init()
{
    JS_NewClassID(&cid);
}

template<typename Impl>
void Content<Impl>::init(JSRuntime *rt)
{
    JS_NewClass(rt, cid, &Content<Impl>::def);
}

template<typename Impl>
void Content<Impl>::init(JSContext *ctx, JSValue glob)
{
    JS_SetPropertyStr(ctx, glob, name(), make(ctx));
}

template<typename Impl>
void Content<Impl>::init(JSContext *ctx)
{
#ifndef NOTOJS_INTERNAL_MODULE
    init();
    init(JS_GetRuntime(ctx));
#endif
    JS_FreeValue(ctx, make(ctx));
}

template<typename Impl>
void Content<Impl>::alias(JSContext *ctx, JSModuleDef *glob, const char *name)
{
    JS_SetModuleExport(ctx, glob, name ? name : Content<Impl>::name(), JS_GetPropertyStr(ctx, proto, "constructor"));
}

template<typename Impl>
JSValue Content<Impl>::data(JSContext *ctx, JSValue data, bool json)
{
    JSValue self = JS_NewObjectProtoClass(ctx, Impl::proto, Impl::cid);
    if(JS_IsException(self))
    {
        JS_FreeValue(ctx, data);
        return self;
    }

    if(JS_DefinePropertyValueStr(ctx, self, "data", data, JS_PROP_ENUMERABLE) < 0
        || (!json && JS_DefinePropertyValueStr(ctx, self, ".json", JS_FALSE, 0) < 0)
        || JS_PreventExtensions(ctx, self) < 0)
    {
        JS_FreeValue(ctx, self);
        return JS_EXCEPTION;
    }
    return self;
}

template<typename Impl>
JSValue Content<Impl>::make(JSContext *ctx)
{
    proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, Impl::funcs, sizeof(Impl::funcs)/sizeof(Impl::funcs[0]));

    JSValue ctor = JS_NewCFunction2(ctx, &Unconstructable<Impl>::invoke, name(), 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);

    JS_SetClassProto(ctx, cid, proto);
    return ctor;
}

template<typename Impl>
JSValue Content<Impl>::ctor(JSContext *ctx)
{
    JSValue self = JS_NewObjectProtoClass(ctx, Impl::proto, Impl::cid);
    if(JS_IsException(self)) return self;
    if(JS_PreventExtensions(ctx, self) < 0)
    {
        JS_FreeValue(ctx, self);
        return JS_EXCEPTION;
    }
    return self;
}

template<typename Impl>
bool Content<Impl>::ctor(JSContext *ctx, JSValue self)
{
    bridge::Strong<void> ctor{ctx, JS_GetPropertyStr(ctx, proto, "constructor"), false};
    return JS_VALUE_GET_PTR(self) == JS_VALUE_GET_PTR(static_cast<JSValue>(ctor));
}

} // namespace bridge
