#pragma once
#include <notojs/script/config.hpp>

namespace notojs {

JSValue notojs_init_dom(JSContext *ctx);
JSValue notojs_init_dom(JSContext *ctx, ScriptConfig);

} // namespace notojs
