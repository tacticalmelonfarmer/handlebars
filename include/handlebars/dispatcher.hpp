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
  // a handler list is a sequence of handlers that will be called consecutively to handle an event.
  // std::list is used here to prevent invalidating iterators when calling a handler<...> destructor, which removes
  // handlers from a handler list
  using handler_list_type = std::list<handler_type>;
  // handler id  is a signal and an iterator to a handler packed together to make handler removal easier when calling
  // "disconnect" globally or from handler base class
  using handler_id_type = std::tuple<SignalT, typename handler_list_type::iterator>;
  // handler map, simply maps signals to their corresponding handler lists
  using handler_map_type = std::unordered_map<SignalT, handler_list_type>;
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

  // this function iterates over the event queue with a predicate and modifies, erases, or copies elements
  static void update_events(const function<void(event_queue_type&)>& updater);

private:
  // singleton signtature
  dispatcher() {}

  static handler_map_type m_handler_map;
  static event_queue_type m_event_queue, m_busy_queue;
  static std::mutex m_handler_mutex;
  static std::recursive_mutex m_event_mutex, m_busy_mutex;
};

template<typename SignalT, typename... HandlerArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_map_type dispatcher<SignalT, HandlerArgTs...>::m_handler_map =
  {};

template<typename SignalT, typename... HandlerArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::event_queue_type dispatcher<SignalT, HandlerArgTs...>::m_event_queue =
  {};

template<typename SignalT, typename... HandlerArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::event_queue_type dispatcher<SignalT, HandlerArgTs...>::m_busy_queue = {};

template<typename SignalT, typename... HandlerArgTs>
std::mutex dispatcher<SignalT, HandlerArgTs...>::m_handler_mutex = {};

template<typename SignalT, typename... HandlerArgTs>
std::recursive_mutex dispatcher<SignalT, HandlerArgTs...>::m_event_mutex = {};

template<typename SignalT, typename... HandlerArgTs>
std::recursive_mutex dispatcher<SignalT, HandlerArgTs...>::m_busy_mutex = {};
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
  std::unique_lock lock(m_handler_mutex);
  m_handler_map[signal].emplace_back(std::forward<HandlerT>(handler));
  return std::make_tuple(signal, --m_handler_map[signal].end());
}

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT, typename... BoundArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_bind(const SignalT& signal,
                                                   HandlerT&& handler,
                                                   BoundArgTs&&... bound_args)
{
  std::unique_lock lock(m_handler_mutex);
  m_handler_map[signal].emplace_back(std::move(
    [&, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
      std::apply(handler, std::tuple_cat(bound_tuple, std::make_tuple(std::forward<HandlerArgTs>(args)...)));
    }));
  return std::make_tuple(signal, --m_handler_map[signal].end());
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_member(const SignalT& signal, ClassT&& object, MemPtrT member)
{
  std::unique_lock lock(m_handler_mutex);
  m_handler_map[signal].emplace_back(std::forward<ClassT>(object), member);
  return std::make_tuple(signal, --m_handler_map[signal].end());
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type
dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(const SignalT& signal,
                                                          ClassT&& object,
                                                          MemPtrT member,
                                                          BoundArgTs&&... bound_args)
{
  std::unique_lock lock(m_handler_mutex);
  m_handler_map[signal].emplace_back(
    [=, bound_tuple = std::forward_as_tuple(std::forward<BoundArgTs>(bound_args)...)](HandlerArgTs&&... args) {
      std::apply(function(std::forward<ClassT>(object), member),
                 std::tuple_cat(bound_tuple, std::forward_as_tuple(std::forward<HandlerArgTs>(args)...)));
    });
  return std::make_tuple(signal, --m_handler_map[signal].end());
}

template<typename SignalT, typename... HandlerArgTs>
template<typename... FwdHandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::push_event(const SignalT& signal, FwdHandlerArgTs&&... args)
{
  std::unique_lock event_lock(m_event_mutex, std::defer_lock);
  std::unique_lock busy_lock(m_busy_mutex, std::defer_lock);
  if (event_lock.try_lock() == false) {
    busy_lock.lock();
    m_busy_queue.push_front(std::make_tuple(
      signal, std::forward_as_tuple(static_cast<arg_storage_t<HandlerArgTs>>(std::forward<FwdHandlerArgTs>(args))...)));
  } else {
    m_event_queue.push_front(std::make_tuple(
      signal, std::forward_as_tuple(static_cast<arg_storage_t<HandlerArgTs>>(std::forward<FwdHandlerArgTs>(args))...)));
  }
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::events_pending()
{
  std::unique_lock lock(m_event_mutex);
  return m_event_queue.size();
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::respond(size_t limit)
{
  using namespace std::chrono_literals;
  std::scoped_lock handler_lock(m_handler_mutex, m_event_mutex);
  size_t progress = 0;

  if (m_event_queue.size() == 0)
    return 0;
  if (limit == 0) { // respond to an unlimited amount of events
    for (size_t i = m_event_queue.size() - 1;; --i, ++progress) {
      {
        auto& current_event = m_event_queue[i];
        for (auto& handler : m_handler_map[std::get<0>(current_event)]) {
          std::apply(handler, std::get<1>(current_event));
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
        for (auto& handler : m_handler_map[std::get<0>(current_event)]) {
          std::apply(handler, std::get<1>(current_event));
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

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::disconnect(const handler_id_type& handler_id)
{
  std::unique_lock lock(m_handler_mutex);
  m_handler_map[std::get<0>(handler_id)].erase(std::get<1>(handler_id));
}

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::update_events(
  const function<void(typename dispatcher<SignalT, HandlerArgTs...>::event_queue_type&)>& updater)
{
  std::unique_lock lock(m_event_mutex);
  updater(m_event_queue);
}
}
