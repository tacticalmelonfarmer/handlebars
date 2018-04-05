#ifndef CONNECTED_HPP_GAURD
#define CONNECTED_HPP_GAURD

#include <events/handler.hpp>
#include <iostream>
#include <string>

using events::handler;

enum class signals
{
  trigger
};

struct connected : handler<connected, signals>
{
  void boom() { std::cout << message_ << std::endl; }
  void trigger()
  {
    events::dispatcher<signals>::purge(signals::trigger); // prevent consecutive calls to same function
    event(signals::trigger);
  }
  void other_method(int i, float f, char c) { std::cout << "dispatch_bind success!" << i << f << c << std::endl; }
  connected(std::string const& message)
  {
    message_ = message;
    dispatch(signals::trigger, &connected::boom);
    dispatch_bind(signals::trigger, &connected::other_method, 42, 3.14f, '0');
  }
  std::string message_;
};

#endif