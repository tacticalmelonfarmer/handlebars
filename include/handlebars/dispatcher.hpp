#pragma once

#include <list>
#include <mutex>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "function.hpp"

namespace handlebars {

enum class event_transform_operation
{
  modified, // does nothing yet, besides indicating a modification was made without changing the layout
  erase,    // tells `transform_events` to erase the current element
  copy      // tells `transform_events` to save a copy of the current element
};

template<typename SignalT, typename... SlotArgTs>
struct dispatcher
{
  // signal differentiaties the type of event that is happening
  using signal_type = SignalT;
  // a slot is an event handler which can take arguments and has NO RETURN VALUE
  using slot_type = function<void(SlotArgTs...)>;
  // this tuple packs arguments that will be pack with a signal and passed to an event handler
  using args_storage_type = std::tuple<std::remove_reference_t<SlotArgTs>...>;
  // a slot list is a sequence of event handlers that will be called consecutively to handle an event
  // list is used here to prevent invalidating iterators when calling a handler destructor, which removes slots from a
  // slot list
  using slot_list_type = std::list<slot_type>;
  // slot id  is a signal and an iterator to a slot packed together to make slot removal easier when calling
  // "disconnect" globally or from handler base class
  using slot_id_type = std::tuple<SignalT, typename slot_list_type::iterator>;
  // slot map, simply maps signals to their corresponding slot lists
  using slot_map_type = std::unordered_map<SignalT, slot_list_type>;
  // events hold all relevant data used to call an event handler
  using event_type = std::tuple<signal_type, args_storage_type>;
  // event queue is a modify-able fifo queue that stores events
  using event_queue_type = std::deque<event_type>;

  // associates a SignalT signal with a callable entity
  template<typename SlotT>
  static slot_id_type connect(const SignalT& signal, SlotT&& slot);

  // associates a SignalT signal with a callable entity, after binding arguments to it
  template<typename SlotT, typename... BoundArgTs>
  static slot_id_type connect_bind(const SignalT& signal, SlotT&& slot, BoundArgTs&&... args);

  // associates a SignalT signal with a member function pointer of a class instance
  template<typename ClassT, typename SlotT>
  static slot_id_type connect_member(const SignalT& signal, ClassT* target, SlotT slot);

  // associates a SignalT signal with a member function pointer of a class instance, after binding arguments to it
  template<typename ClassT, typename SlotT, typename... BoundArgTs>
  static slot_id_type connect_bind_member(const SignalT& signal, ClassT* target, SlotT slot, BoundArgTs&&... args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdSlotArgTs>
  static void push_event(const SignalT& signal, FwdSlotArgTs&&... args);

  // executes events and pops them off of the event queue the amount can be specified by limit, if limit is 0
  // then all events are executed, returns number of events left on the queue
  static size_t respond(size_t limit = 0);

  //  this removes an event handler from a slot list
  static void disconnect(const slot_id_type& slot_id);

  // remove all pending events of a specific type
  static void purge_events(const SignalT& signal);

  // this function iterates over the event queue with a predicate and modifies, erases, or copies elements
  static event_queue_type transform_events(const function<event_transform_operation(event_type&)>& pred)
  {
    event_queue_type result;
    std::vector<size_t> to_be_erased;
    std::scoped_lock lock(m_event_mutex);
    for (size_t i = 0; i < m_event_queue.size(); ++i) {
      auto op = pred(m_event_queue[i]);
      if (op == event_transform_operation::erase)
        to_be_erased.push_back(i);
      else if (op == event_transform_operation::copy)
        result.push_back(m_event_queue[i]);
    }
    auto new_end = std::remove_if(m_event_queue.begin(), m_event_queue.end(), [&](auto) {
      static size_t index = 0;
      static size_t erase_index = 0;
      if (erase_index < to_be_erased.size())
        return (index++ == to_be_erased[erase_index++]);
      else
        return false;
    });
    m_event_queue.erase(new_end, m_event_queue.end());
    return result;
  }

private:
  // singleton signtature
  dispatcher() {}

  static slot_map_type m_slot_map;
  static event_queue_type m_event_queue;
  static std::mutex m_slot_mutex, m_event_mutex;
};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_map_type dispatcher<SignalT, SlotArgTs...>::m_slot_map = {};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::event_queue_type dispatcher<SignalT, SlotArgTs...>::m_event_queue = {};

template<typename SignalT, typename... SlotArgTs>
std::mutex dispatcher<SignalT, SlotArgTs...>::m_slot_mutex = {};

template<typename SignalT, typename... SlotArgTs>
std::mutex dispatcher<SignalT, SlotArgTs...>::m_event_mutex = {};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace handlebars {

template<typename SignalT, typename... SlotArgTs>
template<typename SlotT>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect(const SignalT& signal, SlotT&& slot)
{
  std::scoped_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(std::forward<SlotT>(slot));
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename SlotT, typename... BoundArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_bind(const SignalT& signal, SlotT&& slot, BoundArgTs&&... args)
{
  std::scoped_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(
    std::move([&, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](SlotArgTs&&... args) {
      std::apply(slot, std::tuple_cat(bound_tuple, std::make_tuple(std::forward<SlotArgTs>(args)...)));
    }));
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename SlotT>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_member(const SignalT& signal, ClassT* target, SlotT slot)
{
  std::scoped_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(target, slot);
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename SlotT, typename... BoundArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_bind_member(const SignalT& signal,
                                                       ClassT* target,
                                                       SlotT slot,
                                                       BoundArgTs&&... bound_args)
{
  std::scoped_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(
    [=, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](SlotArgTs&&... args) {
      std::apply(function(target, slot),
                 std::tuple_cat(bound_tuple, std::forward_as_tuple(std::forward<SlotArgTs>(args)...)));
    });
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename... FwdSlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::push_event(const SignalT& signal, FwdSlotArgTs&&... args)
{
  std::scoped_lock lock(m_event_mutex);
  m_event_queue.push_front(std::make_tuple(signal, std::make_tuple(std::forward<FwdSlotArgTs>(args)...)));
}

template<typename SignalT, typename... SlotArgTs>
size_t
dispatcher<SignalT, SlotArgTs...>::respond(size_t limit)
{
  std::scoped_lock lock(m_event_mutex, m_slot_mutex);
  if (limit == 0)
    limit = m_event_queue.size();
  size_t progress = 0;
  for (size_t i = m_event_queue.size() - 1; progress != limit; --i, ++progress) {
    for (auto& slot : m_slot_map[std::get<0>(m_event_queue[i])]) // fire slot chain
    {
      std::apply(slot, std::get<1>(m_event_queue[i]));
    }
  }
  m_event_queue.erase(m_event_queue.end() - progress, m_event_queue.end());
  return m_event_queue.size();
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::disconnect(const slot_id_type& slot_id)
{
  std::scoped_lock lock(m_slot_mutex);
  m_slot_map[std::get<0>(slot_id)].erase(std::get<1>(slot_id));
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::purge_events(const SignalT& signal)
{
  while (m_event_queue.size()) {
    if (std::get<0>(m_event_queue.front()) == signal)
      m_event_queue.pop();
  }
}
}
