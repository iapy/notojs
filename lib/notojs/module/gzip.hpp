#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_gzip();
void notojs_init_gzip(JSRuntime *);
void notojs_init_gzip(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_gzip(JSContext *, const char *);

} // namespace notojs

