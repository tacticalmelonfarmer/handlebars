#include <cmath>
#include <handlebars/shared_handler.hpp>
#include <iostream>

// signal type to handle events for
enum class signals
{
  signal
};

// class which handles various events through a global dispatcher and can be destroyed without corrupting the
// functionality
struct sturdy : public handlebars::shared_handler<sturdy, signals>
{
  void slot() { std::cout << "I am a sturdy event handler!\n"; }
  sturdy() { connect(signals::signal, &sturdy::slot); }
};

int
main()
{
  { // create the sturdy event handler in a nested scope
    sturdy handler{};
  } // handler is destroyed at this point, coming out of nested scope
  using handlebars::dispatcher;
  dispatcher<signals>::push_event(signals::signal);
  dispatcher<signals>::respond();
  return 0;
}
