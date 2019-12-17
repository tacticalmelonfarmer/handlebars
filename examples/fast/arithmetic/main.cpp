#include <handlebars/fast/handles.hpp>
#include <iostream>

// signal type to handle events for
enum class op
{
  add,
  subtract,
  multiply,
  divide,
		signal_count = 4,
		handler_bind_limit = 0,
		chain_limit = 64,
		event_limit = 64,
		job_limit = 64,
    thread_limit = 0, /* 0 --> single threaded */
};

// class which handles various events through a global dispatcher
struct arithmetic : public tmf::hb::fast::handles<4, arithmetic, op, double&, const double&>
{
  void add(double& a, const double& b)
  {
    std::cout << "add: " << a << " + " << b;
    a = a + b;
    std::cout << " = " << a << "\n\n";
  }
  void subtract(double& a, const double& b)
  {
    std::cout << "subtract: " << a << " - " << b;
    a = a - b;
    std::cout << " = " << a << "\n\n";
  }
  void multiply(double& a, const double& b)
  {
    std::cout << "multiply: " << a << " * " << b;
    a = a * b;
    std::cout << " = " << a << "\n\n";
  }
  void divide(double& a, const double& b)
  {
    std::cout << "divide: " << a << " / " << b;
    a = a / b;
    std::cout << " = " << a << "\n\n";
  }

  arithmetic()
  {
    connect(op::add, &arithmetic::add);
    connect(op::subtract, &arithmetic::subtract);
    connect(op::multiply, &arithmetic::multiply);
    connect(op::divide, &arithmetic::divide);
  }
};

int
main()
{
  using dispatcher = tmf::hb::fast::dispatcher<op, double&, const double&>;
  double a = 5.0;
  std::cout << "a = " << a << "\n";
  arithmetic handler;
  handler.push_event(op::add, a, 2.14);
  handler.push_event(op::subtract, a, 0.5);
  handler.push_event(op::multiply, a, 10.0);
  handler.push_event(op::divide, a, 2.0);
  dispatcher::respond();
  std::cout << "a = " << a << "\n" << std::endl;
  return 0;
}
