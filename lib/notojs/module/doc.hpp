#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_doc();
void notojs_init_doc(JSRuntime *);
void notojs_init_doc(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_doc(JSContext *, const char *);

} // namespace notojs
