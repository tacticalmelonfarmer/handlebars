#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "function.hpp"

namespace handlebars {

inline namespace detail {

template<typename T>
struct special_ref
{
  constexpr special_ref(T&& ref)
    : m_ref(std::move(ref))
  {}
  constexpr special_ref(const T& ref)
    : m_ref(&ref)
  {}

  constexpr operator const T&() const
  {
    if (std::holds_alternative<const T*>(m_ref)) {
      return *std::get<const T*>(m_ref);
    } else {
      return std::get<T>(m_ref);
    }
  }

private:
  const std::variant<T, const T*> m_ref;
};

template<typename T>
struct fake_rval
{
  constexpr fake_rval(T&& ref)
    : m_val(std::move(ref))
  {}

  constexpr operator T() const { return T{ m_val }; }

private:
  T m_val;
};

template<typename T>
struct arg_storage
{
  using type = T;
};
template<typename T>
struct arg_storage<T&&>
{
  using type = fake_rval<T>;
};
template<typename T>
struct arg_storage<const T&>
{
  using type = special_ref<T>;
};
template<typename T>
struct arg_storage<T&>
{
  using type = T&;
};
template<typename T>
using arg_storage_t = typename arg_storage<T>::type;
}

template<typename SignalT, typename... HandlerArgTs>
struct dispatcher
{
  // signal differentiaties the type of event that is happening
  // preferably use a type that is cheap to copy
  using signal_type = SignalT;
  // a handler is an event handler which can take arguments and has NO RETURN VALUE.
  // see "function.hpp"
  using handler_type = function<void(HandlerArgTs...)>;
  // this tuple holds the data which will be passed to the handler
  using args_storage_type = std::tuple<arg_storage_t<HandlerArgTs>...>;
  // a handler chain is a sequence of handlers that will be called consecutively to handle an event.
  using handler_chain_type = std::vector<std::optional<handler_type>>;
  // handler id  is a signal and an iterator to a handler packed together to make handler removal easier when calling
  // "disconnect" globally or from handler base class
  using handler_id_type = std::tuple<SignalT, size_t>;
  // handler map, simply maps signals to their corresponding handler chains
  using handler_map_type = std::unordered_map<SignalT, handler_chain_type>;
  // events hold all relevant data to call a handler list
  using event_type = std::tuple<signal_type, args_storage_type>;
  // event queue is a modify-able fifo queue that stores events
  using event_queue_type = std::deque<event_type>;

  // associates a SignalT signal with a callable entity (any lambda, free function, static member function
  // or function object)
  template<typename HandlerT>
  static handler_id_type connect(const SignalT& signal, HandlerT&& handler);

  // associates a SignalT signal with a callable entity, after binding arguments to it
  template<typename HandlerT, typename... BoundArgTs>
  static handler_id_type connect_bind(const SignalT& signal, HandlerT&& handler, BoundArgTs&&... bound_args);

  // associates a SignalT signal with a member function pointer of a class instance
  // ClassT must be either a raw pointer or shared_ptr
  template<typename ClassT, typename MemPtrT>
  static handler_id_type connect_member(const SignalT& signal, ClassT&& object, MemPtrT member);

