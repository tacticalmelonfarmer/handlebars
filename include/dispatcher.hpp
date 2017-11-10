#pragma once
#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <tuple>
#include <utility>

namespace events
{

template <class SignalT, class ... SlotArgTs>
struct dispatcher
{
    typedef SignalT                                    signal_type;
    typedef std::function<void (SlotArgTs...)>         slot_type;
    typedef std::tuple<SlotArgTs...>                   args_storage_type;
    typedef std::vector<slot_type>                     chain_type;
    typedef std::map<SignalT, chain_type>              map_type;
    typedef std::tuple<signal_type, args_storage_type> message_type;
    typedef std::queue<message_type>                   queue_type;

    // connect a signal to a function object
    size_t dispatch(const SignalT& signal, const slot_type& slot);

    // connect a signal to a dynamic/instance member function object
    template <class ClassT>
    size_t dispatch(const SignalT& signal, const ClassT* target, void (ClassT::*slot)(SlotArgTs...));

    // connect a signal to a static member function object
    template <class ClassT>
    size_t dispatch(const SignalT& signal, void (ClassT::*slot)(SlotArgTs...));

    // push a signal and associated arguments onto the message queue
    void message(const SignalT& signal, SlotArgTs... args);

    // pull #(limit) messages from the queue and handle them
    bool poll(size_t limit = 0);
    // poll only messages with the specified signal "filter" up to #(limit)
    bool poll(SignalT filter, size_t limit = 0);

    // remove all handler functions (slot chain) which map to "signal"
    void calloff(const SignalT& signal);
    // remove 1 handler function (slot), indicated by "index", from the chain which maps to "signal"
    void calloff(const SignalT& signal, size_t index);
private:
    map_type map_;
    queue_type message_queue_;
};

}

// DISPATCH macro makes it easier to dispatch a signal to a member of an instance of a class type
// syntactic sugar i guess.
// example: "EASY_DISPATCH(signal, class_instance, id_of_member)" instead of "dispatch(signal, &class_instance, &decltype(class_instance)::id_of_member)"
#define HOSTED_DISPATCH(signal, object, member) dispatch(signal, & object, & decltype(object)::member)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "utility.hpp"
#include <tuple>

namespace events
{

template <class SignalT, class ... SlotArgTs>
size_t dispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, const typename dispatcher<SignalT, SlotArgTs...>::slot_type& slot)
{
    if(map_.count(signal) == 0) map_[signal] = dispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(utility::get_address(f) == utility::get_address(slot)) return 0;
    }
    map_[signal].push_back(slot);
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
template <class ClassT>
size_t dispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, const ClassT* target, void (ClassT::*slot)(SlotArgTs...))
{
    if(map_.count(signal) == 0) map_[signal] = typename dispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(reinterpret_cast<size_t>(&slot) == utility::get_address(f)) return 0;
    }
    map_[signal].push_back([target, slot](SlotArgTs... args){ (target->*slot)(args...); });
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
template <class ClassT>
size_t dispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, void (ClassT::*slot)(SlotArgTs...))
{
    if(map_.count(signal) == 0) map_[signal] = typename dispatcher<SignalT, SlotArgTs...>::chain_type();
    bool exists = false;
    for(auto& f : map_[signal])
    {
        if(reinterpret_cast<size_t>(&slot) == utility::get_address(f)) exists = true;
    }
    if(!exists) map_[signal].push_back(slot);
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
void dispatcher<SignalT, SlotArgTs...>::message(const SignalT& signal, SlotArgTs... args)
{
    message_queue_.push(std::make_tuple(signal, std::forward_as_tuple(args...)));
}

template <class SignalT, class ... SlotArgTs>
bool dispatcher<SignalT, SlotArgTs...>::poll(size_t limit)
{
    if(message_queue_.size() == 0) return false;
    if(limit == 0) limit = message_queue_.size();
    for(size_t i=0; i<limit; i++)
    {
        auto& front = message_queue_.front();
        auto& signal = std::get<0>(front);
        auto& args_tuple = std::get<1>(front);
        for(auto& slot : map_[signal]) // fire slot chain
        {
            std::apply(slot, args_tuple);
        }
        message_queue_.pop();
    }
    return message_queue_.size() > 0;
}

template <class SignalT, class ... SlotArgTs>
bool dispatcher<SignalT,SlotArgTs...>::poll(SignalT filter, size_t limit)
{
    if(message_queue_.size() == 0) return false;
    if(limit == 0) limit = message_queue_.size();
    size_t progress = 0;
    std::vector<decltype(message_queue_.begin())> found;
    for(auto iterator = message_queue_.begin(); iterator != message_queue_.end(); iterator++)
    {
        auto& signal = std::get<0>(*iterator);
        auto& args_tuple = std::get<1>(*iterator);
        if(signal == filter)
        {
            for(auto& slot : map_[filter]) // fire slot chain
            {
                std::apply(slot, args_tuple);
            }
            found.push_back(iterator);
            ++progress;
        }
        if(progress = limit) break;
    }
    for(auto& d : found)
    {
        message_queue_.erase(d);
    }
    return message_queue_.size() > 0;
}

template <class SignalT, class ... SlotArgTs>
void dispatcher<SignalT,SlotArgTs...>::calloff(const SignalT& signal)
{
    if(map_.count(signal))
        map_.erase(signal);
}

template <class SignalT, class ... SlotArgTs>
void dispatcher<SignalT,SlotArgTs...>::calloff(const SignalT& signal, size_t index)
{
    if(map_.count(signal) && map_[signal].size() > index)
        map_[signal].erase(index);
}

}