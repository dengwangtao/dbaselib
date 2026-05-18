#pragma once

#include <type_traits>
#include "dbase/stl/util.hpp"

namespace dbase::stl
{

// template <class T>
// struct is_pair : dwt_stl::false_type
// {
// };

// template <class T1, class T2>
// struct is_pair<dbase::stl::pair<T1, T2>> : dwt_stl::true_type
// {
// };

// 主模板：默认非 pair 类型
template <class T, class = void>
struct is_pair : dbase::stl::false_type
{
};

// 特化模板：当 T 有 first_type 和 second_type 时继承 true_type
template <class T>
struct is_pair<T, dbase::stl::void_t<typename T::first_type, typename T::second_type>>
    : dbase::stl::true_type
{
};

DECL__V(is_pair);

}  // namespace dbase::stl