#include "dbase/math/fraction.h"
#include <iostream>

using namespace dbase::math;

int main()
{
    Fraction f(1, 2);  // 1/2
    Fraction g(3, 4);  // 3/4

    auto h = f + g;  // 5/4

    std::cout << h << std::endl;  // 5/4

    return 0;
}