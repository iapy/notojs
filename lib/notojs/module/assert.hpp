#pragma once
#include <boost/property_tree/ptree.hpp>
#include <quickjs/quickjs.h>
#include <cstdint>
#include <atomic>

namespace notojs {

extern std::atomic<std::uint64_t> assertions;

void notojs_init_assert();
void notojs_init_assert(JSRuntime *);
void notojs_init_assert(boost::property_tree::ptree const &);
JSModuleDef *notojs_init_assert(JSContext *, const char *);

} // namespace notojs

