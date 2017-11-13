#ifndef CONNECTED_HPP_GAURD
#define CONNECTED_HPP_GAURD

#include <events/handler.hpp>
#include <iostream>
#include <string>

using events::handler;

enum class signals { trigger };

struct connected : handler<connected, signals>
{
    void boom()
    {
        std::cout << message_ << std::endl;
    }
    void trigger()
    {
        event(signals::trigger);
    }
    connected(std::string const& message)
    {
        message_ = message;
        dispatch(signals::trigger, &connected::boom);
    }
    std::string message_;
};



#endif