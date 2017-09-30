#include "variant.hpp"
#include <chrono>
#include <iostream>

using namespace std::chrono;
using namespace utility;

int main()
{
    steady_clock::time_point start, end;
    variant<int, const char*, bool> var[100];

    size_t i = 0;
    while((i++)+3 < 100)
    {   
        start = steady_clock::now();
        var[i++] = 23;
        end = steady_clock::now();
        std::cout << "took " << duration_cast<nanoseconds>(end - start).count() << " nanoseconds\n";
        
        start = steady_clock::now();
        var[i++] = "hi";
        end = steady_clock::now();
        std::cout << "took " << duration_cast<nanoseconds>(end - start).count() << " nanoseconds\n";

        start = steady_clock::now();
        var[i++] = false;
        end = steady_clock::now();
        std::cout << "took " << duration_cast<nanoseconds>(end - start).count() << " nanoseconds\n";
    }
}
