#include <bridge.hpp>
#include <map>

namespace {

struct TestError : bridge::Exception<TestError>
{
    TestError(int code): code{code} {}

    void populate(JSContext *ctx, bridge::Object &obj) const
    {
        obj.set("code", bridge::Number(ctx, code));
    }

private:
    int code;
};

JSValue raise(JSContext *ctx, JSValue self, int argc, JSValueConst *argv)
{
    return TestError::throw_(ctx, {42});
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("raise", 1, &raise)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    TestError::init(ctx, m);
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExport(ctx, mod, TestError::name());
    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
