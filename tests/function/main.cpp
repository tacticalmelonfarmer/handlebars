#include <cassert>
#include <handlebars/function.hpp>
#include <iostream>

using handlebars::function;

int
free_function(int i)
{
  return i + 1;
}

struct object
{
  int member(int i) { return i + 1; }
};

struct functor
{
  int operator()(int i) { return i + 1; }
};

int
main()
{
  // here we test every way to initialize and/or assign a functional thing to a handlebars::function<...>
  function<int(int)> f_lambda([](int i) -> int { return i + 1; });
  int j = 1;
  function<int(int)> f_ref_capturing_lambda([&j](int i) -> int { return i + j; });
  int k = 1;
  function<int(int)> f_copy_capturing_lambda([k](int i) -> int { return i + k; });
  function<int(int)> f_free(&free_function);
  function<int(int)> f_member(object{}, &object::member);
  function<int(int)> f_member_from_pair;
  f_member_from_pair = { object{}, &object::member };
  function<int(int)> f_functor(function<int(int)>(functor{}));
  function<int(int)> f_safe_ptr(std::make_shared<object>(), &object::member);
  function<int(int)> f_unsafe_ptr(new object, &object::member);

  assert(f_lambda(1) == 2);
  assert(f_ref_capturing_lambda(1) == 2);
  assert(f_copy_capturing_lambda(1) == 2);
  assert(f_free(1) == 2);
  assert(f_member(1) == 2);
  assert(f_member_from_pair(1) == 2);
  assert(f_functor(1) == 2);
  assert(f_safe_ptr(1) == 2);
  assert(f_unsafe_ptr(1) == 2);

  // here we test the c++17 deduction guides, which allow us to not explicitly specify any template arguments

  function f_free_deduced = &free_function;
  function f_member_deduced(object{}, &object::member);
  function f_lambda_deduced([](int i) -> int { return i + 1; });

  assert(f_free_deduced(1) == 2);
  assert(f_member_deduced(1) == 2);
  assert(f_lambda_deduced(1) == 2);
}