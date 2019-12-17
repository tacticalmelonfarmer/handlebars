#include <iostream>
#include <string>

#include "repl.h"

int
main()
{
  std::string input;
  repl repl_instance{};
  repl_instance(input);
  while (repl_instance.active()) {
    std::getline(std::cin, input);
    repl_instance(input);
  }
  return 0;
}