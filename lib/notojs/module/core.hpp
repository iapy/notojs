#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_core();
void notojs_init_core(JSRuntime *);
void notojs_init_core(detail::Config const &);
JSModuleDef *notojs_init_core(JSContext *, const char *);

} // namespace notojs
