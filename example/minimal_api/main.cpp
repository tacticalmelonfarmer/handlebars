#include <assert.h>
#include <cmath>
#include <handlebars/handler.hpp>
#include <iostream>

// signal type to handle events for
enum class op
{
  add,
  subtract,
  multiply,
  divide
};

// class which implements various event handlers and connects them to a dispatcher
struct arithmetic : public handlebars::handler<arithmetic, op, double&, const double&>
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
  using disp = handlebars::dispatcher<op, double&, const double&>;
  double a = 1.0;
  arithmetic handler;
  handler.push_event(op::add, a, 1.0);
  handler.push_event(op::subtract, a, 0.5);
  handler.push_event(op::multiply, a, 10.0);
  handler.push_event(op::divide, a, 2.0);
  disp::respond();
  std::cout << a; // 7.5
  return 0;
}
