#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <callable.hpp>

#include <handlebars/dispatcher.hpp>
#include <handlebars/fast/dispatcher.hpp>
#include <handlebars/fast/static_queue.hpp>
#include <handlebars/fast/static_stack.hpp>

namespace tmf::hb::fast {

/*
 * this class is a set of requirements on a strong enumeration {E}.
 * these requirements map a constant member to an enum label
 */
template<typename E>
struct valid_signal_enum
{
  // how many signal values can be handled?
  static constexpr auto signal_count = static_cast<size_t>(E::signal_count);
  // how much extra memory per handler, for storing bound arguments?
  static constexpr auto handler_bind_limit = static_cast<size_t>(E::handler_bind_limit);
  // how many active handlers can be assigned to a specific signal value?
  // requirements!
  static constexpr auto chain_limit = static_cast<size_t>(E::chain_limit);
  // how many events can be stored on the queue?
  static constexpr auto event_limit = static_cast<size_t>(E::event_limit);
  // how many unfinished jobs can be stored on the queue?
  static constexpr auto job_limit = static_cast<size_t>(E::job_limit);
  // how many threads can attempt to push events at the same time?
  static constexpr auto thread_limit = static_cast<size_t>(E::thread_limit);
  using type = E;
};

template<typename T0, typename... Ts>
struct max_size_of
{
  // extract the largest type, or the first type if all types are the same size
  using type = std::
    conditional_t</*if*/ ((sizeof(T0) >= sizeof(Ts)) && ...), /*then*/ T0, /*else*/ typename max_size_of<Ts...>::type>;
};

template<typename... CandidateTypes>
using max_size_of_t = typename max_size_of<CandidateTypes...>::type;

template<typename SignalT, typename... HandlerArgTs>
struct dispatcher
{
  // a thin wrapper around `SignalT` providing compile-time checked type information
  using signal_type = valid_signal_enum<SignalT>;

  static constexpr auto handler_capacity = tmf::default_callable_capacity + signal_type::handler_bind_limit;
  // stored callable used to handle events
  using handler_type = tmf::callable<void(HandlerArgTs...), handler_capacity>;

  // `tuple` of storage capable types, for each type in `HandlerArgTs...`
  using args_storage_type = std::tuple<arg_storage_t<HandlerArgTs>...>;

  // `static_stack` of `optional`s of `handler_type`
  using handler_chain_type = static_stack<std::optional<handler_type>, signal_type::chain_limit>;

  // used to refer to an active event handler, NOTE: YOU MUST RETAIN THIS TO REMOVE SPECIFIC HANDLERS
  struct handler_id_type
  {
    size_t signal_int;
    size_t index;
  };

  // `array` that maps signal values to `handler_chain`s
  using handler_map_type = std::array<handler_chain_type, signal_type::signal_count>;

  // an event consisting of a signal and some stored arguments that will be passed to event handlers
  struct event_type
  {
    SignalT signal;
    args_storage_type args;
  };

  // a fixed-size FIFO queue to store events
  using event_queue_type = static_queue<event_type, signal_type::event_limit>;

  static constexpr auto job_capacity = (handler_capacity >= sizeof(event_type) ? handler_capacity : sizeof(event_type));
  using job_type = tmf::callable<void(), job_capacity>;

  using job_queue_type = static_queue<job_type, signal_type::job_limit>;

  template<typename HandlerT>
  static std::future<handler_id_type> connect(const SignalT signal_id, HandlerT&& handler);

  template<typename HandlerT, typename... BoundArgTs>
  static std::future<handler_id_type> connect_bind(const SignalT signal_id,
                                                   HandlerT&& handler,
                                                   BoundArgTs&&... bound_args);

  template<typename ClassT, typename MemPtrT>
  static std::future<handler_id_type> connect_member(const SignalT signal_id, ClassT&& object, MemPtrT member);

  template<typename ClassT, typename MemPtrT, typename... BoundArgTs>
  static std::future<handler_id_type> connect_bind_member(const SignalT signal_id,
                                                          ClassT&& object,
                                                          MemPtrT member,
                                                          BoundArgTs&&... bound_args);

  template<typename... FwdHandlerArgTs>
  static std::future<bool> push_event(const SignalT signal_id, FwdHandlerArgTs&&... args);

  static size_t events_pending();

  static size_t respond(size_t count = 0);

  static void disconnect(const handler_id_type handler_id);

private:
  static void finish_jobs();

  dispatcher() {}

  inline static handler_map_type m_handler_map{};
  inline static event_queue_type m_event_queue{};

  using handler_storage_gaps_type = static_stack<size_t, signal_type::chain_limit>;

  inline static std::array<handler_storage_gaps_type, signal_type::signal_count> m_handler_map_storage_gaps{};

