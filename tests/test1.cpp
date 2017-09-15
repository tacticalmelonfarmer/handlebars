#include "dispatcher.hpp"
#include "typelist.hpp"
#include <cstdint>
#include "variant.hpp"
#include <string>
#include <iostream>
#include <vector>

enum class ops
{ reduce, parenthesis, exponent, multiply, divide, add, subtract, modulo };

struct tokenizer
{
    std::string raw;
    std::string::const_iterator position;
    tokenizer(std::string& Input) : raw(Input), position(raw.begin()) {}
    //std::string get() {}
};

struct calculator
{
    std::vector<std::vector<double>> data_nest;
    std::vector<std::vector<ops>> ops_nest;
    decltype(data_nest)::iterator data_stack;
    decltype(ops_nest)::iterator ops_stack;
    
    void reduce()
    {}
    void parenthesis()
    { data_nest.push_back({}); ops_nest.push_back({}); }
    void exponent(double left, double right)
    { data_stack->push_back(left); data_stack->push_back(right); }
    void multiply()
    {}
    void divide()
    {}
    void add()
    {}
    void subtract()
    {}
    void modulo()
    {}
    calculator() {}
};

int main()
{
    return 0;
}