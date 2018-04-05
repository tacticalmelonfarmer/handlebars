#pragma once

#include "dispatcher.hpp"
#include <tuple>
#include <utility>
#include <vector>

namespace handlebars {

// handler is a crtp style class which turns the derived class into a global event handler
// DerivedT must be the same type as the class which inherits this class
template<typename DerivedT, typename SignalT, typename... SlotArgTs>
struct handler
{
  // see dispatcher.hpp
  typedef typename dispatcher<SignalT, SlotArgTs...>::slot_id_type slot_id_type;

  // performs dispatcher<SignalT,SlotArgTs...>::connect_member(...) on a member function of the derived class
  auto connect(const SignalT& signal, void (DerivedT::*slot)(SlotArgTs...));

  // performs dispatcher<SignalT,SlotArgTs...>::connect_bind_member(...) on a member function of the derived class
  template<typename... ArgTs>
  auto connect_bind(const SignalT& signal, void (DerivedT::*slot)(ArgTs..., SlotArgTs...), ArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  void push_event(const SignalT& signal, SlotArgTs&&... args);

  // removes all events using signal from event queue, useful for preventing duplicates when pushing an
  // event
  void purge_events(const SignalT& signal);

  // destructor, removes slots that correspond to this class instance from global dispatcher
  ~handler();

private:
  std::vector<slot_id_type> m_slots;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
auto
handler<DerivedT, SignalT, SlotArgTs...>::connect(const SignalT& signal, void (DerivedT::*slot)(SlotArgTs...))
{
  m_slots.push_back(dispatcher<SignalT, SlotArgTs...>::connect_member(signal, static_cast<DerivedT*>(this), slot));
  return m_slots.back();
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
template<typename... ArgTs>
auto
handler<DerivedT, SignalT, SlotArgTs...>::connect_bind(const SignalT& signal,
                                                       void (DerivedT::*slot)(ArgTs..., SlotArgTs...),
                                                       ArgTs&&... bound_args)
{
  m_slots.push_back(dispatcher<SignalT, SlotArgTs...>::connect_bind_member(
    signal, static_cast<DerivedT*>(this), slot, std::forward<ArgTs>(bound_args)...));
  return m_slots.back();
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
void
handler<DerivedT, SignalT, SlotArgTs...>::push_event(const SignalT& signal, SlotArgTs&&... args)
{
  dispatcher<SignalT, SlotArgTs...>::push_event(signal, std::forward<SlotArgTs>(args)...);
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
  for (auto& handle : m_slots) {
    dispatcher<SignalT, SlotArgTs...>::disconnect(handle);
  }
}
}
