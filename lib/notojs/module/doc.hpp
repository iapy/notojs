#pragma once
#include <notojs/detail/config.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_doc();
void notojs_init_doc(JSRuntime *);
void notojs_init_doc(detail::Config const &);
JSModuleDef *notojs_init_doc(JSContext *, const char *);

} // namespace notojs
