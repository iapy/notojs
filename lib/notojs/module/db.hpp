#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>

namespace notojs {

void notojs_init_db();
void notojs_init_db(JSRuntime *);
void notojs_init_db(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_db(JSContext *, const char *);

} // namespace notojs
