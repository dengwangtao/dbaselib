#pragma once

#include <cstdint>

#define DECL__V(struct_type) \
    template <class T>       \
    constexpr bool struct_type##_v = struct_type<T>::value;

#define DECL__V2(struct_type)    \
    template <class T, class T2> \
    constexpr bool struct_type##_v = struct_type<T, T2>::value;

#define DECL__T(struct_type) \
    template <class T>       \
    using struct_type##_t = typename struct_type<T>::type;

#define DECL__T2(struct_type)    \
    template <class T, class T2> \
    using struct_type##_t = typename struct_type<T, T2>::type;

namespace dbase::stl
{

template <class T, T val>
struct constant
{
        static constexpr T value = val;
        using value_type = T;
        using type = constant;

        constexpr operator value_type() const noexcept { return value; }
        constexpr value_type operator()() const noexcept { return value; }
};

template <bool val>
using bool_constant = constant<bool, val>;

using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

};  // namespace dbase::stl