#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_fs();
void notojs_init_fs(JSRuntime *);
void notojs_init_fs(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_fs(JSContext *, const char *);

} // namespace notojs

