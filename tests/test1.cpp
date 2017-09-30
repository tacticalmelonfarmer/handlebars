#include "dispatcher.hpp"
#include "variant.hpp"
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

using utility::variant;

enum class ops
{ reduce, parenthesis, exponent, multiply, divide, add, subtract, modulo };

typedef variant<char, double> token;

struct tokenizer
{
    std::string input;
    tokenizer(const std::string& Input) : input(Input) {}
    token get() {}
};

int main()
{
    return 0;
}