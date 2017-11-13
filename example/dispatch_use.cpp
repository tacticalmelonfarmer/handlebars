#include <events/dispatcher.hpp>
#include <string>
#include <iostream>
#include "arsenal.hpp"

using events::dispatcher;
enum signals { attach, arm, disarm };

int main()
{
    using d = dispatcher<signals, bomb*, int>;
    bomb tactical_nuke("{utter annihilation!}");
    remote ctrl;
    
    d::dispatch(signals::attach, &ctrl, &remote::attach);
    d::dispatch(signals::arm, &ctrl, &remote::arm);
    d::dispatch(signals::disarm, &ctrl, &remote::disarm);

    while(tactical_nuke.active())
    {
        d::poll();
        std::string cmd;
        std::getline(std::cin, cmd);
        if(cmd == "attach")
            d::event(attach, &tactical_nuke, 0);
        if(cmd == "arm")
            d::event(arm, nullptr, 5);
        if(cmd == "disarm")
            d::event(disarm, nullptr, 0);
    }
}