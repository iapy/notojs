#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_db();
void notojs_init_db(JSRuntime *);
void notojs_init_db(detail::Config const &);
JSModuleDef *notojs_init_db(JSContext *, const char *);

} // namespace notojs
