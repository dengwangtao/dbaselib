#ifndef DBASE_STL_TYPE_TRAITS_H_
#define DBASE_STL_TYPE_TRAITS_H_

// 这个头文件用于提取类型信息

// use standard header for type_traits
#include <type_traits>
#include "dbase/stl/util.h"
namespace dbase::stl
{

/*****************************************************************************************/
// type traits

// is_pair

// 主模板：默认非 pair 类型
// template <class T, class = void>
// struct is_pair : dbase::stl::false_type {};

// // 特化模板：当 T 有 first_type 和 second_type 时继承 true_type
// template <class T>
// struct is_pair<T, dbase::stl::void_t<typename T::first_type, typename T::second_type>>
//     : dbase::stl::true_type {};

template <class T>
struct is_pair : dbase::stl::false_type
{
};

template <class T1, class T2>
struct is_pair<dbase::stl::pair<T1, T2>> : dbase::stl::true_type
{
};

DECL__V(is_pair);

}  // namespace dbase::stl

#endif  // !DBASE_STL_TYPE_TRAITS_H_
