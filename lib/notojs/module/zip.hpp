#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_zip();
void notojs_init_zip(JSRuntime *);
void notojs_init_zip(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_zip(JSContext *, const char *);

} // namespace notojs

