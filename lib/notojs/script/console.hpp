#pragma once
#include <notojs/script/config.hpp>

namespace notojs {

JSValue notojs_init_console(JSContext *ctx);
JSValue notojs_init_console(JSContext *ctx, ScriptConfig);

} // namespace notojs
