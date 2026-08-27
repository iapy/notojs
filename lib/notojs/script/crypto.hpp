#pragma once
#include <notojs/script/config.hpp>

namespace notojs {

JSValue notojs_init_crypto(JSContext *ctx);
JSValue notojs_init_crypto(JSContext *ctx, ScriptConfig);

} // namespace notojs
