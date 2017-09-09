#include "dispatcher.hpp"
#include "utility.hpp"
#include <iostream>
#include <functional>
#include <type_traits>

using namespace utility;
using std::is_same;

// testing: ct_select
typedef ct_select<2, char, float, int>::type tl_ct_select_test0;
static_assert( is_same<int, tl_ct_select_test0>::value, "ct_select failed" );

// testing: typelist
typedef typelist<int, double, char> tl_typelist_test;

// testing: tl_apply
typedef typelist<int, double, char> tl_apply_test;
template <class A, class B, class C>
struct tl_applied_test {};
static_assert( is_same<tl_applied_test<int, double, char>, tl_apply<tl_applied_test, tl_apply_test>::type>::value, "tl_apply failed" );

// testing: tl_apply_before
template <class A, class B, class C, class D, class E>
struct tl_applied_before_test {};
static_assert( is_same<tl_applied_before_test<int, double, char, bool, char>, tl_apply_before<tl_applied_before_test, tl_apply_test, bool, char>::type>::value, "tl_apply_before failed" );

// testing: ct_if_else
static_assert( is_same<ct_if_else<true, int, float>::type, int>::value, "ct_if_else failed" );
static_assert( is_same<ct_if_else<false, int, float>::type, float>::value, "ct_if_else failed" );

// testing: tl_get
typedef typelist<char, signed, unsigned, float, double, std::function<void()>, bool> tl_get_test;
typedef tl_get<3, tl_get_test>::type real;
static_assert( is_same<real, float>::value, "tl_get failed" );

// testing: tl_push_back
typedef typelist<int, bool, bool> tl_push_back_part;
typedef tl_push_back<tl_push_back_part, int, double>::type tl_push_back_test;
static_assert( is_same< tl_push_back_test, typelist<int, bool, bool, int, double> >::value, "tl_push_back failed" );

// testing: tl_push_front
typedef typelist<int, bool, bool> tl_push_front_part;
typedef tl_push_front<tl_push_front_part, int, double>::type tl_push_front_test;
static_assert( is_same< tl_push_front_test, typelist<int, double, int, bool, bool> >::value, "tl_push_front failed" );

// testing: tl_back
static_assert( is_same<tl_back<tl_typelist_test>::type, char>::value, "tl_back failed" );

// testing: tuple and tl_apply
typedef typelist<int, bool, const char*> tuple_tlist;
typedef tl_apply<tuple, tuple_tlist>::type tuple_test;
static_assert( is_same< bool, tl_get<1, tuple_tlist>::type >::value, "using tuple as typelist failed" );

int main()
{
    tuple_test my_tuple(5, true, "hello");
    std::cout << get<2>(my_tuple) << std::endl;
}