  // associates a SignalT signal with a member function pointer of a class instance, after binding arguments to it
  // ClassT must be either a raw pointer or shared_ptr
  template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
  static handler_id_type connect_bind_member(const SignalT& signal,
                                             ClassT&& object,
                                             MemPtrT member,
                                             BoundArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdHandlerArgTs>
  static void push_event(const SignalT& signal, FwdHandlerArgTs&&... args);

  // returns the size of the event queue
  static size_t events_pending();

  // handles events and pops them off of the event queue.
  // the amount can be specified by limit.
  // if limit is 0, then all events are executed.
  // returns number of events that were succesfully handled
  static size_t respond(size_t limit = 0);

  //  this removes an event handler from a handler list
  static void disconnect(const handler_id_type& handler_id);

  // this function lets you modify the event queue in a thread aware manner
  static void update_events(const function<void(event_queue_type&)>& updater);

private:
  // singleton signtature
  dispatcher() {}

  inline static handler_map_type m_handler_map{};
  inline static std::unordered_map<SignalT, std::vector<size_t>> m_unused_handler_storage_indices;
  inline static event_queue_type m_event_queue{};

  inline static std::atomic<std::uint32_t> m_threads_modifying_handlers{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_pushing_events{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_responding_to_events{ 0 };
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace handlebars {

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect(const SignalT& signal, HandlerT&& handler)
{
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  handler_id_type handler_id;
  if (m_unused_handler_storage_indices[signal].size() > 0) {
    handler_id = std::make_tuple(signal, m_unused_handler_storage_indices[signal].back());
    m_unused_handler_storage_indices[signal].pop_back();
    m_handler_map[signal].emplace(
      m_handler_map[signal].begin() + std::get<1>(handler_id), std::in_place, std::forward<HandlerT>(handler));
  } else {
    handler_id = std::make_tuple(signal, m_handler_map[signal].size());
    m_handler_map[signal].emplace_back(std::in_place, std::forward<HandlerT>(handler));
  }
  m_threads_modifying_handlers -= 1;
  return handler_id;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT, typename... BoundArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_bind(const SignalT& signal,
                                                   HandlerT&& handler,
                                                   BoundArgTs&&... bound_args)
{
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  handler_id_type handler_id;
  if (m_unused_handler_storage_indices[signal].size() > 0) {
    handler_id = std::make_tuple(signal, m_unused_handler_storage_indices[signal].back());
    m_unused_handler_storage_indices[signal].pop_back();
    m_handler_map[signal].emplace(
      m_handler_map[signal].begin() + std::get<1>(handler_id),
      std::in_place,
      [&, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
        std::apply(handler, std::tuple_cat(bound_tuple, std::make_tuple(std::forward<HandlerArgTs>(args)...)));
      });
  } else {
    handler_id = std::make_tuple(signal, m_handler_map[signal].size());
    m_handler_map[signal].emplace_back(
      std::in_place,
      [&, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
        std::apply(handler, std::tuple_cat(bound_tuple, std::make_tuple(std::forward<HandlerArgTs>(args)...)));
      });
  }
  m_threads_modifying_handlers -= 1;
  return handler_id;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_member(const SignalT& signal, ClassT&& object, MemPtrT member)
{
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  handler_id_type handler_id;
  if (m_unused_handler_storage_indices[signal].size() > 0) {
    handler_id = std::make_tuple(signal, m_unused_handler_storage_indices[signal].back());
    m_unused_handler_storage_indices[signal].pop_back();
    m_handler_map[signal].emplace(
      m_handler_map[signal].begin() + std::get<1>(handler_id), std::in_place, std::forward<ClassT>(object), member);
  } else {
    handler_id = std::make_tuple(signal, m_handler_map[signal].size());
    m_handler_map[signal].emplace_back(std::in_place, std::forward<ClassT>(object), member);
  }
  m_threads_modifying_handlers -= 1;
  return handler_id;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(const SignalT& signal,
                                                          ClassT&& object,
                                                          MemPtrT member,
                                                          BoundArgTs&&... bound_args)
{
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  handler_id_type handler_id;
  if (m_unused_handler_storage_indices[signal].size() > 0) {
    handler_id = std::make_tuple(signal, m_unused_handler_storage_indices[signal].back());
    m_unused_handler_storage_indices[signal].pop_back();
    m_handler_map[signal].emplace(
      m_handler_map[signal].begin() + std::get<1>(handler_id),
      std::in_place,
      [=, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
        std::apply(function(std::forward<ClassT>(object), member),
                   std::tuple_cat(bound_tuple, std::forward_as_tuple(std::forward<HandlerArgTs>(args)...)));
      });
  } else {
    handler_id = std::make_tuple(signal, m_handler_map[signal].size());
    m_handler_map[signal].emplace_back(
      std::in_place,
      [=, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
        std::apply(function(std::forward<ClassT>(object), member),
                   std::tuple_cat(bound_tuple, std::forward_as_tuple(std::forward<HandlerArgTs>(args)...)));
      });
  }
  m_threads_modifying_handlers -= 1;
  return handler_id;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename... FwdHandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::push_event(const SignalT& signal, FwdHandlerArgTs&&... args)
{
  m_threads_pushing_events += 1;
  while (m_threads_pushing_events.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  m_event_queue.push_back(std::make_tuple(
    signal, std::forward_as_tuple(static_cast<arg_storage_t<HandlerArgTs>>(std::forward<FwdHandlerArgTs>(args))...)));
  m_threads_pushing_events -= 1;
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::events_pending()
{
  size_t qsize = m_event_queue.size();
  return qsize;
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::respond(size_t limit)
{
  m_threads_responding_to_events += 1;
  if (m_threads_responding_to_events.load(std::memory_order_relaxed) > 1) {
    return 0; // here we have a non-fatal error, we are already responding to events
  }

  size_t progress = 0;

  if (limit == 0) { // respond to an unlimited amount of events
    for (auto&& e : m_event_queue) {
      for (auto&& h : m_handler_map[std::get<0>(e)]) {
        if (h.has_value()) {
          std::apply(h.value(), std::get<1>(e));
        }
      }
    }
  } else { // respond to a limited amount of events
    for (auto&& e : m_event_queue) {
      for (auto&& h : m_handler_map[std::get<0>(e)]) {
        if (h.has_value()) {
          std::apply(h.value(), std::get<1>(e));
        }
        ++progress;
        if (progress == limit) {
          break;
        }
      }
    }
  }

  m_threads_responding_to_events -= 1;
  return progress;
}

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::disconnect(const handler_id_type& handler_id)
{
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  m_handler_map[std::get<0>(handler_id)][std::get<1>(handler_id)] = std::nullopt;
  m_unused_handler_storage_indices[std::get<0>(handler_id)].push_back(std::get<1>(handler_id));
  m_threads_modifying_handlers -= 1;
}

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::update_events(
  const function<void(typename dispatcher<SignalT, HandlerArgTs...>::event_queue_type&)>& updater)
{
  m_threads_pushing_events += 1;
  m_threads_responding_to_events += 1;
  while (m_threads_pushing_events.load(std::memory_order_relaxed) > 1 ||
         m_threads_responding_to_events.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  updater(m_event_queue);
  m_threads_pushing_events -= 1;
  m_threads_responding_to_events -= 1;
}
}
