#pragma once

#include <cstddef>
#include <concepts>
#include <compare>
#include "dbase/stl/common.hpp"

namespace dbase::stl
{

// -----------------------------------
// remove_reference 用于移除 T 的引用属性
template <class T>
struct remove_reference
{
        using type = T;
        using const_type = const T;
};

template <class T>
struct remove_reference<T&>
{
        using type = T;
        using const_type = const T;
};

template <class T>
struct remove_reference<T&&>
{
        using type = T;
        using const_type = const T;
};

DECL__T(remove_reference);

// -----------------------------------
// remove_const 用于移除T的const属性
template <class T>
struct remove_const
{
        using type = T;
};

template <class T>
struct remove_const<const T>
{
        using type = T;
};

DECL__T(remove_const);

// -----------------------------------
// remove_cv 用于移除T的 const volatile属性
template <class T>
struct remove_cv
{
        using type = T;
};

template <class T>
struct remove_cv<const T>
{
        using type = T;
};

template <class T>
struct remove_cv<volatile T>
{
        using type = T;
};

template <class T>
struct remove_cv<const volatile T>
{
        using type = T;
};

DECL__T(remove_cv);

// -----------------------------------
// is_same  用于判断两个类型是否完全相同
template <class T1, class T2>
struct is_same : false_type
{
};

template <class T>
struct is_same<T, T> : true_type
{
};

DECL__V2(is_same);

// -----------------------------------
// 判断是否为左值引用
template <class T>
struct is_lvalue_reference : false_type
{
};

template <class T>
struct is_lvalue_reference<T&> : true_type
{
};

DECL__V(is_lvalue_reference);

// -----------------------------------
// 判断是否为右值引用
template <class T>
struct is_rvalue_reference : false_type
{
};

template <class T>
struct is_rvalue_reference<T&&> : true_type
{
};

// -----------------------------------

// move
template <class T>
typename dbase::stl::remove_reference_t<T>&& move(T&& arg) noexcept
{
    // 移除引用 再强制转换为右值引用
    return static_cast<typename dbase::stl::remove_reference_t<T>&&>(arg);
}

// forward
template <class T>
T&& forward(typename dbase::stl::remove_reference_t<T>& arg) noexcept
{
    return static_cast<T&&>(arg);  // 利用引用折叠 & && -> &
}

template <class T>
T&& forward(typename dbase::stl::remove_reference_t<T>&& arg) noexcept
{
    static_assert(!dbase::stl::is_lvalue_reference_v<T>, "bad forward");
    return static_cast<T&&>(arg);  // // 利用引用折叠 && && -> &&
}

// swap
template <class Tp>
void swap(Tp& lhs, Tp& rhs)
{
    auto tmp(dbase::stl::move(lhs));
    lhs = dbase::stl::move(rhs);
    rhs = dbase::stl::move(tmp);
}

template <class ForwardIter1, class ForwardIter2>
ForwardIter2 swap_range(ForwardIter1 first1, ForwardIter1 last1, ForwardIter2 first2)
{
    for (; first1 != last1; ++first1, (void)++first2)
    {
        dbase::stl::swap(*first1, *first2);
    }
    return first2;
}

template <class Tp, size_t N>
void swap(Tp (&a)[N], Tp (&b)[N])
{
    dbase::stl::swap_range(a, a + N, b);
}

// --------------------------------------------------------------------------------------

// 判断类型是否可默认构造
template <class T>
concept default_constructible = std::is_default_constructible_v<T>;

// 判断类型可拷贝构造 + 可隐式转换
template <class From, class To>
concept copy_convertible =
        std::is_copy_constructible_v<From> && std::is_convertible_v<const From&, To>;

// --------------------------------------------------------------------------------------
// pair

// 结构体模板 : pair
// 两个模板参数分别表示两个数据的类型
// 用 first 和 second 来分别取出第一个数据和第二个数据
template <class Ty1, class Ty2>
struct pair
{
        using first_type = Ty1;
        using second_type = Ty2;
        using self = pair<Ty1, Ty2>;

        first_type first;
        second_type second;

        // 默认构造函数
        constexpr pair()
            requires default_constructible<Ty1> && default_constructible<Ty2>
            : first(), second()
        {
        }

        // 带参构造：隐式版本 (拷贝+可转换)
        constexpr pair(const Ty1& a, const Ty2& b)
            requires copy_convertible<Ty1, Ty1> && copy_convertible<Ty2, Ty2>
            : first(a), second(b)
        {
        }

