#include <handlebars/dispatcher.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;
using d = handlebars::dispatcher<int>;

bool is_running = true;

void
talkative_thread()
{
  static auto id = d::connect(42, [] { is_running = false; });
  d::push_event(42);
  std::cout << "i got a word in edgewise!\n";
}

void
responsive_thread()
{
  d::connect(13, [] {
    std::cout << "responding!\n";
    std::this_thread::sleep_for(50ms);
  });
  for (int i = 0; i < 100; ++i) {
    d::push_event(13);
  }
  while (is_running) {
    d::respond();
  }
}

int
main()
{
  std::thread responder(&responsive_thread);
  std::this_thread::sleep_for(500ms);
  std::thread talker(&talkative_thread);
  responder.join();
  talker.join();
}