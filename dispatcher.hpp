#pragma once
#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <tuple>
#include <utility>
#include "mpark/variant.hpp"

namespace events
{

template <class SignalT, class ... SlotArgTs>
struct dispatcher
{
    typedef SignalT                                    signal_type;
    typedef std::function<void (SlotArgTs...)>         slot_type;
    typedef std::tuple<SlotArgTs...>                   args_storage_type;
    typedef std::vector<slot_type>                     chain_type;
    typedef std::map<SignalT,
                     chain_type>                       map_type;
    typedef std::tuple<signal_type,
                       args_storage_type>              message_type;
    typedef std::queue<message_type>                   queue_type;

    size_t dispatch(const SignalT& signal,
                    const std::function<void()>& slot);
    
    size_t dispatch(const SignalT& signal,
                    const slot_type& slot);
    
    template <class ClassT>
    size_t dispatch(const SignalT& signal,
                    ClassT* target,
                    void (ClassT::*slot)());

    template <class ClassT>
    size_t dispatch(const SignalT& signal,
                    ClassT* target,
                    void (ClassT::*slot)(SlotArgTs...));
    
    template <class ClassT>
    size_t dispatch(const SignalT& signal,
                    void (ClassT::*slot)(SlotArgTs...));

    void message(const SignalT& signal);

    void message(const SignalT& signal,
                 SlotArgTs... args);

    bool poll(size_t limit = 0);
    bool poll(SignalT filter, size_t limit = 0);

    void calloff(const SignalT& signal);
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

// static versions
#define DISPATCH(signal, function) dispatch(signal, & function)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "dispatcher.ipp"