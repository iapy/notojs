#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_dom();
void notojs_init_dom(JSRuntime *);
void notojs_init_dom(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_dom(JSContext *, const char *);

} // namespace notojs

