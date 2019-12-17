#pragma once

#include <tuple>
#include <utility>

#include <handlebars/fast/dispatcher.hpp>
#include <handlebars/fast/static_queue.hpp>

namespace tmf::hb::fast {

// handles is a crtp style class which turns the derived class into a container for event handlers
// and exposes some convenience functions
// DerivedT must be the same type as the class which inherits this class
template<size_t Limit, typename DerivedT, typename SignalT, typename... HandlerArgTs>
struct handles
{
  // see dispatcher.hpp
  using handler_id_type = typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type;

protected:
  // performs dispatcher<SignalT,HandlerArgTs...>::connect_member(...) on a member function of the derived class
  template<typename MemPtrT>
  void connect(const SignalT signal, MemPtrT handler);

  // performs dispatcher<SignalT,HandlerArgTs...>::connect_bind_member(...) on a member function of the derived class
  template<typename MemPtrT, typename... BoundArgTs>
  void connect_bind(const SignalT signal, MemPtrT handler, BoundArgTs&&... bound_args);

public:
  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdHandlerArgTs>
  void push_event(const SignalT signal, FwdHandlerArgTs&&... args);

  // calls global dispatcher respond function
  size_t respond(size_t count = 0);

  // destructor, removes handlers that correspond to this class instance from global dispatcher
  virtual ~handles();

private:
  static_queue<handler_id_type, Limit> m_handlers;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<size_t Handlers, typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename MemPtrT>
void
handles<Handlers, DerivedT, SignalT, HandlerArgTs...>::connect(const SignalT signal, MemPtrT handler)
{
  m_handlers.push(
    dispatcher<SignalT, HandlerArgTs...>::connect_member(signal, static_cast<DerivedT*>(this), handler).get());
}

template<size_t Handlers, typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename MemPtrT, typename... BoundArgTs>
void
handles<Handlers, DerivedT, SignalT, HandlerArgTs...>::connect_bind(const SignalT signal,
                                                                    MemPtrT handler,
                                                                    BoundArgTs&&... bound_args)
{
  m_handlers.push(dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(
                    signal, static_cast<DerivedT*>(this), handler, std::forward<BoundArgTs>(bound_args)...)
                    .get());
}

template<size_t Handlers, typename DerivedT, typename SignalT, typename... HandlerArgTs>
template<typename... FwdHandlerArgTs>
void
handles<Handlers, DerivedT, SignalT, HandlerArgTs...>::push_event(const SignalT signal, FwdHandlerArgTs&&... args)
{
  dispatcher<SignalT, HandlerArgTs...>::push_event(signal, std::forward<FwdHandlerArgTs>(args)...);
}

template<size_t Handlers, typename DerivedT, typename SignalT, typename... HandlerArgTs>
size_t
handles<Handlers, DerivedT, SignalT, HandlerArgTs...>::respond(size_t count)
{
  return dispatcher<SignalT, HandlerArgTs...>::respond(count);
}

template<size_t Handlers, typename DerivedT, typename SignalT, typename... HandlerArgTs>
handles<Handlers, DerivedT, SignalT, HandlerArgTs...>::~handles()
{
  for (auto& handler : m_handlers) {
    dispatcher<SignalT, HandlerArgTs...>::disconnect(handler);
  }
  m_handlers.clear();
}
}
