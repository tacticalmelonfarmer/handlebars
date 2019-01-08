#pragma once

#include "dispatcher.hpp"
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace handlebars {

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
struct shared_handler : std::enable_shared_from_this<DerivedT>
{
  // see dispatcher.hpp
  using slot_id_type = typename dispatcher<SignalT, SlotArgTs...>::slot_id_type;

  shared_handler()
    : m_ptr(std::make_shared<DerivedT>(static_cast<DerivedT&>(*this)))
  {}

  // performs dispatcher<SignalT,SlotArgTs...>::connect_member(...) on a member function of the derived class
  template<typename MemPtrT>
  slot_id_type connect(const SignalT& signal, MemPtrT slot);

  // performs dispatcher<SignalT,SlotArgTs...>::connect_bind_member(...) on a member function of the derived class
  template<typename MemPtrT, typename... BoundArgTs>
  slot_id_type connect_bind(const SignalT& signal, MemPtrT slot, BoundArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdSlotArgTs>
  void push_event(const SignalT& signal, FwdSlotArgTs&&... args);

  // calls global dispatcher respond function
  size_t respond(size_t limit = 0);

  // NOTE: shared_handler, unlike handler, has a noop destructor. when this instance is destroyed
  // enable_shared_from_this's destructor keeps this object alive through shared_ptr until the global dispatcher is
  // destroyed.
private:
  std::shared_ptr<DerivedT> m_ptr;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
template<typename MemPtrT>
typename shared_handler<DerivedT, SignalT, SlotArgTs...>::slot_id_type
shared_handler<DerivedT, SignalT, SlotArgTs...>::connect(const SignalT& signal, MemPtrT slot)
{
  return dispatcher<SignalT, SlotArgTs...>::connect_member(
    signal, static_cast<DerivedT&>(*this).shared_from_this(), slot);
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
template<typename MemPtrT, typename... BoundArgTs>
typename shared_handler<DerivedT, SignalT, SlotArgTs...>::slot_id_type
shared_handler<DerivedT, SignalT, SlotArgTs...>::connect_bind(const SignalT& signal,
                                                              MemPtrT slot,
                                                              BoundArgTs&&... bound_args)
{
  return dispatcher<SignalT, SlotArgTs...>::connect_bind_member(
    signal, static_cast<DerivedT&>(*this).shared_from_this(), slot, std::forward<BoundArgTs>(bound_args)...);
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
template<typename... FwdSlotArgTs>
void
shared_handler<DerivedT, SignalT, SlotArgTs...>::push_event(const SignalT& signal, FwdSlotArgTs&&... args)
{
  dispatcher<SignalT, SlotArgTs...>::push_event(signal, std::forward<FwdSlotArgTs>(args)...);
}

template<typename DerivedT, typename SignalT, typename... SlotArgTs>
size_t
shared_handler<DerivedT, SignalT, SlotArgTs...>::respond(size_t limit)
{
  return dispatcher<SignalT, SlotArgTs...>::respond(limit);
}
}
