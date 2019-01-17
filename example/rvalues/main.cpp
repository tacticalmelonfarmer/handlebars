#include <handlebars/dispatcher.hpp>
#include <iostream>

int
main()
{
  enum class sig
  {
    action
  };
  using d = handlebars::dispatcher<sig, int&&>;
  d::connect(sig::action, [](int&& v) { v += 42; });                // increments a temporary through rvalue reference
  d::connect(sig::action, [](int&& v) { std::cout << v << "\n"; }); // get's its own copy of original, prints 10
  d::push_event(sig::action, 10);
  d::respond();
}