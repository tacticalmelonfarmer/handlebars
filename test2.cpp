#include "dispatcher.hpp"
#include <iostream>
#include <vector>
#include <string>
#include "mpark/variant.hpp"

enum class sig
{ inc, dec, put, get };

struct entity
{
    typedef mpark::variant<int, int*> var_t;
    std::vector<int> data;
    std::vector<int>::iterator position;
    entity(const std::vector<int>& init):data(init),position(data.begin()){}
    void increment(){++position;}
    void decrement(){--position;}
    void put(var_t in){*position = mpark::get<int>(in);}
    void get(var_t out){*mpark::get<int*>(out) = *position;}
};

int main()
{
    SignalDispatcher<sig, entity::var_t> disp;
    entity e({0,1,2,3,4,5});
    disp.HOSTED_DISPATCH(sig::inc, e, increment);
    disp.HOSTED_DISPATCH(sig::dec, e, decrement);
    disp.HOSTED_DISPATCH(sig::put, e, put);
    disp.HOSTED_DISPATCH(sig::get, e, get);

    int d = 7;

    disp.message(sig::inc);
    disp.message(sig::put, d);
    disp.message(sig::inc);
    disp.message(sig::get, &d);
    disp.message(sig::inc);
    disp.poll();

    for(auto& i : e.data)
        std::cout << i << "\n";
    std::cout << d << "\n";
}
