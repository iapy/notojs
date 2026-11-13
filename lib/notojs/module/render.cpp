#include <notojs/module/render.hpp>
#include <notojs/notojs.hpp>
#include <notojs/global.hpp>
#include <bridge.hpp>
#include <global.hpp>

namespace notojs {
namespace {

struct Render : bridge::Interface<Render, bridge::Object>
{
    using Base::Base;

    static JSValue make(JSContext *ctx, JSValueConst fn, JSValueConst rx, JSValueConst v, JSValueConst p)
    {
        bridge::Strong<void> result{ctx, ctor(ctx), false};
        if(!JS_IsException(result))
        {
            if(JS_DefinePropertyValueStr(ctx, result, "render.f", JS_DupValue(ctx, fn), JS_PROP_ENUMERABLE) < 0) return JS_EXCEPTION;
            if(JS_DefinePropertyValueStr(ctx, result, "render.r", JS_DupValue(ctx, rx), JS_PROP_ENUMERABLE) < 0) return JS_EXCEPTION;
            if(JS_DefinePropertyValueStr(ctx, result, "render.v", JS_DupValue(ctx, v), 0) < 0) return JS_EXCEPTION;
            if(JS_DefinePropertyValueStr(ctx, result, "render.p", JS_DupValue(ctx, p), 0) < 0) return JS_EXCEPTION;

            for(char const *name : {"length", "call"})
            {
                JSValue value = JS_GetPropertyStr(ctx, fn, name);
                if(JS_IsException(value) || JS_DefinePropertyValueStr(ctx, result, name, value, 0) < 0)
                    return JS_EXCEPTION;
            }

            bridge::Strong<void> proto{ctx, JS_GetPrototype(ctx, fn), false};
            if(JS_IsException(proto) || JS_SetPrototype(ctx, result, proto) < 0)
                return JS_EXCEPTION;
        }
        return result.release();
    }

    JSValue operator () (JSContext *ctx, JSValueConst receiver, int argc, JSValueConst *argv)
    {
        auto f = (*this)["render.f"];
        if(JS_IsException(f)) return f.release();

        auto v = get<bridge::String>("render.v");
        auto p = get<bridge::Object>("render.p");
        bridge::Strong<void> res{ctx, JS_Call(ctx, f, v ? JS_UNDEFINED : receiver, argc, argv), false};
        if(JS_IsException(res)) return res.release();

        if(p && JS_SetPrototype(ctx, res, *p) < 0) return JS_EXCEPTION;

        if(v && JS_DefinePropertyValueStr(ctx, res, "view", v->release(), JS_PROP_ENUMERABLE) < 0) return JS_EXCEPTION;
        return res.release();
    }

    JSValue get_property(JSContext *ctx, char const *name) const
    {
        auto rx = (*this)["render.r"];
        if(JS_IsUndefined(rx) || JS_IsException(rx)) return rx.release();

        bridge::Strong<void> value{ctx, JS_NewString(ctx, name), false};
        if(JS_IsException(value)) return value.release();

        bridge::Strong<void> match{ctx, JS_GetPropertyStr(ctx, value, "match"), false};
        if(JS_IsException(match)) return match.release();

        bridge::Strong<void> matched{ctx, JS_Call(ctx, match, value, 1, +rx), false};
        if(JS_IsException(matched)) return matched.release();
        if(!JS_ToBool(ctx, matched)) return JS_ThrowTypeError(ctx, "render: invalid view %s", name);

        auto fn = (*this)["render.f"];
        if(JS_IsException(fn)) return fn.release();

        auto p = get<bridge::Object>("render.p");
        return make(ctx, fn, JS_UNDEFINED, value, p ? static_cast<JSValue>(*p) : JS_UNDEFINED);
    }

    static JSValue call(JSContext *ctx, JSValueConst self, bridge::String name, bridge::Lambda fn, bridge::Tail<0, bridge::Value> args)
    {
        JSValue result = JS_Call(ctx, fn, self, static_cast<int>(args.size()), args.data());
        if(JS_IsException(result)) return result;

        Global::Context::ptr(ctx)->renderers.insert(name);
        return Content::make(ctx, name, result);
    }

    static JSValue call_p(JSContext *ctx, JSValueConst self, bridge::Object prototype, bridge::String name, bridge::Lambda fn, bridge::Tail<0, bridge::Value> args)
    {
        JSValue data = JS_Call(ctx, fn, self, static_cast<int>(args.size()), args.data());
        if(JS_IsException(data)) return data;

        Global::Context::ptr(ctx)->renderers.insert(name);
        bridge::Strong<void> result{ctx, Content::make(ctx, name, data), false};
        if(JS_IsException(result)) return result.release();
        if(JS_SetPrototype(ctx, result, prototype) < 0) return JS_EXCEPTION;
        return result.release();
    }

    template<typename T>
    static JSValue spec(JSContext *ctx, JSValueConst self, bridge::String name, bridge::Lambda fn, bridge::Tail<0, bridge::Value> args)
    {
        JSValue data = JS_Call(ctx, fn, self, static_cast<int>(args.size()), args.data());
        if(JS_IsException(data)) return data;

        Global::Context::ptr(ctx)->renderers.insert(name);

        bridge::Strong<bridge::Object> obj{ctx, bridge::Object{ctx}};
        obj.set("data", data);
        obj.set("type", JS_DupValue(ctx, name));
        return T::ctor(ctx, &obj);
    }

