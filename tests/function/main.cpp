#define CATCH_CONFIG_MAIN
#include "../catch.hpp"
#include <handlebars/function.hpp>

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

TEST_CASE("assigning callables handlebars::function<...>s", "[function/assign]")
{
  function<int(int)> f_lambda{};
  f_lambda = [](int i) -> int { return i + 1; };

  int j = 1;
  function<int(int)> f_ref_capturing_lambda{};
  f_ref_capturing_lambda = [&j](int i) -> int { return i + j; };

  int k = 1;
  function<int(int)> f_copy_capturing_lambda{};
  f_copy_capturing_lambda = [k](int i) -> int { return i + k; };

  function<int(int)> f_free{};
  f_free = &free_function;

  function<int(int)> f_member{};
  f_member = { object{}, &object::member };

  function<int(int)> f_safe_ptr{};
  f_safe_ptr = { std::make_shared<object>(), &object::member };

  function<int(int)> f_unsafe_ptr{};
  f_unsafe_ptr = { new object, &object::member };

  function<int(int)> f_functor{};
  f_functor = function<int(int)>(functor{});

  REQUIRE(f_lambda(1) == 2);
  REQUIRE(f_ref_capturing_lambda(1) == 2);
  REQUIRE(f_copy_capturing_lambda(1) == 2);
  REQUIRE(f_free(1) == 2);
  REQUIRE(f_member(1) == 2);
  REQUIRE(f_safe_ptr(1) == 2);
  REQUIRE(f_unsafe_ptr(1) == 2);
  REQUIRE(f_functor(1) == 2);
}

TEST_CASE("constructing handlebars::function<...>s", "[function/construct]")
{
  function<int(int)> f_lambda([](int i) -> int { return i + 1; });
  int j = 1;
  function<int(int)> f_ref_capturing_lambda([&j](int i) -> int { return i + j; });
  int k = 1;
  function<int(int)> f_copy_capturing_lambda([k](int i) -> int { return i + k; });
  function<int(int)> f_free(&free_function);
  function<int(int)> f_member(object{}, &object::member);
  function<int(int)> f_safe_ptr(std::make_shared<object>(), &object::member);
  function<int(int)> f_unsafe_ptr(new object, &object::member);
  function<int(int)> f_functor(function<int(int)>(functor{}));

  REQUIRE(f_lambda(1) == 2);
  REQUIRE(f_ref_capturing_lambda(1) == 2);
  REQUIRE(f_copy_capturing_lambda(1) == 2);
  REQUIRE(f_free(1) == 2);
  REQUIRE(f_member(1) == 2);
  REQUIRE(f_safe_ptr(1) == 2);
  REQUIRE(f_unsafe_ptr(1) == 2);
  REQUIRE(f_functor(1) == 2);

  // here we test the c++17 deduction guides, which allow us to not explicitly specify any template arguments

  function f_free_deduced(&free_function);
  function f_member_deduced(object{}, &object::member);
  function f_lambda_deduced([](int i) -> int { return i + 1; });

  REQUIRE(f_free_deduced(1) == 2);
  REQUIRE(f_member_deduced(1) == 2);
  REQUIRE(f_lambda_deduced(1) == 2);
}