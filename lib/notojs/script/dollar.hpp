#pragma once
#include <notojs/script/config.hpp>

namespace notojs {

JSValue notojs_init_dollar(JSContext *ctx);
JSValue notojs_init_dollar(JSContext *ctx, ScriptConfig);

} // namespace notojs
