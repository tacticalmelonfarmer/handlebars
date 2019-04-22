#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <type_traits>

#include "../function.hpp"
#include "../dispatcher.hpp"
#include "static_queue.hpp"

namespace handlebars::fast {

template<typename E>
struct valid_signal_enum
{
  static constexpr auto signal_limit = static_cast<size_t>(E::signal_limit);
  static constexpr auto handler_chain_size = static_cast<size_t>(E::handler_chain_size);
  static constexpr auto event_queue_size = static_cast<size_t>(E::event_queue_size);
  using type = E;
};

template<typename SignalT, typename... HandlerArgTs>
struct dispatcher
{
  using signal_type = valid_signal_enum<SignalT>;

  using handler_type = function<void(HandlerArgTs...)>;
  
  using args_storage_type = std::tuple<arg_storage_t<HandlerArgTs>...>;
  
  using handler_chain_type = static_stack<std::optional<handler_type>, signal_type::handler_chain_size>;

  struct handler_id_type
  {
    SignalT signal;
    size_t index;
  };
  
  using handler_map_type = std::array<handler_chain_type, signal_type::signal_limit>;
  
  struct event_type
  {
    signal_type signal;
    args_storage_type args;
  };
  
  using event_queue_type = static_queue<event_type, signal_type::event_queue_size>;

  template<typename HandlerT>
  static handler_id_type connect(SignalT signal_id, HandlerT&& handler);

  template<typename HandlerT, typename... BoundArgTs>
  static handler_id_type connect_bind(SignalT signal_id, HandlerT&& handler, BoundArgTs&&... bound_args);

  template<typename ClassT, typename MemPtrT>
  static handler_id_type connect_member(SignalT signal_id, ClassT&& object, MemPtrT member);

  template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
  static handler_id_type connect_bind_member(SignalT signal_id,
                                             ClassT&& object,
                                             MemPtrT member,
                                             BoundArgTs&&... bound_args);

  template<typename... FwdHandlerArgTs>
  static bool push_event(SignalT signal_id, FwdHandlerArgTs&&... args);

  static size_t events_pending();

  static size_t respond(size_t limit = 0);

  static void disconnect(const handler_id_type& handler_id);

  static void update_events(const function<void(event_queue_type&)>& updater);

private:
  dispatcher() {}

  inline static handler_map_type m_handler_map{};
  inline static event_queue_type m_event_queue{};

  inline static std::array<static_stack<size_t, signal_type::handler_chain_size>, signal_type::signal_limit> m_unused_handler_indices;

  inline static std::atomic<std::uint32_t> m_threads_modifying_handlers{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_pushing_events{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_responding_to_events{ 0 };
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace handlebars::fast {

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect(SignalT signal_id, HandlerT&& handler)
{
  auto signal = static_cast<size_t>(signal_id);
  m_threads_modifying_handlers += 1;
  while (m_threads_modifying_handlers.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  handler_id_type handler_id;
  auto& chain = m_handler_map[signal];
  auto& unused_indices = m_unused_handler_indices[signal];
  if (unused_indices.size() > 0) {
    // previous handlers have been disconnected, leaving unintialized gaps in the storage
    handler_id = { signal, unused_indices[signal].top() };
    unused_indices[signal].pop();
    ::new (&chain[handler_id.index]) handler_type{ std::forward<HandlerT>(handler) };
  } else {
    // must be pushed onto the back of handler chain
    handler_id = { signal, chain.size() };
    chain.push(std::forward<HandlerT>(handler));
  }
  m_threads_modifying_handlers -= 1;
  return handler_id;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT, typename... BoundArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_bind(SignalT signal_id,
                                                   HandlerT&& handler,
                                                   BoundArgTs&&... bound_args)
{
  auto signal = static_cast<size_t>(signal_id);
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
dispatcher<SignalT, HandlerArgTs...>::connect_member(SignalT signal_id, ClassT&& object, MemPtrT member)
{
  auto signal = static_cast<size_t>(signal_id);
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
dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(SignalT signal_id,
                                                          ClassT&& object,
                                                          MemPtrT member,
                                                          BoundArgTs&&... bound_args)
{
  auto signal = static_cast<size_t>(signal_id);
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
bool
dispatcher<SignalT, HandlerArgTs...>::push_event(SignalT signal_id, FwdHandlerArgTs&&... args)
{
  auto signal = static_cast<size_t>(signal_id);
  m_threads_pushing_events += 1;
  while (m_threads_pushing_events.load(std::memory_order_relaxed) > 1) {
    // BLOCK EXECUTION
  }
  m_event_queue.emplace_back(event_type{
    signal, std::forward_as_tuple(static_cast<arg_storage_t<HandlerArgTs>>(std::forward<FwdHandlerArgTs>(args))...) });
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
      for (auto&& h : m_handler_map[e.signal]) {
        if (h.has_value()) {
          std::apply(h.value(), e.args);
        }
      }
    }
    m_event_queue.clear();
  } else { // respond to a limited amount of events
    for (auto&& e : m_event_queue) {
      for (auto&& h : m_handler_map[e.signal]) {
        if (h.has_value()) {
          std::apply(h.value(), e.args);
          m_event_queue.pop_front();
        }
        ++progress;
        if (progress == limit) {
          break;
        }
      }
    }
    /// auto eqbegin = m_event_queue.begin();
    /// m_event_queue.erase(eqbegin, eqbegin + progress);
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

  auto& chain = m_handler_map[handler_id.signal];
  auto& unused_indices = m_unused_handler_indices[handler_id.signal];
  if(handler_id.index == chain.size - 1)
  {
    // if handler was at end of chain, just pop it
    chain.pop();
  } else {
    if (unused_indices.size() + 1 >= chain.size())
    {
      // if the unused index storage is full, this implies that the handler chain is empty and we may reset the state of both
      chain.clear();
      unused_indices.clear();
    } else {
      // destroy handler stored in an optional
      chain[handler_id.index] = std::nullopt;
      // if handler was in the middle or beginning of chain, store its index to be reused
      unused_indices.push_back(handler_id.index);
    }
  }
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
