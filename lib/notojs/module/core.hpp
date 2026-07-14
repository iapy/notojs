#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_core();
void notojs_init_core(JSRuntime *);
void notojs_init_core(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_core(JSContext *, const char *);

} // namespace notojs

