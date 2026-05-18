#include <iostream>

#include "dbase/stl/vector.h"
#include "dbase/stl/util.h"
#include "dbase/stl/astring.h"
#include "dbase/stl/map.h"
#include "dbase/stl/common.h"
#include "dbase/stl/astring.h"

template <typename T, class = void>
struct has_iterator : std::false_type
{
};

template <typename T>
struct has_iterator<T, std::void_t<typename T::iterator>> : std::true_type
{
};

DECL__V(has_iterator);

template <typename T>
struct is_string : dbase::stl::false_type
{
};

template <typename... Args>
struct is_string<dbase::stl::basic_string<Args...>> : dbase::stl::true_type
{
};

template <class... Args>
constexpr bool is_string_v = is_string<Args...>::value;

template <typename T, class = void>
struct PrintOne
{
        void operator()(const T&) const
        {
            std::cout << "unknown type";
        }
};

template <typename T>
struct PrintOne<T, std::enable_if_t<std::is_trivial<T>::value && !std::is_array_v<T>, void>>
{
        void operator()(const T& val) const
        {
            std::cout << val;
        }
};

// char* 特化
template <typename T>
struct PrintOne<T, std::enable_if_t<std::is_same_v<std::remove_cv<T>, char*>, void>>
{
        void operator()(const T& val) const
        {
            std::cout << "\"" << val << "\"";
        }
};

// 或者针对传统数组语法
template <typename T, std::size_t N>
struct PrintOne<T[N], void>
{
        void operator()(const T (&arr)[N]) const
        {
            // if constexpr (std::is_same_v<T, char>)
            // {
            //     std::cout << "\"" << arr << "\"";
            //     return;
            // }
            std::cout << "[";
            bool first = true;
            for (std::size_t i = 0; i < N; ++i)
            {
                if (!first) std::cout << ", ";
                first = false;
                PrintOne<T>()(arr[i]);
            }
            std::cout << "]";
        }
};

template <std::size_t N>
struct PrintOne<char[N], void>
{
        void operator()(const char (&arr)[N]) const
        {
            std::cout << "\"" << arr << "\"";
        }
};

// string 特化
template <typename T>
struct PrintOne<T, std::enable_if_t<is_string_v<T>, void>>
{
        void operator()(const T& val) const
        {
            std::cout << "\"" << val << "\"";
        }
};

// pair 特化
template <typename T>
struct PrintOne<T, std::enable_if_t<dbase::stl::is_pair_v<T>, void>>
{
        void operator()(const T& val) const
        {
            std::cout << "(";
            PrintOne<typename T::first_type>()(val.first);
            std::cout << ", ";
            PrintOne<typename T::second_type>()(val.second);
            std::cout << ")";
        }
};

// 可迭代容器 特化
template <typename T>
struct PrintOne<T, std::enable_if_t<!is_string_v<T> && has_iterator_v<T>, void>>
{
        void operator()(const T& val) const
        {
            bool first = true;
            // std::cout << typeid(T).name() << " {";
            std::cout << "{";
            for (const typename T::value_type& i : val)
            {
                if (!first)
                {
                    std::cout << ", ";
                }
                first = false;
                PrintOne<typename T::value_type>()(i);
            }
            std::cout << "}";
        }
};

void print()
{
    std::cout << std::endl;
}

template <typename T, typename... Args>
void print(const T& val, Args... args)
{
    PrintOne<T>()(val);

    print(args...);
}


s32 main()
{
    {
        print("Test: print raw array");
        s32 v[3] = {1, 2, 3};

        for (auto& x : v)
        {
            print("x = ", x);
        }

        print(v);
    }

    {
        print("Test: print vector");
        dbase::stl::vector<s32> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);

        for (auto& x : v)
        {
            print("x = ", x);
        }

        print(v);
    }

    {
        print("Test: print vector<pair>");
        dbase::stl::vector<dbase::stl::pair<s32, dbase::stl::pair<double, dbase::stl::string>>> v;
        v.push_back({1, {2.3, "hello"}});
        v.push_back({2, {4.5, "world"}});
        v.push_back({3, {6.7, "!"}});

        for (auto& x : v)
        {
            print("val = ", x);
        }

        print(v);
    }

    {
        print("Test: print vector<pair<pair>>");
        dbase::stl::vector<dbase::stl::pair<s32, dbase::stl::pair<double, const char*>>> v;
        v.push_back({1, {2.3, "hello"}});
        v.push_back({2, {4.5, "world"}});
        v.push_back({3, {6.7, "!"}});

        for (auto& x : v)
        {
            print("val = ", x);
        }

        print(v);
    }

    {
        print("Test: print map 01");
        dbase::stl::map<s32, dbase::stl::string> mp;
        mp[1] = "hello";
        mp[2] = "world";
        mp[3] = "!";

        for (auto& x : mp)
        {
            print("val = ", x);
        }

        print(mp);
    }

    {
        print("Test: print map 02");
        dbase::stl::map<s32, dbase::stl::map<dbase::stl::string, double>> mp;
        mp[1]["hello"] = 1.2;
        mp[1]["world"] = 2.3;
        mp[2]["hello"] = 3.4;
        mp[2]["world"] = 4.5;
        mp[3]["hello"] = 5.6;
        mp[3]["world"] = 6.7;

        for (auto& x : mp)
        {
            print("val = ", x);
        }

        print(mp);
    }

    {
        print("Test: sort");
        dbase::stl::vector<s32> v{1, 7, 3, 4, 5, 6, 2, 9, 0, 8};
        dbase::stl::sort(v.begin(), v.end());
        print(v);
    }

    {
        print("Test: make_heap");
        dbase::stl::vector<s32> v{1, 7, 3, 4, 5, 6, 2, 9, 0, 8};
        dbase::stl::make_heap(v.begin(), v.end());
        print(v);
    }

    {
        print("Test: sort_heap");
        dbase::stl::vector<s32> v{1, 7, 3, 4, 5, 6, 2, 9, 0, 8};
        dbase::stl::make_heap(v.begin(), v.end());
        dbase::stl::sort_heap(v.begin(), v.end());
        print(v);
    }
    return 0;
}