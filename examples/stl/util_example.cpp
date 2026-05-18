#include "dbase/stl/util.hpp"
#include "dbase/log/log.h"

int main()
{
    int x = 1;
    int& ref_x = x;

    using t1 = dbase::stl::remove_reference<decltype(ref_x)>;
    t1 y;


    auto p1 = dbase::stl::make_pair(1, 2);
    auto p2 = dbase::stl::make_pair(1, std::string("str"));

    auto p3 = p2;
    
    
    DBASE_LOG_INFO("p3=[{},{}]", p3.first, p3.second);
    auto p4 =  dbase::stl::pair<int, float>(1, 0.5);
    auto p5 =  dbase::stl::pair<long long, double>();
    p5 = p4;
    dbase::stl::pair<long long, double> p6(p4);
    p6.first ++;

    dbase::stl::swap(p5, p6);

    DBASE_LOG_INFO("p5=[{},{}]", p5.first, p5.second);
    DBASE_LOG_INFO("p6=[{},{}]", p6.first, p6.second);


    return 0;
}