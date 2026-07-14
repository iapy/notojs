#pragma once
#include <boost/config.hpp>
#include <tuple>

template<typename T>
struct Component
{
    T *instance{nullptr};
};

template<typename ...Ts>
class Container;

template<typename ...Ts>
struct Requires : protected Component<Ts>...
{
    template<typename T>
    BOOST_FORCEINLINE bool has() const { return Component<T>::instance != nullptr; }

    template<typename T>
    BOOST_FORCEINLINE T &get() { return *(Component<T>::instance); }

    template<typename T>
    BOOST_FORCEINLINE T const &get() const { return *(Component<T>::instance); }

    template<typename...>
    friend class Container;

private:
    template<typename ...Us>
    void initialize(std::tuple<Us...> &container)
    {
        if constexpr (sizeof ...(Ts)) initialize_<Ts...>(container);
    }

    template<typename V, typename ...Vs, typename ...Us>
    void initialize_(std::tuple<Us...> &container)
    {
        if constexpr (std::disjunction_v<std::is_same<V, Us>...>)
            Component<V>::instance = &std::get<V>(container);
        if constexpr (sizeof ...(Vs))
            initialize_<Vs...>(container);
    }
};

template<typename ...Ts>
class Container
{
public:
    Container()
    {
        init<Ts...>();
    }

    template<typename T, typename ...Args>
    int main(Args&&... args)
    {
        return std::get<T>(components).main(std::forward<Args>(args)...);
    }

private:
    template<typename U, typename ...Us>
    void init()
    {
        std::get<U>(components).initialize(components);
        if constexpr (sizeof ...(Us)) init<Us...>();
    }

    std::tuple<Ts...> components;
};

