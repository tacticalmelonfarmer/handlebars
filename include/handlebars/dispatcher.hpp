#pragma once

#include <list>
#include <mutex>
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
special_ref(T &&)->special_ref<T>;

template<typename T>
struct arg_storage
{
  using type = T;
};
template<typename T>
struct arg_storage<T&&>
{
  using type = T&&;
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

template<typename SignalT, typename... SlotArgTs>
struct dispatcher
{
  // signal differentiaties the type of event that is happening
  // preferably use a type that is cheap to copy
  using signal_type = SignalT;
  // a slot is an event handler which can take arguments and has NO RETURN VALUE.
  // see "function.hpp"
  using slot_type = function<void(SlotArgTs...)>;
  // this tuple holds the data which will be passed to the slot
  using args_storage_type = std::tuple<arg_storage_t<SlotArgTs>...>;
  // a slot list is a sequence of slots that will be called consecutively to handle an event.
  // std::list is used here to prevent invalidating iterators when calling a handler<...> destructor, which removes
  // slots from a slot list
  using slot_list_type = std::list<slot_type>;
  // slot id  is a signal and an iterator to a slot packed together to make slot removal easier when calling
  // "disconnect" globally or from handler base class
  using slot_id_type = std::tuple<SignalT, typename slot_list_type::iterator>;
  // slot map, simply maps signals to their corresponding slot lists
  using slot_map_type = std::unordered_map<SignalT, slot_list_type>;
  // events hold all relevant data to call a slot list
  using event_type = std::tuple<signal_type, args_storage_type>;
  // event queue is a modify-able fifo queue that stores events
  using event_queue_type = std::deque<event_type>;

  // associates a SignalT signal with a callable entity (any lambda, free function, static member function
  // or function object)
  template<typename SlotT>
  static slot_id_type connect(const SignalT& signal, SlotT&& slot);

  // associates a SignalT signal with a callable entity, after binding arguments to it
  template<typename SlotT, typename... BoundArgTs>
  static slot_id_type connect_bind(const SignalT& signal, SlotT&& slot, BoundArgTs&&... bound_args);

  // associates a SignalT signal with a member function pointer of a class instance
  // ClassT must be either a raw pointer or shared_ptr
  template<typename ClassT, typename MemPtrT>
  static slot_id_type connect_member(const SignalT& signal, ClassT&& object, MemPtrT member);

  // associates a SignalT signal with a member function pointer of a class instance, after binding arguments to it
  // ClassT must be either a raw pointer or shared_ptr
  template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
  static slot_id_type connect_bind_member(const SignalT& signal,
                                          ClassT&& object,
                                          MemPtrT member,
                                          BoundArgTs&&... bound_args);

  // pushes a new event onto the queue with a signal value and arguments, if any
  template<typename... FwdSlotArgTs>
  static void push_event(const SignalT& signal, FwdSlotArgTs&&... args);

  // returns the size of the event queue
  static size_t events_pending();

  // handles events and pops them off of the event queue.
  // the amount can be specified by limit.
  // if limit is 0, then all events are executed.
  // returns number of events that were succesfully handled
  static size_t respond(size_t limit = 0);

  //  this removes an event handler from a slot list
  static void disconnect(const slot_id_type& slot_id);

  // this function iterates over the event queue with a predicate and modifies, erases, or copies elements
  static void update_events(const function<void(event_queue_type&)>& updater);

private:
  // singleton signtature
  dispatcher() {}

