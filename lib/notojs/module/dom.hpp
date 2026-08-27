#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_dom();
void notojs_init_dom(JSRuntime *);
void notojs_init_dom(detail::Config const &);
JSModuleDef *notojs_init_dom(JSContext *, const char *);

} // namespace notojs