    template<typename T>
    static JSValue spec_p(JSContext *ctx, JSValueConst self, bridge::Object prototype, bridge::String name, bridge::Lambda fn, bridge::Tail<0, bridge::Value> args)
    {
        JSValue data = JS_Call(ctx, fn, self, static_cast<int>(args.size()), args.data());
        if(JS_IsException(data)) return data;

        Global::Context::ptr(ctx)->renderers.insert(name);

        bridge::Strong<bridge::Object> obj{ctx, bridge::Object{ctx}};
        obj.set("data", data);
        obj.set("type", JS_DupValue(ctx, name));

        bridge::Strong<void> result{ctx, T::ctor(ctx, &obj), false};
        if(JS_IsException(result)) return result.release();
        if(JS_DefinePropertyValueStr(ctx, result, "render.p", JS_DupValue(ctx, prototype), 0) < 0) return JS_EXCEPTION;
        return result.release();
    }

    static JSValue print(JSContext *ctx, bridge::Array &out, bridge::Object const &ref)
    {
        auto data = ref.get<bridge::Value>("data");
        auto type = ref.get<bridge::String>("type");
        auto p = ref.get<bridge::Object>("render.p");

        if(!data || !type) return JS_ThrowTypeError(ctx, "Invalid Block");

        bridge::Strong<void> content{ctx, Content::make(ctx, *type, data->release()), false};
        if(JS_IsException(content)) return content.release();
        if(p && JS_SetPrototype(ctx, content, *p) < 0) return JS_EXCEPTION;

        bridge::Array arr{ctx};
        if(!arr.append(content.release())) return JS_EXCEPTION;
        return out.append(arr) ? JS_UNDEFINED : JS_EXCEPTION;
    }

    static constexpr bool constructible = false;
    static JSClassExoticMethods exoticMethods;
};

JSClassExoticMethods Render::exoticMethods = {
    .get_property = &bridge::get_property<Render>
};

struct Block : bridge::Interface<Block, bridge::Object>
{
    using Base::Base;
    struct I : Base::I<I, NOTOJS_VERSION_NS::Print>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array out) const
        {
            return Render::print(ctx, out, ref);
        }
    };

    using impl = bridge::Implements<I>;
    static constexpr bool constructible = false;
};

struct Final : bridge::Interface<Final, bridge::Object>
{
    using Base::Base;
    struct I : Base::I<I, NOTOJS_VERSION_NS::Print>
    {
        using Base::Base;

        JSValue print(JSContext *ctx, bridge::Array out) const
        {
            JSValue res = Render::print(ctx, out, ref);
            if(JS_IsUndefined(res) && JS_PreventExtensions(ctx, out) < 0)
                return JS_EXCEPTION;
            return res;
        }
    };

    using impl = bridge::Implements<I>;
    static constexpr bool constructible = false;
};

JSValue render_0(JSContext *ctx, bridge::String name, bridge::Lambda fn)
{
    return bridge::FunctionData<&Render::call>::bind(ctx, name, fn);
}

JSValue render_1(JSContext *ctx, bridge::String name, bridge::Value regex, bridge::Lambda fn)
{
    if(Block::ctor(ctx, regex)) return bridge::FunctionData<&Render::spec<Block>>::bind(ctx, name, fn);
    if(Final::ctor(ctx, regex)) return bridge::FunctionData<&Render::spec<Final>>::bind(ctx, name, fn);

    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};

    auto regexp = glob["RegExp"];
    if(JS_IsException(regexp)) return regexp.release();

    if(int valid = JS_IsInstanceOf(ctx, regex, regexp); valid < 0) return JS_EXCEPTION;
    else if(!valid) return JS_ThrowTypeError(ctx, "render: expected a RegExp");

    bridge::Strong<void> r{ctx, render_0(ctx, name, fn), false};
    if(JS_IsException(r)) return r.release();
    return Render::make(ctx, r, regex, JS_UNDEFINED, JS_UNDEFINED);
}

JSValue render_2(JSContext *ctx, bridge::Object prototype, bridge::String name, bridge::Lambda fn)
{
    return bridge::FunctionData<&Render::call_p>::bind(ctx, prototype, name, fn);
}

JSValue render_3(JSContext *ctx, bridge::Object prototype, bridge::String name, bridge::Value regex, bridge::Lambda fn)
{
    if(Block::ctor(ctx, regex)) return bridge::FunctionData<&Render::spec_p<Block>>::bind(ctx, prototype, name, fn);
    if(Final::ctor(ctx, regex)) return bridge::FunctionData<&Render::spec_p<Final>>::bind(ctx, prototype, name, fn);

    bridge::Strong<bridge::Object> glob{ctx, JS_GetGlobalObject(ctx)};

    auto regexp = glob["RegExp"];
    if(JS_IsException(regexp)) return regexp.release();

    if(int valid = JS_IsInstanceOf(ctx, regex, regexp); valid < 0) return JS_EXCEPTION;
    else if(!valid) return JS_ThrowTypeError(ctx, "render: expected a RegExp");

    bridge::Strong<void> r{ctx, render_0(ctx, name, fn), false};
    if(JS_IsException(r)) return r.release();
    return Render::make(ctx, r, regex, JS_UNDEFINED, prototype);
}

using render = bridge::Function<&render_0, &render_1, &render_2, &render_3>;

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("render", 2, &render::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    Render::init(ctx);
    Block::init(ctx, m);
    Final::init(ctx, m);
    JS_SetModuleExport(ctx, m, "default", JS_NewCFunction(ctx, render::invoke, "render", 2));
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

void notojs_init_render()
{
    Render::init();
    Block::init();
    Final::init();
}

void notojs_init_render(JSRuntime *rt)
{
    Render::init(rt);
    Block::init(rt);
    Final::init(rt);
}

void notojs_init_render(detail::Config const &) {}

JSModuleDef *notojs_init_render(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, "default");
    JS_AddModuleExport(ctx, mod, Block::name());
    JS_AddModuleExport(ctx, mod, Final::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // namespace notojs
