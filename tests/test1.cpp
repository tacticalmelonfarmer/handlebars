#include "dispatcher.hpp"
#include <string>
#include <iostream>
#include <algorithm>

enum class signals
{ hello, busy, goodbye, hosted1, hosted2, static_hosted, modify };

struct slot_host
{
    std::string id;
    slot_host(const std::string& Id) : id(Id)
    {}
    void member1(std::string msg)
    { std::cout << "\"" << msg << "\"" << " from " << id << ".member\n"; }
    void member2(std::string& msg)
    { std::sort(msg.begin(), msg.end()); std::cout << "\"" << msg << "\"" << " from " << id << ".member\n"; }
};

struct static_host
{
    static void member(const std::string& msg)
    { std::cout << msg << " from static member function.\n"; }
};

void hello(std::string msg)
{ std::cout << "hello " << msg << "\n"; }

void busy(std::string msg)
{ std::cout << "not now " << msg << " i'm busy...\n"; }

void goodbye(std::string msg)
{ std::cout << "i'm leaving, " << msg << ", i'll see ya l8r.\n"; }

void modify(std::string& msg)
{ std::sort(msg.begin(), msg.end()); std::cout << msg << std::endl; }

int main()
{
    slot_host host1("host1");
    slot_host host2("host2");

    events::dispatcher<signals, std::string> disp1;
    disp1.dispatch(signals::hello, &hello);
    disp1.dispatch(signals::busy, &busy);
    disp1.dispatch(signals::goodbye, &goodbye);
    disp1.HOSTED_DISPATCH(signals::hosted1, host1, member1);
    disp1.dispatch(signals::static_hosted, &static_host::member);

    events::dispatcher<signals, std::string&> disp2;
    disp2.dispatch(signals::modify, &modify);
    disp2.HOSTED_DISPATCH(signals::hosted2, host2, member2);

    
    std::string cmd_in;
    std::cout << "Enter yer name bud: \n";
    std::cin >> cmd_in;

    disp1.message(signals::hello, cmd_in);
    disp1.message(signals::busy, cmd_in);
    disp1.message(signals::goodbye, cmd_in);
    disp1.message(signals::hosted1, cmd_in);
    disp1.message(signals::hosted2, cmd_in);
    disp1.message(signals::static_hosted, cmd_in);
    std::string ref_test = "hello";
    disp2.message(signals::modify, ref_test);
    
    disp1.poll();
    disp2.poll();
}