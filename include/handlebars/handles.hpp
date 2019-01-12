#pragma once

#include <tuple>
#include <utility>
#include <vector>

#include "dispatcher.hpp"

namespace handlebars {

// handles is a crtp style class which turns the derived class into a global event handles
// DerivedT must be the same type as the class which inherits this class
template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
struct handles
{
  // see dispatcher.hpp
  using handler_id_type = typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type;

  // performs dispatcher<SignalT,HandlerArgTs...>::connect_member(...) on a member function of the derived class
  template<typename MemPtrT>
  handler_id_type connect(const SignalT& signal, MemPtrT handler);

  // performs dispatcher<SignalT,HandlerArgTs...>::connect_bind_member(...) on a member function of the derived class
  template<typename MemPtrT, typename... BoundArgTs>
  handler_id_type connect_bind(const SignalT& signal, MemPtrT handler, BoundArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdHandlerArgTs>
  void push_event(const SignalT& signal, FwdHandlerArgTs&&... args);

  // calls global dispatcher respond function
  size_t respond(size_t limit = 0);

  // destructor, removes handlers that correspond to this class instance from global dispatcher
  ~handles();

private:
  std::vector<handler_id_type> m_handlers;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename MemPtrT>
typename handles<DerivedT, SignalT, HandlerArgTs...>::handler_id_type
handles<DerivedT, SignalT, HandlerArgTs...>::connect(const SignalT& signal, MemPtrT handler)
{
  m_handlers.push_back(
    dispatcher<SignalT, HandlerArgTs...>::connect_member(signal, static_cast<DerivedT*>(this), handler));
  return m_handlers.back();
}

template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename MemPtrT, typename... BoundArgTs>
typename handles<DerivedT, SignalT, HandlerArgTs...>::handler_id_type
handles<DerivedT, SignalT, HandlerArgTs...>::connect_bind(const SignalT& signal,
                                                          MemPtrT handler,
                                                          BoundArgTs&&... bound_args)
{
  m_handlers.push_back(dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(
    signal, static_cast<DerivedT*>(this), handler, std::forward<BoundArgTs>(bound_args)...));
  return m_handlers.back();
}

template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename... FwdHandlerArgTs>
void
handles<DerivedT, SignalT, HandlerArgTs...>::push_event(const SignalT& signal, FwdHandlerArgTs&&... args)
{
  dispatcher<SignalT, HandlerArgTs...>::push_event(signal, std::forward<FwdHandlerArgTs>(args)...);
}

template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
size_t
handles<DerivedT, SignalT, HandlerArgTs...>::respond(size_t limit)
{
  return dispatcher<SignalT, HandlerArgTs...>::respond(limit);
}

template<typename DerivedT, typename SignalT, typename... HandlerArgTs>
handles<DerivedT, SignalT, HandlerArgTs...>::~handles()
{
  for (auto& handle : m_handlers) {
    dispatcher<SignalT, HandlerArgTs...>::disconnect(handle);
  }
}
}
