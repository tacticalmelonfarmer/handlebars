#include "utility.hpp"
#include "typelist.hpp"
#include <iostream>
#include <type_traits>

using namespace utility;
using std::is_same;

struct custom_t {};
template <typename ... Ts>
struct apply_to {};

typedef typelist<char, short, int, float, double, const char*, custom_t> tlist;

static_assert( is_same<typename tl_join<typelist<char, short, int, float>, typelist<double, const char*, custom_t>>::type, tlist>::value, "tl_join failed" );
static_assert( is_same<typename tl_insert<typelist<char, short, const char*, custom_t>, 2, typelist<int, float, double>>::type, tlist>::value, "tl_insert failed" );
static_assert( is_same<typename tl_insert_t<typelist<char, short, const char*, custom_t>, 2, int, float, double>::type, tlist>::value, "tl_insert_t failed" );
static_assert( is_same<typename tl_remove<typelist<char, short, bool, int, float, double, const char*, custom_t>, 2>::type, tlist>::value, "tl_remove failed" );
static_assert( is_same<typename tl_remove_subrange<typelist<char, short, bool, bool, bool, int, float, double, const char*, custom_t>, 2, 4>::type, tlist>::value, "tl_remove_subrange failed" );
static_assert( is_same<typename tl_pop_back<typelist<char, short, int, float, double, const char*, custom_t, bool>>::type, tlist>::value, "tl_pop_back failed" );
static_assert( is_same<typename tl_pop_front<typelist<bool, char, short, int, float, double, const char*, custom_t>>::type, tlist>::value, "tl_pop_front failed" );

int main()
{
}
