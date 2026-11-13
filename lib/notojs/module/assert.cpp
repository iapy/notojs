#include <notojs/module/assert.hpp>
#include <notojs/global.hpp>
#include <bridge.hpp>
#include <regex>

#ifdef assert
#undef assert
#endif

namespace notojs {
namespace {

class AssertionError : public bridge::Exception<AssertionError>
{
    std::string message;
public:
    AssertionError(std::string &&message): message{message} {}
    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        obj.set("message", bridge::String(ctx, message));
    }
};

JSValue assert(JSContext *ctx, bridge::Lambda lambda)
{
    using namespace bridge;
    if(auto res = lambda(); JS_IsException(res))
    {
        return res.release();
    }
    else if(!bridge::Boolean::check(ctx, res))
    {
        Strong<String> expr{ctx, JS_ToString(ctx, lambda)};
        if(JS_IsException(expr)) return JS_EXCEPTION;
        return JS_ThrowTypeError(ctx, "Expecting bool: %s", static_cast<std::string_view const &>(expr).data());
    }
    else if(!JS_ToBool(ctx, res))
    {
        Strong<String> expr{ctx, JS_ToString(ctx, lambda)};
        if(JS_IsException(expr)) return JS_EXCEPTION;
        if(std::cmatch m; std::regex_match(std::begin(expr), std::end(expr), m, std::regex{R"(\s*\(\s*\)\s*=>\s*(.+))"}))
            return AssertionError::throw_(ctx, AssertionError{m[1]});
        else
            return AssertionError::throw_(ctx, AssertionError{expr});
    }
    ++assertions;
    return JS_UNDEFINED;
}

JSValue throws_match(JSContext *ctx, JSValueConst error, JSValueConst type, JSValueConst message)
{
    using namespace bridge;
    if(!JS_IsUndefined(type))
    {
        if(int match = JS_IsInstanceOf(ctx, error, type); match < 0) return JS_EXCEPTION;
        else if(!match) return JS_FALSE;
    }
    if(JS_IsUndefined(message)) return JS_TRUE;

    Strong<void> msg{ctx, JS_IsNull(error) || JS_IsUndefined(error)
        ? JS_UNDEFINED : JS_GetPropertyStr(ctx, error, "message"), false};

    if(JS_IsException(msg)) return JS_EXCEPTION;
    if(!JS_IsString(msg))
    {
        JS_FreeValue(ctx, msg.swap(JS_ToString(ctx, error)));
        if(JS_IsException(msg)) return JS_EXCEPTION;
    }
    return JS_NewBool(ctx, static_cast<std::string_view const &>(String{ctx, message})
        == static_cast<std::string_view const &>(String{ctx, msg}));
}

JSValue throws_call(JSContext *ctx, bridge::Lambda lambda, bridge::Value type, bridge::Value message)
{
    using namespace bridge;
    auto res = lambda();
    if(JS_IsException(res))
    {
        Strong<void> error{ctx, JS_GetException(ctx), false};
        return throws_match(ctx, error, type, message);
    }
    if(!Promise::check(ctx, res)) return JS_FALSE;

    auto p = Promise{ctx, res}.wrap(
        [](JSContext *, JSValueConst, int, JSValueConst *){
            return JS_UNDEFINED;
        },
        [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv){
            // Box the rejection so even undefined and thenables remain distinct from fulfillment.
            bridge::Strong<void> result{ctx, JS_NewObjectProto(ctx, JS_NULL), false};
            if(JS_IsException(result)) return JS_EXCEPTION;
            if(JS_SetPropertyStr(ctx, result, "error", argc ? JS_DupValue(ctx, argv[0]) : JS_UNDEFINED) < 0)
                return JS_EXCEPTION;
            return result.release();
        }
    );
    if(JS_IsException(p)) return JS_EXCEPTION;

    auto *context = Global::Context::ptr(ctx);
    context->wait(ctx);
    if(context->perror)
    {
        JSValue error = *context->perror;
        context->perror.reset();
        return JS_Throw(ctx, error);
    }
    if(JS_PromiseState(ctx, p) == JS_PROMISE_PENDING) return JS_FALSE;

    Strong<void> result{ctx, JS_PromiseResult(ctx, p), false};
    if(JS_PromiseState(ctx, p) == JS_PROMISE_REJECTED)
    {
        // Reading a rejection does not mark it handled in QuickJS.
        auto ignore = [](JSContext *, JSValueConst, int, JSValueConst *){ return JS_UNDEFINED; };
        auto handled = p.wrap(ignore, ignore);
        if(JS_IsException(handled)) return JS_EXCEPTION;
        return JS_Throw(ctx, result.release());
    }
    if(JS_IsUndefined(result)) return JS_FALSE;

    Strong<void> error{ctx, JS_GetPropertyStr(ctx, result, "error"), false};
    if(JS_IsException(error)) return JS_EXCEPTION;
    return throws_match(ctx, error, type, message);
}

JSValue throws_description(JSContext *ctx, bridge::Lambda lambda, bridge::Value type, bridge::Value message)
{
    using namespace bridge;

    Strong<String> source{ctx, JS_ToString(ctx, lambda)};
    if(JS_IsException(source)) return JS_EXCEPTION;

    std::string description = "throws(" + static_cast<std::string>(source);
    if(!JS_IsUndefined(type))
    {
        Strong<Value> name{ctx, JS_GetPropertyStr(ctx, type, "name")};
        if(JS_IsException(name)) return JS_EXCEPTION;
        auto text = name.toString();
        if(JS_IsException(text)) return JS_EXCEPTION;
        description += ", " + static_cast<std::string>(text);
    }
    if(!JS_IsUndefined(message))
    {
        Strong<String> quoted{ctx, JS_JSONStringify(ctx, message, JS_UNDEFINED, JS_UNDEFINED)};
        if(JS_IsException(quoted)) return JS_EXCEPTION;
        description += ", " + static_cast<std::string>(quoted);
    }
    description += ')';
    return JS_NewStringLen(ctx, description.data(), description.size());
}

JSValue throws_bind(JSContext *ctx, bridge::Lambda lambda, JSValueConst type, JSValueConst message)
{
    using namespace bridge;
    Strong<void> predicate{ctx, FunctionData<throws_call>::bind(ctx, lambda, type, message), false};
    if(JS_IsException(predicate)) return JS_EXCEPTION;

    Strong<void> description{ctx, FunctionData<throws_description>::bind(ctx, lambda, type, message), false};
    if(JS_IsException(description)) return JS_EXCEPTION;

    if(JS_DefinePropertyValueStr(ctx, predicate, "toString", description.release(), JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE) < 0)
        return JS_EXCEPTION;
    return predicate.release();
}

JSValue throws_1(JSContext *ctx, bridge::Lambda lambda)
{
    return throws_bind(ctx, lambda, JS_UNDEFINED, JS_UNDEFINED);
}

JSValue throws_2(JSContext *ctx, bridge::Lambda lambda, bridge::String message)
{
    return throws_bind(ctx, lambda, JS_UNDEFINED, message);
}

JSValue throws_3(JSContext *ctx, bridge::Lambda lambda, bridge::Lambda type)
{
    return throws_bind(ctx, lambda, type, JS_UNDEFINED);
}

JSValue throws_4(JSContext *ctx, bridge::Lambda lambda, bridge::Lambda type, bridge::String message)
{
    return throws_bind(ctx, lambda, type, message);
}

using throws = bridge::Function<throws_1, throws_2, throws_3, throws_4>;

JSCFunctionListEntry func[] = {
    JS_CFUNC_DEF("assert", 1, bridge::Function<assert>::invoke),
    JS_CFUNC_DEF("throws", 1, throws::invoke)
};

int init(JSContext *ctx, JSModuleDef *m)
{
    AssertionError::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

} // namespace

std::atomic<std::uint64_t> assertions{0};

void notojs_init_assert()
{
    AssertionError::init();
}

void notojs_init_assert(JSRuntime *rt)
{
    AssertionError::init(rt);
}

void notojs_init_assert(detail::Config const &) {}

JSModuleDef *notojs_init_assert(JSContext *ctx, const char *name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, name, init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, AssertionError::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // namespace notojs
