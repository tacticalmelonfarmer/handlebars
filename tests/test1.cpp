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
    variant<bool, int, const char*> var[100];
    int i = 0;
    std::cout << sizeof(var[0]) << std::endl;
    while(i+6 < 100)
    {
        var[i++] = true;
        var[i++] = 21;
        var[i++] = "hello";
        var[i++] = false;
        var[i++] = 42;
        var[i++] = "goodbye";
    }
    return 0;
}