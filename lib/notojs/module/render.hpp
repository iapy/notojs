#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_render();
void notojs_init_render(JSRuntime *);
void notojs_init_render(detail::Config const &);
JSModuleDef *notojs_init_render(JSContext *, const char *);

} // namespace notojs
