#include <handlebars/dispatcher.hpp>
#include <iostream>

int
main()
{
  enum class sig
  {
    action
  };
  using d = tmf::hb::dispatcher<sig, int&>;
  d::connect(sig::action, [](int& v) { v += 42; });                // increments a temporary through lvalue reference
  d::connect(sig::action, [](int& v) { std::cout << v << "\n"; }); // uses same value as previously called event handler, prints 52
  int i = 10;
  d::push_event(sig::action, i);
  d::respond();

  using dc = tmf::hb::dispatcher<sig, const int&>;
  dc::connect(sig::action, [](const int& v){ std::cout << v << "\n"; });
  dc::push_event(sig::action, 13); // using const& "extends the life" of temporaries, actually makes an internal copy if passed a temporary value
  dc::respond();
}