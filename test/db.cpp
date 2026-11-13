#include <lmdbxx/lmdb++.h>
#include <module.hpp>

namespace {

JSValue count(JSContext *ctx, notojs::NOTOJS_VERSION_NS::db::Handle db)
{
    auto ref = db->get();
    if(!ref->db) return notojs::NOTOJS_VERSION_NS::db::error(ctx);
    return bridge::Number(ctx, static_cast<std::uint64_t>(ref->db->stat(*ref->tx).ms_entries));
}

JSCFunctionListEntry const func[] = {
    JS_CFUNC_DEF("count", 1, &bridge::Function<&count>::invoke)
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
