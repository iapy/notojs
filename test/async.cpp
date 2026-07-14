#include <engine.hpp>
#include <bridge.hpp>

namespace {

class Task : public notojs::Task
{
public:
    Task(std::int64_t n): n{n} {}

protected:
    Step step() override
    {
        if(n <= 0) return Finish;
  
        result.push_back(std::to_string(n--));
        return Again;
    }

    Then then(JSContext *ctx, JSValue &v) override
    {
        if(result.empty())
            return Reject;

        bridge::Array a{ctx};
        for(auto const &n : result)
            a.append(bridge::String(ctx, n));

        return (v = a), Resolve;
    }

private:
    std::int64_t n;
    std::vector<std::string> result;
};

JSValue run(JSContext *ctx, bridge::Number p)
{
    return std::make_shared<Task>(p)->run(ctx);
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("run", 1, &bridge::Function<&run>::invoke)
};

} // namespace

static int init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, func, sizeof(func)/sizeof(func[0]));
}

extern "C" {

JSModuleDef *js_init_module(JSContext *ctx, const char *module_name)
{
    JSModuleDef *mod = JS_NewCModule(ctx, module_name, &init);
    if(!mod) return NULL;

    JS_AddModuleExportList(ctx, mod, func, sizeof(func)/sizeof(func[0]));
    return mod;
}

} // extern "C"
