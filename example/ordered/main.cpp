#include <handlebars/dispatcher.hpp>

#include <algorithm>
#include <iostream>
#include <random>

int
main()
{
  using D = handlebars::dispatcher<int>;
  std::random_device rd;
  std::uniform_int_distribution<int> dist(1, 5);
  auto push = [&](size_t i) {
    for (size_t j = 0; j < i; ++j) {
      auto v = dist(rd);
      D::push_event(v);
      std::cout << v << "\n";
    }
  };
  std::cout << "not sorted:\n";
  push(10);
  D::connect(1, [] { std::cout << "1\n"; });
  D::connect(2, [] { std::cout << "2\n"; });
  D::connect(3, [] { std::cout << "3\n"; });
  D::connect(4, [] { std::cout << "4\n"; });
  D::connect(5, [] { std::cout << "5\n"; });
  D::update_events(
    [](auto q) { std::sort(q.begin(), q.end(), [](auto l, auto r) { return std::get<0>(l) > std::get<0>(r); }); });
  std::cout << "sorted:\n";
  D::respond();
  return 0;
}