  static slot_map_type m_slot_map;
  static event_queue_type m_event_queue, m_busy_queue;
  static std::recursive_mutex m_slot_mutex, m_event_mutex, m_busy_mutex;
};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_map_type dispatcher<SignalT, SlotArgTs...>::m_slot_map = {};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::event_queue_type dispatcher<SignalT, SlotArgTs...>::m_event_queue = {};

template<typename SignalT, typename... SlotArgTs>
typename dispatcher<SignalT, SlotArgTs...>::event_queue_type dispatcher<SignalT, SlotArgTs...>::m_busy_queue = {};

template<typename SignalT, typename... SlotArgTs>
std::recursive_mutex dispatcher<SignalT, SlotArgTs...>::m_slot_mutex = {};

template<typename SignalT, typename... SlotArgTs>
std::recursive_mutex dispatcher<SignalT, SlotArgTs...>::m_event_mutex = {};

template<typename SignalT, typename... SlotArgTs>
std::recursive_mutex dispatcher<SignalT, SlotArgTs...>::m_busy_mutex = {};
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
  std::unique_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(std::forward<SlotT>(slot));
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename SlotT, typename... BoundArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_bind(const SignalT& signal, SlotT&& slot, BoundArgTs&&... bound_args)
{
  std::unique_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(
    std::move([&, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](SlotArgTs&&... args) {
      std::apply(slot, std::tuple_cat(bound_tuple, std::make_tuple(std::forward<SlotArgTs>(args)...)));
    }));
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename MemPtrT>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_member(const SignalT& signal, ClassT&& object, MemPtrT member)
{
  std::unique_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(std::forward<ClassT>(object), member);
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
typename dispatcher<SignalT, SlotArgTs...>::slot_id_type
dispatcher<SignalT, SlotArgTs...>::connect_bind_member(const SignalT& signal,
                                                       ClassT&& object,
                                                       MemPtrT member,
                                                       BoundArgTs&&... bound_args)
{
  std::unique_lock lock(m_slot_mutex);
  m_slot_map[signal].emplace_back(
    [=, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](SlotArgTs&&... args) {
      std::apply(function(std::forward<ClassT>(object), member),
                 std::tuple_cat(bound_tuple, std::forward_as_tuple(std::forward<SlotArgTs>(args)...)));
    });
  return std::make_tuple(signal, --m_slot_map[signal].end());
}

template<typename SignalT, typename... SlotArgTs>
template<typename... FwdSlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::push_event(const SignalT& signal, FwdSlotArgTs&&... args)
{
  std::unique_lock event_lock(m_event_mutex, std::defer_lock);
  std::unique_lock busy_lock(m_busy_mutex, std::defer_lock);
  if (event_lock.try_lock() == false) {
    busy_lock.lock();
    m_busy_queue.push_front(std::make_tuple(
      signal, std::forward_as_tuple(static_cast<arg_storage_t<SlotArgTs>>(std::forward<FwdSlotArgTs>(args))...)));
  } else {
    m_event_queue.push_front(std::make_tuple(
      signal, std::forward_as_tuple(static_cast<arg_storage_t<SlotArgTs>>(std::forward<FwdSlotArgTs>(args))...)));
  }
}

template<typename SignalT, typename... SlotArgTs>
size_t
dispatcher<SignalT, SlotArgTs...>::events_pending()
{
  std::unique_lock lock(m_event_mutex);
  return m_event_queue.size();
}

template<typename SignalT, typename... SlotArgTs>
size_t
dispatcher<SignalT, SlotArgTs...>::respond(size_t limit)
{
  using namespace std::chrono_literals;
  std::scoped_lock slot_lock(m_slot_mutex, m_event_mutex);
  size_t progress = 0;

  if (m_event_queue.size() == 0)
    return 0;
  if (limit == 0) { // respond to an unlimited amount of events
    for (size_t i = m_event_queue.size() - 1;; --i, ++progress) {
      {
        auto& current_event = m_event_queue[i];
        for (auto& slot : m_slot_map[std::get<0>(current_event)]) {
          std::apply(slot, std::get<1>(current_event));
        }
      }
      m_event_queue.pop_back();
      if (i == 0)
        break;
    }
  } else { // respond to a limited amount of events
    for (size_t i = m_event_queue.size() - 1; progress != limit; --i, ++progress) {
      {
        auto& current_event = m_event_queue[i];
        for (auto& slot : m_slot_map[std::get<0>(current_event)]) {
          std::apply(slot, std::get<1>(current_event));
        }
      }
      m_event_queue.pop_back();
      if (i == 0)
        break;
    }
  }
  // here we load any events that were pushed by another thread while responding
  std::unique_lock tmp_lock(m_busy_mutex);
  for (auto i = m_busy_queue.rbegin(); i != m_busy_queue.rend(); ++i) {
    auto&& e = *i; // forwarding here allows us to transfer references without them decaying into values
    m_event_queue.push_front(std::forward<decltype(e)>(e));
  }
  m_busy_queue.clear();
  return progress;
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::disconnect(const slot_id_type& slot_id)
{
  std::unique_lock lock(m_slot_mutex);
  m_slot_map[std::get<0>(slot_id)].erase(std::get<1>(slot_id));
}

template<typename SignalT, typename... SlotArgTs>
void
dispatcher<SignalT, SlotArgTs...>::update_events(
  const function<void(typename dispatcher<SignalT, SlotArgTs...>::event_queue_type&)>& updater)
{
  std::unique_lock lock(m_event_mutex);
  updater(m_event_queue);
}
}
