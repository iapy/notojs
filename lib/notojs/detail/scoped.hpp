#pragma once
#include <boost/config.hpp>

namespace notojs::detail {

template<typename F>
class Scoped
{
public:
    BOOST_FORCEINLINE explicit Scoped(F &&f): f{f} {}
    BOOST_FORCEINLINE ~Scoped() { f(); }
private:
    F f;
};

template<typename F>
Scoped(F&&) -> Scoped<F>;

} // namespace notojs::detail
