#pragma once

#include <functional>
#include <list>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace handlebars {

template<typename SignalT, typename... SlotArgTs>
struct dispatcher
{
  // signal differentiaties the type of event that is happening
  using signal_type = SignalT;
  // a slot is an event handler which can take arguments and has NO RETURN VALUE
  using slot_type = std::function<void(SlotArgTs...)>;
  // this tuple packs arguments that will be pack with a signal and passed to an event handler
  using args_storage_type = std::tuple<SlotArgTs...>;
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

  // associates a SignalT signal with a callable entity slot
  static slot_id_type connect(const SignalT& signal, const slot_type& slot);

  // associates a SignalT signal with a member function pointer slot of a class instance
  template<typename ClassT>
  static slot_id_type connect_member(const SignalT& signal, ClassT* target, void (ClassT::*slot)(SlotArgTs...));

  // associates a SignalT signal with a member function pointer slot of a class instance, after binding arguments to it
  template<typename ClassT, typename... ArgTs>
  static slot_id_type connect_bind_member(const SignalT& signal,
                                          ClassT* target,
                                          void (ClassT::*slot)(ArgTs..., SlotArgTs...),
                                          ArgTs&&... args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  static void push_event(const SignalT& signal, SlotArgTs&&... args);

  // executes events and pops them off of the event queue the amount can be specified by limit, if limit is 0
  // then all events are executed
  static bool respond(size_t limit = 0);

  //  this removes an event handler from a slot list
  static void disconnect(const slot_id_type& slot_id);

  // remove all pending events of a specific type
  static void purge_events(const SignalT& signal);

private:
  // singleton signtature
  dispatcher() {}

  static slot_map_type m_slot_map;
  static event_queue_type m_event_queue;
};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_map_type dispatcher<SignalT, SlotArgTs...>::m_slot_map = {};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::event_queue_type dispatcher<SignalT, SlotArgTs...>::m_event_queue = {};
}

// DISPATCH macro makes it easier to dispatch a signal to a member of an instance of a typename type
// syntactic sugar i guess.
// example: "DISPATCH(signal, typename_instance, id_of_member)" instead of "dispatch(signal,
// &typename_instance, &decltype(typename_instance)::id_of_member)"
#define DISPATCH(signal, object, member) dispatch(signal, &object, &decltype(object)::member)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace handlebars {

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect(const SignalT& signal, const slot_type& slot)
{
  m_slot_map[signal].push_back(slot);
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_member(const SignalT& signal,
                                                  ClassT* target,
                                                  void (ClassT::*slot)(SlotArgTs...))
{
  m_slot_map[signal].push_back(
    [target, slot](SlotArgTs&&... args) { (target->*slot)(std::forward<SlotArgTs>(args)...); });
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename... ArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_bind_member(const SignalT& signal,
                                                       ClassT* target,
                                                       void (ClassT::*slot)(ArgTs..., SlotArgTs...),
                                                       ArgTs&&... bound_args)
{
  m_slot_map[signal].push_back([target, slot, &bound_args...](SlotArgTs&&... args) {
    (target->*slot)(std::forward<ArgTs>(bound_args)..., std::forward<SlotArgTs>(args)...);
  });
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::push_event(const SignalT& signal, SlotArgTs&&... args)
{
  m_event_queue.push_back(std::make_tuple(signal, std::forward_as_tuple(std::forward<SlotArgTs>(args)...)));
}

template<typename SignalT, typename... SlotArgTs>
bool
dispatcher<SignalT, SlotArgTs...>::respond(size_t limit)
{
  if (m_event_queue.size() == 0)
    return false;
  size_t progress = 0;
  for (auto& event : m_event_queue) {
    for (auto& slot : m_slot_map[std::get<0>(event)]) // fire slot chain
    {
      std::apply(slot, std::get<1>(event));
    }
    ++progress;
    m_event_queue.pop_front();
    if (progress == limit)
      break;
  }
  return m_event_queue.size() > 0;
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::disconnect(const slot_id_type& slot_id)
{

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
