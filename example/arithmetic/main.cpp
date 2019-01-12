#include <cmath>
#include <handlebars/handles.hpp>
#include <iostream>

// signal type to handle events for
enum class op
{
  add,
  subtract,
  multiply,
  divide
};

// class which handles various events through a global dispatcher
struct arithmetic : public handlebars::handles<arithmetic, op, double&, const double&>
{
  void add(double& a, const double& b)
  {
    a = a + b;
    std::cout << "add\n";
  }
  void subtract(double& a, const double& b)
  {
    a = a - b;
    std::cout << "subtract\n";
  }
  void multiply(double& a, const double& b)
  {
    a = a * b;
    std::cout << "multiply\n";
  }
  void divide(double& a, const double& b)
  {
    a = a / b;
    std::cout << "divide\n";
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
  using dispatcher = handlebars::dispatcher<op, double&, const double&>;
  double a = 1.0, c = 0.0;
  arithmetic handler;
  handler.push_event(op::add, a, c);
  c = 1.0;
  handler.push_event(op::subtract, a, 0.5);
  handler.push_event(op::multiply, a, 10.0);
  handler.push_event(op::divide, a, 2.0);
  dispatcher::respond();
  std::cout << a; // 7.5
  return 0;
}
