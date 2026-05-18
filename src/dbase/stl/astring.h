#ifndef DBASE_STL_ASTRING_H_
#define DBASE_STL_ASTRING_H_

// 定义了 string, wstring, u16string, u32string 类型

#include "dbase/stl/basic_string.h"

namespace dbase::stl
{

using string = dbase::stl::basic_string<char>;
using wstring = dbase::stl::basic_string<wchar_t>;
using u16string = dbase::stl::basic_string<char16_t>;
using u32string = dbase::stl::basic_string<char32_t>;

}  // namespace dbase::stl
#endif  // !DBASE_STL_ASTRING_H_
