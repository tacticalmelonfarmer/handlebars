#include "utility.hpp"

template <class SignalT, class ... SlotArgTs>
size_t SignalDispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, const std::function<void()>& slot)
{
    if(map_.count(signal) == 0) map_[signal] = SignalDispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(getAddress(f) == getAddress(slot)) return 0;
    }
    map_[signal].push_back([slot](SlotArgTs... dummy){ slot(); });
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
size_t SignalDispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, const typename SignalDispatcher<SignalT, SlotArgTs...>::slot_type& slot)
{
    if(map_.count(signal) == 0) map_[signal] = SignalDispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(getAddress(f) == getAddress(slot)) return 0;
    }
    map_[signal].push_back(slot);
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
template <class ClassT>
size_t SignalDispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, ClassT* target, void (ClassT::*slot)())
{
    if(map_.count(signal) == 0) map_[signal] = typename SignalDispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(reinterpret_cast<size_t>(&slot) == getAddress(f)) return 0;
    }
    map_[signal].push_back([target, slot](SlotArgTs... dummy){ (target->*slot)(); });
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
template <class ClassT>
size_t SignalDispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, ClassT* target, void (ClassT::*slot)(SlotArgTs...))
{
    if(map_.count(signal) == 0) map_[signal] = typename SignalDispatcher<SignalT, SlotArgTs...>::chain_type();
    for(auto& f : map_[signal])
    {
        if(reinterpret_cast<size_t>(&slot) == getAddress(f)) return 0;
    }
    map_[signal].push_back([target, slot](SlotArgTs... args){ (target->*slot)(args...); });
    return map_[signal].size();
}

template <class SignalT, class ... SlotArgTs>
template <class ClassT>
size_t SignalDispatcher<SignalT, SlotArgTs...>::dispatch(const SignalT& signal, void (ClassT::*slot)(SlotArgTs...))
{
    if(map_.count(signal) == 0) map_[signal] = typename SignalDispatcher<SignalT, SlotArgTs...>::chain_type();
    bool exists = false;
    for(auto& f : map_[signal])
    {
        if(reinterpret_cast<size_t>(&slot) == getAddress(f)) exists = true;
    }
    if(!exists) map_[signal].push_back(slot);
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT, SlotArgTs...>::message(const SignalT& signal)
{
    message_queue_.push(std::make_tuple(signal, std::forward_as_tuple(SlotArgTs() ...)));
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT, SlotArgTs...>::message(const SignalT& signal, SlotArgTs... args)
{
    message_queue_.push(std::make_tuple(signal, std::forward_as_tuple(args...)));
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT, SlotArgTs...>::poll(size_t limit)
{
    for(auto& f : pre_)
        f();
    if(message_queue_.size() == 0) return;
    if(limit == 0) limit = message_queue_.size();
    for(auto i=0; i<limit; i++)
    {
        auto& front = message_queue_.front();
        auto& signal = std::get<0>(front);
        auto& args_tuple = std::get<1>(front);
        for(auto& slot : map_[signal]) // fire slot chain
        {
            apply_tuple(slot, args_tuple);
        }
        message_queue_.pop();
    }
    for(auto& f : post_)
        f();
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT,SlotArgTs...>::poll(SignalT filter, size_t limit)
{
    for(auto& f : pre_)
        f();
    if(message_queue_.size() == 0) return;
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
                apply_tuple(slot, args_tuple);
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
    for(auto& f : post_)
        f();
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT,SlotArgTs...>::calloff(const SignalT& signal)
{
    if(map_.count(signal))
        map_.erase(signal);
}

template <class SignalT, class ... SlotArgTs>
void SignalDispatcher<SignalT,SlotArgTs...>::calloff(const SignalT& signal, size_t index)
{
    if(map_.count(signal) && map_[signal].size() > index)
        map_[signal].erase(index);
}