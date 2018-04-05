#ifndef handler_HPP_GAURD
#define handler_HPP_GAURD

#include <events/dispatcher.hpp>
#include <utility>
#include <vector>

namespace events {

template<class CRTP, class SignalT, class... SlotArgTs>
struct handler
{
  typedef std::pair<SignalT, size_t> slot_handle_type;

  void dispatch(const SignalT& signal, void (CRTP::*slot)(SlotArgTs...));

  template<class... ArgTs>
  void dispatch_bind(const SignalT& signal, void (CRTP::*slot)(ArgTs..., SlotArgTs...), ArgTs... bound_args);

  void event(const SignalT& signal, SlotArgTs... args);

  void purge(const SignalT& signal);
  void purge_all();

  ~handler();

private:
  std::vector<slot_handle_type> used_slots_;
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utility.hpp"
#include <tuple>

namespace events {
template<class CRTP, class SignalT, class... SlotArgTs>
void
handler<CRTP, SignalT, SlotArgTs...>::dispatch(const SignalT& signal, void (CRTP::*slot)(SlotArgTs...))
{
  used_slots_.push_back(
    std::make_pair(signal, dispatcher<SignalT, SlotArgTs...>::dispatch(signal, reinterpret_cast<CRTP*>(this), slot)));
}

template<class CRTP, class SignalT, class... SlotArgTs>
template<class... ArgTs>
void
handler<CRTP, SignalT, SlotArgTs...>::dispatch_bind(const SignalT& signal,
                                                    void (CRTP::*slot)(ArgTs..., SlotArgTs...),
                                                    ArgTs... bound_args)
{
  used_slots_.push_back(
    std::make_pair(signal,
                   dispatcher<SignalT, SlotArgTs...>::dispatch_bind(
                     signal, reinterpret_cast<CRTP*>(this), slot, std::forward<ArgTs>(bound_args)...)));
}

template<class CRTP, class SignalT, class... SlotArgTs>
void
handler<CRTP, SignalT, SlotArgTs...>::event(const SignalT& signal, SlotArgTs... args)
{
  dispatcher<SignalT, SlotArgTs...>::event(signal, std::forward<SlotArgTs>(args)...);
}

// usefull for preventing consecutive duplicates
template<class CRTP, class SignalT, class... SlotArgTs>
void
handler<CRTP, SignalT, SlotArgTs...>::purge(const SignalT& signal)
{
  dispatcher<SignalT, SlotArgTs...>::purge(signal);
}

// this function purges events associated with this specific instance of handler<...> from the event
// queue, can be very usefull for delaying execution or preventing consecutive duplicates
template<class CRTP, class SignalT, class... SlotArgTs>
void
handler<CRTP, SignalT, SlotArgTs...>::purge_all()
{
  for (auto i = 0; i < used_slots_.size(); i++) {
    for (auto& s : used_slots_) {
      dispatcher<SignalT, SlotArgTs...>::purge(std::get<0>(s));
    }
  }
}

template<class CRTP, class SignalT, class... SlotArgTs>
handler<CRTP, SignalT, SlotArgTs...>::~handler()
{
  for (auto& handle : used_slots_) {
    dispatcher<SignalT, SlotArgTs...>::calloff(handle.first, handle.second);
  }
}
}

#endif