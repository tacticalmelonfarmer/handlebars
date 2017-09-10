#include "utility.hpp"
#include <chrono>
#include <iostream>

using namespace utility;

typedef tuple<int, int, int, int, int, int> tuple6_t;
typedef tl_apply<tuple, tl_join<tuple6_t, tuple6_t>::type>::type tuple12_t;
typedef tl_apply<tuple, tl_join<tuple12_t, tuple12_t>::type>::type tuple24_t;
typedef tl_apply<tuple, tl_join<tuple24_t, tuple24_t>::type>::type tuple48_t;
typedef tl_apply<tuple, tl_join<tuple48_t, tuple48_t>::type>::type tuple96_t;

int main()
{
    tuple<const char *> some_tuple("hello");
    tuple_apply([](auto v){ std::cout << v << std::endl; }, some_tuple);
    
    tuple96_t my_big_tuple;
    tuple_foreach_noreturn([](auto v){ std::cout << v <<std::endl; }, my_big_tuple);

    int refd_to = 10;
    tuple<int&> ref_tuple(refd_to);
    refd_to = 5;
    std::cout << tuple_get<0>(ref_tuple) << std::endl;
}