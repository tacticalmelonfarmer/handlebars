#ifndef handler_HPP_GAURD
#define handler_HPP_GAURD

#include <events/dispatcher.hpp>
#include <utility>
#include <vector>

namespace events
{

template <class CRTP, class SignalT, class ... SlotArgTs>
struct handler
{
    typedef std::pair<SignalT, size_t> slot_handle_type;

    void dispatch(const SignalT& signal, void (CRTP::*slot)(SlotArgTs...));
    void event(const SignalT& signal, SlotArgTs... args);

    ~handler();
private:
    std::vector<slot_handle_type> used_slots_;
};

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <events/utility.hpp>
#include <tuple>

namespace events
{
template <class CRTP, class SignalT, class ... SlotArgTs>
void handler<CRTP, SignalT, SlotArgTs...>::dispatch(const SignalT& signal, void (CRTP::*slot)(SlotArgTs...))
{
    used_slots_.push_back(
        std::make_pair(
            signal,
            dispatcher<SignalT, SlotArgTs...>::dispatch(signal, reinterpret_cast<CRTP*>(this), slot)));
}

template <class CRTP, class SignalT, class ... SlotArgTs>
void handler<CRTP, SignalT, SlotArgTs...>::event(const SignalT& signal, SlotArgTs... args)
{
    dispatcher<SignalT, SlotArgTs...>::event(signal, std::forward<SlotArgTs>(args) ...);
}

template <class CRTP, class SignalT, class ... SlotArgTs>
handler<CRTP, SignalT, SlotArgTs...>::~handler()
{
    for(auto& handle : used_slots_)
    {
        dispatcher<SignalT, SlotArgTs...>::calloff(handle.first, handle.second);
    }
}

}

#endif