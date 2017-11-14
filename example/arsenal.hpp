#ifndef ARSENAL_HPP_GAURD
#define ARSENAL_HPP_GAURD

#include <thread>
#include <chrono>
#include <string>
#include <iostream>

struct bomb
{
    bomb(std::string const& Boom) : boom(Boom), _active(true) {}
    void trigger(){ std::cout << boom << std::endl; _active = false; }
    bool active(){ return _active; }
private:
    std::string boom;
    bool _active;
};

struct remote
{
    void attach(bomb* Bomb, int){b = Bomb;}
    void arm(bomb*, int Timeout)
    {
        if(b == nullptr)
        {
            std::cout << "attach to a bomb first" << std::endl;
            return;
        }
        std::thread timer([this, Timeout]{
            this->armed = true;
            std::cout << "we have armed a bomb for " << Timeout << " seconds." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(Timeout));
            if(this->armed)
                this->b->trigger();
        });
    }
    void disarm(bomb*, int){ armed = false; }
private:
    bomb* b=nullptr;
    bool armed;
};

#endif