  inline static job_queue_type m_job_queue{};

  inline static std::atomic<std::uint32_t> m_threads_modifying_handlers{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_pushing_events{ 0 };
  inline static std::atomic<std::uint32_t> m_threads_responding_to_events{ 0 };
  inline static std::atomic<bool> m_finishing_jobs{ false };
};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////Implementation//////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace tmf::hb::fast {

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::finish_jobs()
{
  bool busy = m_finishing_jobs.exchange(true);
  if (busy == true) {
    return;
  }
  for (auto job = m_job_queue.begin();; ++job) {
    if (job == m_job_queue.end()) {
      break;
    }
    job->operator()();
  }
  m_job_queue.clear();
  m_finishing_jobs = false;
}

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT>
std::future<typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type>
dispatcher<SignalT, HandlerArgTs...>::connect(const SignalT signal_id, HandlerT&& handler)
{
  std::promise<handler_id_type> promise{};
  std::future<handler_id_type> future = promise.get_future();

  auto implementation = [](auto promise, auto signal_id, auto&& handler) mutable -> void {
    handler_id_type handler_id;
    auto signal_int = static_cast<size_t>(signal_id);
    auto& chain = m_handler_map[signal_int];
    auto& storage_gaps = m_handler_map_storage_gaps[signal_int];
    // check if storage gaps tracker is full
    if (storage_gaps.size() > signal_type::handler_chain_capacity) {
      // TODO: hard error!
    }
    if (storage_gaps.size() > 0) {
      // previous handlers have been disconnected, leaving unintialized gaps in the storage
      handler_id.signal_int = signal_int;
      handler_id.index = *(storage_gaps.top());
      storage_gaps.pop();
    } else {
      // must be pushed onto the back of handler chain
      handler_id.signal_int = signal_int;
      handler_id.index = chain.size();
    }
    chain.push(handler_type{ std::forward<HandlerT>(handler) });
    // fulfill promise to asynchronous value
    promise.set_value(handler_id);
  };

  size_t thread_index = m_threads_modifying_handlers.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() == signal_type::job_queue_capacity) {
      // TODO: hard error!
    }
    // push new asynchronous job
    m_job_queue.push(std::move([implementation, promise = std::move(promise), signal_id, &handler]() mutable {
      implementation(std::move(promise), signal_id, std::forward<HandlerT>(handler));
    }));
  } else { /* only thread on deck */
    implementation(std::move(promise), signal_id, std::forward<HandlerT>(handler));
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_modifying_handlers.fetch_sub(1) == 1) {
    finish_jobs();
  }
  // return asynchronous value
  return std::move(future);
}

