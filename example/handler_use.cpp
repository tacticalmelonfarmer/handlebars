#include <events/handler.hpp>
#include <iostream>
#include "connected.hpp"

using events::dispatcher;

int main()
{
    connected a("hello");
    connected b("howdy");
    connected c("hi");
    connected d("yo");
    b.event(signals::trigger);
    std::cin.get();
    dispatcher<signals>::poll();
}