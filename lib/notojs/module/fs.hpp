#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_fs();
void notojs_init_fs(JSRuntime *);
void notojs_init_fs(detail::Config const &);
JSModuleDef *notojs_init_fs(JSContext *, const char *);

} // namespace notojs