template<typename SignalT, typename... HandlerArgTs>
template<typename HandlerT, typename... BoundArgTs>
std::future<typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type>
dispatcher<SignalT, HandlerArgTs...>::connect_bind(const SignalT signal_id,
                                                   HandlerT&& handler,
                                                   BoundArgTs&&... bound_args)
{
  std::promise<handler_id_type> promise{};
  std::future<handler_id_type> future = promise.get_future();

  auto implementation = [](auto promise, auto signal_id, auto&& handler, auto&&... bound_args) mutable -> void {
    handler_id_type handler_id;
    size_t signal_int = static_cast<size_t>(signal_id);
    auto& chain = m_handler_map[signal_int];
    auto& storage_gaps = m_handler_map_storage_gaps[signal_int];
    // check if storage gaps tracker is full
    if (storage_gaps.size() > signal_type::handler_chain_capacity) {
      // TODO: hard error!
    }
    auto binder = [&handler,
                   bound_tuple = std::make_tuple(arg_storage_t<decltype((bound_args))>{
                     std::forward<decltype(bound_args)>(bound_args) }...)](auto&&... args) mutable {
      std::apply(tmf::callable{ std::forward<decltype(handler)>(handler) },
                 std::tuple_cat(bound_tuple, args_storage_type{ std::forward<decltype(args)>(args)... }));
    };
    if (storage_gaps.size() > 0) {
      // previous handlers have been disconnected, leaving unintialized gaps in the storage
      auto& gaps = storage_gaps[signal_int];
      handler_id = { signal_int, gaps.top() };
      gaps.pop();
      ::new (&chain[handler_id.index]) handler_type{ std::move(binder) };
    } else {
      // must be pushed onto the back of handler chain
      handler_id = { signal_int, chain.size() };
      chain.push(std::move(binder));
    }
    promise.set_value(handler_id);
  };

  size_t thread_index = m_threads_modifying_handlers.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() == signal_type::job_queue_capacity) {
      // TODO: hard error!
    }
    // push new asynchronous job
    m_job_queue.push(std::move([implementation, promise = std::move(promise), signal_id, &handler]() mutable {
      implementation(std::move(promise), signal_id, std::forward<HandlerT>(handler));
    }));
  } else { /* only thread on deck */
    implementation(std::move(promise), signal_id, std::forward<HandlerT>(handler));
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_modifying_handlers.fetch_sub(1) == 1) {
    finish_jobs();
  }
  return std::move(future);
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT>
std::future<typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type>
dispatcher<SignalT, HandlerArgTs...>::connect_member(const SignalT signal_id, ClassT&& object, MemPtrT member)
{
  std::promise<handler_id_type> promise{};
  std::future<handler_id_type> future = promise.get_future();

  auto implementation = [](auto promise, auto signal_id, auto&& object, auto member) mutable -> void {
    handler_id_type handler_id;
    auto signal_int = static_cast<size_t>(signal_id);
    auto& chain = m_handler_map[signal_int];
    auto& storage_gaps = m_handler_map_storage_gaps[signal_int];
    // check if storage gaps tracker is full
    if (storage_gaps.size() > signal_type::chain_limit) {
      // TODO: hard error!
    }
    if (storage_gaps.size() > 0) {
      // previous handlers have been disconnected, leaving unintialized gaps in the storage
      handler_id.signal_int = signal_int;
      handler_id.index = *(storage_gaps.top());
      storage_gaps.pop();
    } else {
      // must be pushed onto the back of handler chain
      handler_id.signal_int = signal_int;
      handler_id.index = chain.size();
    }
    chain.push(handler_type{ std::forward<ClassT>(object), member });
    // fulfill promise to asynchronous value
    promise.set_value(handler_id);
  };

  size_t thread_index = m_threads_modifying_handlers.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() == signal_type::job_limit) {
      // TODO: hard error!
    }
    // push new asynchronous job
    m_job_queue.push(std::move([implementation, promise = std::move(promise), signal_id, &object, member]() mutable {
      implementation(std::move(promise), signal_id, std::forward<ClassT>(object), member);
    }));
  } else { /* only thread on deck */
    implementation(std::move(promise), signal_id, std::forward<ClassT>(object), member);
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_modifying_handlers.fetch_sub(1) == 1) {
    finish_jobs();
  }
  // return asynchronous value
  return std::move(future);
}

template<typename SignalT, typename... HandlerArgTs>
template<typename ClassT, typename MemPtrT, typename... FwdBoundArgTs>
std::future<typename dispatcher<SignalT, HandlerArgTs...>::handler_id_type>
dispatcher<SignalT, HandlerArgTs...>::connect_bind_member(const SignalT signal_id,
                                                          ClassT&& object,
                                                          MemPtrT member,
                                                          FwdBoundArgTs&&... bound_args)
{
  std::promise<handler_id_type> promise{};
  std::future<handler_id_type> future = promise.get_future();

  auto implementation =
    [](auto promise, auto signal_id, auto&& object, auto member, auto&&... bound_args) mutable -> void {
    handler_id_type handler_id;
    size_t signal_int = static_cast<size_t>(signal_id);
    auto& chain = m_handler_map[signal_int];
    auto& storage_gaps = m_handler_map_storage_gaps[signal_int];
    // check if storage gaps tracker is full
    if (storage_gaps.size() > signal_type::handler_chain_capacity) {
      // TODO: hard error!
    }
    auto binder = [&object,
                   member,
                   bound_tuple = std::make_tuple(arg_storage_t<decltype((bound_args))>{
                     std::forward<decltype(bound_args)>(bound_args) }...)](auto&&... args) mutable {
      std::apply(tmf::callable{ std::forward<decltype(object)>(object), member },
                 std::tuple_cat(bound_tuple, args_storage_type{ std::forward<decltype(args)>(args)... }));
    };
    if (storage_gaps.size() > 0) {
      // previous handlers have been disconnected, leaving unintialized gaps in the storage
      auto& gaps = storage_gaps[signal_int];
      handler_id = { signal_int, gaps.top() };
      gaps.pop();
      ::new (&chain[handler_id.index]) handler_type{ std::move(binder) };
    } else {
      // must be pushed onto the back of handler chain
      handler_id = { signal_int, chain.size() };
      chain.push(std::move(binder));
    }
    promise.set_value(handler_id);
  };

  size_t thread_index = m_threads_modifying_handlers.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() == signal_type::job_queue_capacity) {
      // TODO: hard error!
    }
    // push new asynchronous job
    m_job_queue.push(std::move([implementation, promise = std::move(promise), signal_id, &object, member]() mutable {
      implementation(std::move(promise), signal_id, std::forward<ClassT>(object), member);
    }));
  } else { /* only thread on deck */
    implementation(std::move(promise), signal_id, std::forward<ClassT>(object), member);
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_modifying_handlers.fetch_sub(1) == 1) {
    finish_jobs();
  }
  return std::move(future);
}

