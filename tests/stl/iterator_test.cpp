#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_approx.hpp>
#include <iostream>
#include <sstream>

#include "dbase/stl/iterator.h"
#include "dbase/stl/stream_iterator.h"

TEST_CASE("stream iterator", "[stl]")
{
    static_assert(dbase::stl::is_exactly_input_iterator<dbase::stl::istream_iterator<s32>>::value,
                  "istream_iterator must have input_iterator_tag)");

    std::istringstream is("1 2 3");
    dbase::stl::istream_iterator<s32> first{is}, last;
    std::cout << dbase::stl::distance(first, last) << '\n';

    std::istringstream istream("1 2 3 4 5 6");
    dbase::stl::istream_iterator<s32> beg{istream}, end;
    for (; beg != end; ++beg)
    {
        std::cout << *beg << " ";
    }
    std::cout << '\n';
}