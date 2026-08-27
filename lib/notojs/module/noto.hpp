#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_noto();
void notojs_init_noto(JSRuntime *);
void notojs_init_noto(detail::Config const &);
JSModuleDef *notojs_init_noto(JSContext *, const char *);

} // namespace notojs
