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
    if(auto res = lambda(); !bridge::Boolean::check(ctx, res))
    {
        String expr{ctx, lambda};
        return JS_ThrowTypeError(ctx, "Expecting bool: %s", static_cast<std::string_view const &>(expr).data());
    }
    else if(!JS_ToBool(ctx, res))
    {
        String expr{ctx, lambda};
        if(std::cmatch m; std::regex_match(std::begin(expr), std::end(expr), m, std::regex{R"(\s*\(\s*\)\s*=>\s*(.+))"}))
        {
            return AssertionError::throw_(ctx, AssertionError{m[1]});
        }
        else
        {
            return AssertionError::throw_(ctx, AssertionError{expr});
        }
    }
    ++assertions;
    return JS_UNDEFINED;
}

JSValue throws_1(JSContext *ctx, bridge::Lambda lambda)
{
    using namespace bridge;
    if(auto res = lambda(); Promise::check(ctx, res))
    {
        auto p = Promise{ctx, res}.wrap(
            [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv){
                return JS_FALSE;
            },
            [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv){
                return JS_TRUE;
            }
        );

        auto *ptr = Global::Context::ptr(ctx);
        ptr->wait(ctx);

        return JS_PromiseResult(ctx, p);
    }
    else
    {
        return Error::check(ctx, res) ? JS_TRUE : JS_FALSE;
    }
}

JSValue throws_2(JSContext *ctx, bridge::Lambda lambda, bridge::String message)
{
    using namespace bridge;
    if(auto res = lambda(); Promise::check(ctx, res))
    {
        auto p = Promise{ctx, res}.wrap(
            [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv){
                return JS_UNDEFINED;
            },
            [](JSContext *ctx, JSValueConst, int argc, JSValueConst *argv){
                return argc ? JS_DupValue(ctx, argv[0]) : JS_UNDEFINED;
            }
        );

        auto *ptr = Global::Context::ptr(ctx);
        ptr->wait(ctx);

        auto result = Strong<Error>{ctx, JS_PromiseResult(ctx, p)};
        if(auto msg = result.message(); msg)
        {
            return static_cast<std::string_view const &>(message) == *msg ? JS_TRUE : JS_FALSE;
        }
    }
    else if(!Error::check(ctx, res))
    {
        return JS_FALSE;
    }
    auto err = Strong<Error>(ctx, JS_GetException(ctx));
    if(auto msg = err.message(); msg)
    {
        return static_cast<std::string_view const &>(message) == *msg ? JS_TRUE : JS_FALSE;
    }

    auto msg = Strong<String>(ctx, JS_ToString(ctx, err));
    return static_cast<std::string_view const &>(message) == static_cast<std::string_view const &>(msg) ? JS_TRUE : JS_FALSE;
}

using throws = bridge::Function<throws_1, throws_2>;

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
