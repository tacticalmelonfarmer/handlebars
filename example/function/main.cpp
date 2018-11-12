#include <assert.h>
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
  ~functor() { std::cout << "destructor called successfully" << std::endl; }
};

int
main()
{
  function<int(int)> f_lambda([](int i) -> int { return i + 1; });
  int j = 1;
  function<int(int)> f_capturing_lambda([&, j](int i) -> int { return i + j; });
  function<int(int)> f_free(&free_function);
  function<int(int)> f_member(object{}, &object::member);
  function<int(int)> f_member_from_pair;
  f_member_from_pair = { object{}, &object::member };
  function<int(int)> f_functor(function<int(int)>(functor{})); // should move construct from functor
  function<int(int)> f_remote(std::make_shared<object>(), &object::member);
  function<int(int)> f_rawptr(new object, &object::member);

  assert(f_lambda(1) == 2);
  assert(f_capturing_lambda(1) == 2);
  assert(f_free(1) == 2);
  assert(f_member(1) == 2);
  assert(f_member_from_pair(1) == 2);
  assert(f_functor(1) == 2);
  assert(f_remote(1) == 2);
  assert(f_rawptr(1) == 2);
}