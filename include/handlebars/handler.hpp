#pragma once

#include "dispatcher.hpp"
#include <tuple>
#include <utility>
#include <vector>

namespace handlebars {

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
struct handler
{
  typedef std::pair<SignalT, size_t> slot_handle_type;

  size_t connect_member(const SignalT& signal, void (DerivedT::*slot)(SlotArgTs...));

  template<typename... ArgTs>
  size_t connect_bind_member(const SignalT& signal,
                             void (DerivedT::*slot)(ArgTs..., SlotArgTs...),
                             ArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  void push_event(const SignalT& signal, SlotArgTs&&... args);

  // removes all events using signal from event queue, useful for preventing duplicates when pushing an
  // event
  void purge_events(const SignalT& signal);

  ~handler();

private:
  std::vector<slot_handle_type> m_used_slots;
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace handlebars {
template<typename DerivedT, typename SignalT, typename... SlotArgTs>
size_t
handler<DerivedT, SignalT, SlotArgTs...>::connect_member(const SignalT& signal, void (DerivedT::*slot)(SlotArgTs...))
{
  m_used_slots.push_back(std::make_pair(
    signal, dispatcher<SignalT, SlotArgTs...>::connect_member(signal, static_cast<DerivedT*>(this), slot)));
  return std::get<1>(m_used_slots.back());
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
template<typename... ArgTs>
size_t
handler<DerivedT, SignalT, SlotArgTs...>::connect_bind_member(const SignalT& signal,
                                                              void (DerivedT::*slot)(ArgTs..., SlotArgTs...),
                                                              ArgTs&&... bound_args)
{
  m_used_slots.push_back(
    std::make_pair(signal,
                   dispatcher<SignalT, SlotArgTs...>::connect_bind_member(
                     signal, static_cast<DerivedT*>(this), slot, std::forward<ArgTs>(bound_args)...)));
  return std::get<1>(m_used_slots.back());
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
void
handler<DerivedT, SignalT, SlotArgTs...>::push_event(const SignalT& signal, SlotArgTs&&... args)
{
  dispatcher<SignalT, SlotArgTs...>::event(signal, std::forward<SlotArgTs>(args)...);
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
void
handler<DerivedT, SignalT, SlotArgTs...>::purge_events(const SignalT& signal)
{
  dispatcher<SignalT, SlotArgTs...>::purge_events(signal);
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
handler<DerivedT, SignalT, SlotArgTs...>::~handler()
{
  for (auto& handle : m_used_slots) {
    dispatcher<SignalT, SlotArgTs...>::disconnect(handle.first, handle.second);
  }
}
}
