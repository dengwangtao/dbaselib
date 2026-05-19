
#pragma once

#ifndef DBASE_STL_COMMON_H_
#define DBASE_STL_COMMON_H_

#include <cstdint>

#define DECL__V(trait_name) \
    template <class T>   \
    constexpr bool trait_name##_v = trait_name<T>::value;

#define DECL__V2(trait_name)        \
    template <class T, class T2> \
    constexpr bool trait_name##_v = trait_name<T, T2>::value;

#define DECL__T(trait_name) \
    template <class T>   \
    using trait_name##_t = typename trait_name<T>::type;

#define DECL__T2(trait_name)        \
    template <class T, class T2> \
    using trait_name##_t = typename trait_name<T, T2>::type;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;
using f64 = double;

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

template <class... _Types>
using void_t = void;

};  // namespace dbase::stl

#endif  // !DBASE_STL_COMMON_H_