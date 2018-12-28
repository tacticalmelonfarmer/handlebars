#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE 64
#include <handlebars/dispatcher.hpp>
#include <handlebars/handler.hpp>
#include <iostream>
#include <string>

// signal type to handle events for
enum class my_signals
{
  open,
  print,
  close
};

// class which implements various event handlers and connects them to a dispatcher
struct my_event_handler : public handlebars::handler<my_event_handler, my_signals, const std::string&>
{
  void open(const std::string& Name, const std::string& Msg)
  {
    std::cout << "Hello, " << Name << "!" << std::endl;
    std::cout << Msg << "\n";
  }
  void print(const std::string& Msg) { std::cout << Msg << "\n"; }
  void close(const std::string& Name, const std::string& Msg)
  {
    std::cout << "Goodbye " << Name << "."
              << "\n";
    std::cout << Msg << "\n";
  }

  my_event_handler(const std::string& Name)
    : bound_name(Name)
  {
    connect_bind(my_signals::open, &my_event_handler::open, bound_name);
    connect(my_signals::print, &my_event_handler::print);
    connect_bind(my_signals::close, &my_event_handler::close, bound_name);
  }

private:
  const std::string bound_name;
};

int
main()
{
  using dispatcher = handlebars::dispatcher<my_signals, const std::string&>;
  my_event_handler steve("Steve");
  my_event_handler hank("Hank");
  dispatcher::push_event(my_signals::open, "How are you?");
  dispatcher::push_event(my_signals::print, "hmm...");
  dispatcher::push_event(my_signals::close, "See you later.");
  dispatcher::respond();
  return 0;
}