        // 带参构造：显式版本 (不可隐式转换)
        explicit constexpr pair(const Ty1& a, const Ty2& b)
            requires(
                            std::is_copy_constructible_v<Ty1>
                            && std::is_copy_constructible_v<Ty2>
                            && !(std::is_convertible_v<const Ty1&, Ty1> && std::is_convertible_v<const Ty2&, Ty2>))
            : first(a), second(b)
        {
        }

        // 万能引用构造：隐式版本 (完美转发)
        template <class U1, class U2>
        constexpr pair(U1&& a, U2&& b)
            requires(
                            std::is_constructible_v<Ty1, U1>
                            && std::is_constructible_v<Ty2, U2>
                            && std::is_convertible_v<U1, Ty1>
                            && std::is_convertible_v<U2, Ty2>)
            : first(stl::forward<U1>(a)),
              second(stl::forward<U2>(b))
        {
        }

        // 万能引用构造：显式版本
        template <class U1, class U2>
        explicit constexpr pair(U1&& a, U2&& b)
            requires(
                            std::is_constructible_v<Ty1, U1>
                            && std::is_constructible_v<Ty2, U2>
                            && !(std::is_convertible_v<U1, Ty1> && std::is_convertible_v<U2, Ty2>))
            : first(stl::forward<U1>(a)),
              second(stl::forward<U2>(b))
        {
        }

        // 拷贝其他 pair：隐式版本
        template <class Other1, class Other2>
        constexpr pair(const pair<Other1, Other2>& other)
            requires(
                            std::is_constructible_v<Ty1, const Other1&>
                            && std::is_constructible_v<Ty2, const Other2&>
                            && std::is_convertible_v<const Other1&, Ty1>
                            && std::is_convertible_v<const Other2&, Ty2>)
            : first(other.first),
              second(other.second)
        {
        }

        // 拷贝其他 pair：显式版本
        template <class Other1, class Other2>
        explicit constexpr pair(const pair<Other1, Other2>& other)
            requires(
                            std::is_constructible_v<Ty1, const Other1&>
                            && std::is_constructible_v<Ty2, const Other2&>
                            && !(std::is_convertible_v<const Other1&, Ty1> && std::is_convertible_v<const Other2&, Ty2>))
            : first(other.first),
              second(other.second)
        {
        }

        // 移动其他 pair：隐式版本
        template <class Other1, class Other2>
        constexpr pair(pair<Other1, Other2>&& other)
            requires(
                            std::is_constructible_v<Ty1, Other1>
                            && std::is_constructible_v<Ty2, Other2>
                            && std::is_convertible_v<Other1, Ty1>
                            && std::is_convertible_v<Other2, Ty2>)
            : first(stl::forward<Other1>(other.first)),
              second(stl::forward<Other2>(other.second))
        {
        }

        // 移动其他 pair：显式版本
        template <class Other1, class Other2>
        explicit constexpr pair(pair<Other1, Other2>&& other)
            requires(
                            std::is_constructible_v<Ty1, Other1>
                            && std::is_constructible_v<Ty2, Other2>
                            && !(std::is_convertible_v<Other1, Ty1> && std::is_convertible_v<Other2, Ty2>))
            : first(stl::forward<Other1>(other.first)),
              second(stl::forward<Other2>(other.second))
        {
        }

        pair(const pair&) = default;
        pair(pair&&) = default;
        ~pair() = default;

        // 拷贝赋值
        pair& operator=(const pair& rhs) = default;

        // 移动赋值
        pair& operator=(pair&& rhs) = default;

        // 拷贝赋值其他 pair
        template <class Other1, class Other2>
        pair& operator=(const pair<Other1, Other2>& other)
        {
            first = other.first;
            second = other.second;
            return *this;
        }

        // 移动赋值其他 pair
        template <class Other1, class Other2>
        pair& operator=(pair<Other1, Other2>&& other)
        {
            first = stl::forward<Other1>(other.first);
            second = stl::forward<Other2>(other.second);
            return *this;
        }

        // 交换函数
        void swap(pair& other) noexcept
        {
            stl::swap(first, other.first);
            stl::swap(second, other.second);
        }

        // --------------------------
        // 三路比较运算符 <=>
        // 自动生成 == != < > <= >=
        // --------------------------
        constexpr auto operator<=>(const self&) const noexcept = default;
};

// 全局函数，让两个数据成为一个 pair
template <class Ty1, class Ty2>
pair<Ty1, Ty2> make_pair(Ty1&& first, Ty2&& second)
{
    return pair<Ty1, Ty2>(dbase::stl::forward<Ty1>(first), dbase::stl::forward<Ty2>(second));
}

}  // namespace dbase::stl
