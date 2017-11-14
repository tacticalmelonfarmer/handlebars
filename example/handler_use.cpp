#include <events/handler.hpp>
#include <iostream>
#include "connected.hpp"

using events::dispatcher;

int main()
{
    connected a("hello");
    connected b("howdy");
    connected c("hi");
    connected d("sup");
    dispatcher<signals>::event(signals::trigger);
    b.event(signals::trigger); // equiavalent to dispatch<signals>::event(signals::trigger)
    b.trigger();

    std::cin.get();
    dispatcher<signals>::poll();
}