#include <assert.h>
#include <handlebars/handler.hpp>

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
  void add(double& a, const double& b) { a = a + b; }
  void subtract(double& a, const double& b) { a = a - b; }
  void multiply(double& a, const double& b) { a = a * b; }
  void divide(double& a, const double& b) { a = a / b; }

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
  double a = 1.0;
  double b = 1.0;
  arithmetic handler;
  handler.push_event(op::add, a, b);
  handler.push_event(op::subtract, a, b);
  handler.push_event(op::multiply, a, b);
  handler.push_event(op::divide, a, b);
  assert((int)a == 1);
  return 0;
}
