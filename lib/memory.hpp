#pragma once
#include <quickjs/quickjs.h>
#include <boost/config.hpp>
#include <memory>

namespace bridge {

struct Deleter
{
    BOOST_FORCEINLINE void operator () (JSContext *ct)
    {
        JSRuntime *rt = JS_GetRuntime(ct);
        JS_FreeContext(ct);
        JS_RunGC(rt);
    }
    BOOST_FORCEINLINE void operator () (JSRuntime *rt)
    {
        JS_FreeRuntime(rt);
    }
};

using Context = std::unique_ptr<JSContext, Deleter>;
using Runtime = std::unique_ptr<JSRuntime, Deleter>;

} // namespace bridge
