#include "dispatcher.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

enum class sig
{ push, nest, leave, reduce };
enum class op
{ add, sub, mul, div, nop };

struct sig_handler
{
    std::vector<std::vector<int>> data{{0}};
    std::vector<op> ops;
    size_t depth = 0;
    int reduction;
    void push(int i, op o)
    {
        data[depth].push_back(i);ops.push_back(o);
    }
    void nest()
    {
        data.push_back({});++depth;
    }
    void leave()
    {
        reduce();
        data[--depth].push_back(reduction);
        data.pop_back();
    }
    void reduce()
    {
        auto j = data[depth].begin();
        reduction = *(j++);
        for(;j!=data[depth].end();j++)
        {
            if(ops.back() == op::add)
                reduction += *j;
            else if(ops.back() == op::sub)
                reduction -= *j;
            else if(ops.back() == op::mul)
                reduction *= *j;
            else if(ops.back() == op::div)
                reduction /= *j;
            else if(ops.back() == op::nop)
                --j;
            ops.pop_back();
        }
    }
};

int main()
{
    SignalDispatcher<sig, int, op> mgr;
    sig_handler handler;

    mgr.dispatch(sig::push, &handler, &sig_handler::push);
    mgr.dispatch(sig::nest, &handler, &sig_handler::nest);
    mgr.dispatch(sig::leave, &handler, &sig_handler::leave);
    mgr.dispatch(sig::reduce, &handler, &sig_handler::reduce);

    std::string input, tmp;
    std::cout << "minimal maybe working calculator:\n";
    std::getline(std::cin, input, '\n');
    input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
    for(auto c = input.begin(); c<=input.end(); c++)
    {
        if(c == input.end())
        {
            if(tmp.size()) mgr.message(sig::push, std::stod(tmp), op::nop);
            tmp.clear();
            mgr.message(sig::reduce);
            break;
        }
        if(std::isdigit(*c))
        {tmp+=*c;}
        else if(*c == '+')
        {mgr.message(sig::push, std::stod(tmp), op::add); tmp.clear();}
        else if(*c == '-')
        {mgr.message(sig::push, std::stod(tmp), op::sub); tmp.clear();}
        else if(*c == '*')
        {mgr.message(sig::push, std::stod(tmp), op::mul); tmp.clear();}
        else if(*c == '/')
        {mgr.message(sig::push, std::stod(tmp), op::div); tmp.clear();}
        else if(*c == '(')
        {mgr.message(sig::nest);}
        else if(*c == ')')
        {if(tmp.size()) mgr.message(sig::push, std::stod(tmp), op::nop); mgr.message(sig::leave); tmp.clear();}
    }
    mgr.poll();
    std::cout << handler.reduction << std::endl;
}
