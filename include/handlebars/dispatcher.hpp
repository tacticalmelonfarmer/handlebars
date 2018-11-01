#pragma once

#include <functional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace handlebars {

template<typename SignalT, typename... SlotArgTs>
struct dispatcher
{
  using signal_type = SignalT;
  using slot_type = std::function<void(SlotArgTs...)>;
  using slot_id_type = std::pair<SignalT, size_t>;
  using args_storage_type = std::tuple<SlotArgTs...>;
  using chain_type = std::vector<slot_type>;
  using map_type = std::unordered_map<SignalT, chain_type>;
  using event_type = std::tuple<signal_type, args_storage_type>;
  using event_queue_type = std::queue<event_type>;

  // the constructor is deleted because this is a singleton class
  dispatcher() = delete;

  // associates a SignalT signal with a callable entity slot
  static size_t connect(const SignalT& signal, const std::function<void(SlotArgTs...)>& slot);

  // associates a SignalT signal with a member function pointer slot of a class instance
  template<typename ClassT>
  static size_t connect_member(const SignalT& signal, ClassT* target, void (ClassT::*slot)(SlotArgTs...));

  // associates a SignalT signal with a member function pointer slot of a class instance, after binding arguments to it
  template<typename ClassT, typename... ArgTs>
  static size_t connect_bind_member(const SignalT& signal,
                                    ClassT* target,
                                    void (ClassT::*slot)(ArgTs..., SlotArgTs...),
                                    ArgTs&&... args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  static void push_event(const SignalT& signal, SlotArgTs&&... args);

  // executes events and pops them off of the event queue the amount can be specified by limit, if limit is 0
  // then all events are executed
  static bool respond(size_t limit = 0);

  // this overload lets you execute events only with a specific signal value
  static bool respond(SignalT filter, size_t limit = 0);

  // remove a handler function (slot), indicated by "index", from the chain which maps to "signal"
  static void disconnect(const SignalT& signal, size_t index);

  // remove all pending events of a specific type
  static void purge_events(const SignalT& signal);

private:
  static map_type m_chain_map;
  static event_queue_type m_event_queue;
};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::map_type dispatcher<SignalT, SlotArgTs...>::m_chain_map = {};

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
size_t
dispatcher<SignalT, SlotArgTs...>::connect(const SignalT& signal, const std::function<void(SlotArgTs...)>& slot)
{
  m_chain_map[signal].push_back(slot);
  return m_chain_map[signal].size() - 1;
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT>
size_t
dispatcher<SignalT, SlotArgTs...>::connect_member(const SignalT& signal,
                                                  ClassT* target,
                                                  void (ClassT::*slot)(SlotArgTs...))
{
  m_chain_map[signal].push_back(
    [target, slot](SlotArgTs&&... args) { (target->*slot)(std::forward<SlotArgTs>(args)...); });
  return m_chain_map[signal].size() - 1;
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename... ArgTs>
size_t
dispatcher<SignalT, SlotArgTs...>::connect_bind_member(const SignalT& signal,
                                                       ClassT* target,
                                                       void (ClassT::*slot)(ArgTs..., SlotArgTs...),
                                                       ArgTs&&... bound_args)
{
  m_chain_map[signal].push_back([target, slot, &bound_args...](SlotArgTs&&... args) {
    (target->*slot)(std::forward<ArgTs>(bound_args)..., std::forward<SlotArgTs>(args)...);
  });
  return m_chain_map[signal].size() - 1;
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::push_event(const SignalT& signal, SlotArgTs&&... args)
{
  m_event_queue.push(std::make_tuple(signal, std::forward_as_tuple(std::forward<SlotArgTs>(args)...)));
}

template<typename SignalT, typename... SlotArgTs>
bool
dispatcher<SignalT, SlotArgTs...>::respond(size_t limit)
{
  if (m_event_queue.size() == 0)
    return false;
  if (limit == 0)
    limit = m_event_queue.size();
  for (size_t i = 0; i < limit; i++) {
    auto& front = m_event_queue.front();
    auto& signal = std::get<0>(front);
    auto& args_tuple = std::get<1>(front);
    for (auto& slot : m_chain_map[signal]) // fire slot chain
    {
      std::apply(slot, args_tuple);
    }
    m_event_queue.pop();
  }
  return m_event_queue.size() > 0;
}

template<typename SignalT, typename... SlotArgTs>
bool
dispatcher<SignalT, SlotArgTs...>::respond(SignalT filter, size_t limit)
{
  if (m_event_queue.size() == 0)
    return false;
  if (limit == 0)
    limit = m_event_queue.size();
  size_t progress = 0;
  std::vector<decltype(m_event_queue.begin())> found;
  for (auto iterator = m_event_queue.begin(); iterator != m_event_queue.end(); iterator++) {
    auto& signal = std::get<0>(*iterator);
    auto& args_tuple = std::get<1>(*iterator);
    if (signal == filter) {
      for (auto& slot : m_chain_map[filter]) // fire slot chain
      {
        std::apply(slot, args_tuple);
      }
      found.push_back(iterator);
      ++progress;
    }
    if ((progress = limit))
      break;
  }
  for (auto& d : found) {
    m_event_queue.erase(d);
  }
  return m_event_queue.size() > 0;
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::disconnect(const SignalT& signal, size_t index)
{
  if (m_chain_map.count(signal) && m_chain_map[signal].size() > index)
    m_chain_map[signal].erase(m_chain_map[signal].begin() + index);
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
