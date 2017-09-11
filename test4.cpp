#include "utility.hpp"
#include <iostream>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <string>

using namespace utility;
using std::is_same;

// testing: tl_subrange
typedef typelist<char, bool, int[2], double*> tl_subrange_test;
static_assert( is_same<tl_subrange<tl_subrange_test, 1, 2>::type, typelist<bool, int[2]>>::value, "tl_subrange failed" );

// testing: tuple and tl_apply
typedef typelist<int, bool, const char*> tuple_tlist;
typedef tl_apply<tuple, tuple_tlist>::type tuple_test;
static_assert( is_same< bool, tl_type_at<1, tuple_tlist>::type >::value, "using tuple as typelist failed" );

// testing tuple and tl_subrange
typedef tuple<int, bool, const char*> tuple_test;
typedef tl_subrange<tuple_test, 1, 2>::type subtuple_type;
static_assert( is_same<subtuple_type, typelist<bool, const char*>>::value, "tl_subrange failed" );

struct printer
{
    int operator()(int v){std::cout << v << std::endl; return v*v;}
    bool operator()(bool v){std::cout << v << std::endl; return !v;}
    const char* operator()(const char* v){std::cout << v << std::endl; return "goodbye";}
};

typedef ct_make_index_range<3, 7>::type index_range;

void print_index(){}
template <typename T, typename ... Ts>
void print_index(T&& v, Ts... nxt)
{
    std::cout << v << std::endl;
    print_index(nxt...);
}

template <template <size_t...> typename IR, size_t ... Is>
void print_indices(IR<Is...>&& range)
{
    print_index(Is...);
}

void applied(int i, bool b, const char* c) {}

int main()
{
    // testing: tuple and ct_index_range and ct_make_index_range
    print_indices(index_range());
    tuple_test my_tuple(5, true, "hello");
    tuple_apply(applied, my_tuple);
    auto return_tuple = tuple_foreach(printer(), my_tuple);
    tuple_foreach_noreturn([](auto& v){ std::cout << v << std::endl; }, return_tuple);
}