template<typename SignalT, typename... HandlerArgTs>
template<typename... FwdHandlerArgTs>
std::future<bool>
dispatcher<SignalT, HandlerArgTs...>::push_event(const SignalT signal_id, FwdHandlerArgTs&&... args)
{
  std::promise<bool> promise{};
  std::future<bool> future = promise.get_future();

  auto implementation = [](auto promise, auto signal_id, auto&& args_tuple) mutable -> void {
    promise.set_value(m_event_queue.push(event_type{ signal_id, std::forward<decltype(args_tuple)>(args_tuple) }));
  };

  size_t thread_index = m_threads_pushing_events.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() == signal_type::job_limit) {
      // TODO: hard error!
    }
    m_job_queue.push(std::move([implementation,
                                promise = std::move(promise),
                                signal_id,
                                args_tuple = args_storage_type{ std::forward<FwdHandlerArgTs>(args)... }]() mutable {
      implementation(std::move(promise), signal_id, std::move(args_tuple));
    }));
  } else { /* only thread on deck */
    implementation(std::move(promise), signal_id, args_storage_type{ std::forward<FwdHandlerArgTs>(args)... });
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_pushing_events.fetch_sub(1) == 1) {
    finish_jobs();
  }
  return std::move(future);
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::events_pending()
{
  return m_event_queue.size();
}

template<typename SignalT, typename... HandlerArgTs>
size_t
dispatcher<SignalT, HandlerArgTs...>::respond(size_t count)
{
  size_t thread_index = m_threads_responding_to_events.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
                          // TODO: hard error!
  } else {                /* only thread on deck */
    size_t progress = 0;
    if (count == 0) { // respond to an uncounted amount of events
      for (auto e = m_event_queue.begin();; ++e) {
        // in order to respond to events pushed concurrently, we explicitly check `.end()` every iteration
        if (e == m_event_queue.end()) {
          break;
        }
        for (auto h = m_handler_map[static_cast<size_t>(e->signal)].begin();; ++h) {
          // again, in order to respond to events pushed concurrently, we explicitly check `.end()` every iteration
          if (h == m_handler_map[static_cast<size_t>(e->signal)].end()) {
            break;
          }
          if (h->has_value()) {
            std::apply(h->value(), e->args);
          }
        }
      }
      m_event_queue.clear();
    } else { // respond to a counted amount of events
      for (auto e = m_event_queue.begin();; ++e) {
        // in order to respond to events pushed concurrently, we explicitly check `.end()` every iteration
        if (e == m_event_queue.end()) {
          break;
        }
        for (auto h = m_handler_map[static_cast<size_t>(e->signal)].begin();; ++h) {
          // again, in order to respond to events pushed concurrently, we explicitly check `.end()` every iteration
          if (e == m_event_queue.end()) {
            break;
          }
          if (h->has_value()) {
            std::apply(h->value(), e->args);
            m_event_queue.pop();
          }
          ++progress;
          if (progress == count) {
            break;
          }
        }
      }
    }
    // decrement thread count
    --m_threads_responding_to_events;
    return m_event_queue.size();
  }
  return 0;
}

template<typename SignalT, typename... HandlerArgTs>
void
dispatcher<SignalT, HandlerArgTs...>::disconnect(const handler_id_type handler_id)
{
  auto implementation = [](auto handler_id) mutable {
    auto& chain = m_handler_map[handler_id.signal_int];
    auto& storage_gaps = m_handler_map_storage_gaps[handler_id.signal_int];
    if (handler_id.index == chain.size() - 1) {
      // if handler was at end of chain, just pop it
      chain.pop();
    } else {
      if (storage_gaps.size() + 1 >= chain.size()) {
        // if the unused index storage is full, this implies that the handler chain is empty and we may reset the state
        // of both
        chain.clear();
        storage_gaps.clear();
      } else {
        // destroy handler stored in an optional
        *chain[handler_id.index] = std::nullopt;
        // if handler was in the middle or beginning of chain, store its index to be reused
        storage_gaps.push(handler_id.index);
      }
    }
  };

  size_t thread_index = m_threads_modifying_handlers.fetch_add(1);
  if (thread_index > 0) { /* multiple threads on deck */
    // check if job queue is full
    if (m_job_queue.size() > signal_type::thread_limit) {
      // TODO: hard error!
    }
    m_job_queue.push([implementation, handler_id]() mutable { implementation(handler_id); });
  } else { /* only thread on deck */
    implementation(handler_id);
  }
  // decrement thread count, and check if all threads are done so we can finish interrupted jobs
  if (m_threads_modifying_handlers.fetch_sub(1) == 1) {
    finish_jobs();
  }
}
}
