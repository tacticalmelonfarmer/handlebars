#pragma once

#include <cstdint>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef HANDLEBARS_FUNCTION_COMMON_MAX_SIZE
#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE 64
#endif

#define HANDLEBARS_FUNCTION_ERROR                                                                                      \
  "`handlebars::function` cannot hold a callable this large, try adjusting macro "                                     \
  "`HANDLEBARS_FUNCTION_COMMON_MAX_SIZE` "                                                                             \
  "before including this file"

namespace handlebars {

enum class function_source
{
  free,
  member_external, // a member of a pointed to object
  member_internal  // a member of a held and owned object
};

enum class function_ownership
{
  owned,
  shared,
  undefined
};

struct function_info
{
  const std::uintptr_t object;
  const std::uintptr_t target;
  const function_source source;
  const function_ownership ownership;
};

inline namespace detail {

template<typename T>
struct forward_ref
{
  constexpr forward_ref(T&& ref)
    : m_ref(&ref)
  {}
  constexpr forward_ref(const T& ref)
    : m_ref(&ref)
  {}

  constexpr operator const T&() const { return *m_ref; }

private:
  const T* m_ref;
  ;
};

template<typename T>
struct in_place_forward
{
  using type = forward_ref<T>;
};

template<typename T>
struct in_place_forward<T&>
{
  using type = T&;
};

template<typename T>
using in_place_forward_t = typename in_place_forward<T>::type;

template<typename ReturnT, typename... ArgTs>
struct function_base
{
  virtual ~function_base() {}
  virtual ReturnT operator()(in_place_forward_t<ArgTs>...) = 0;
  virtual ReturnT operator()(in_place_forward_t<ArgTs>...) const = 0;
  virtual function_info get_info() = 0;
};

template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function : function_base<ReturnT, ArgTs...>
{
  template<typename FwdClassT>
  member_function(FwdClassT&& object, MemPtrT member)
    : m_object(std::forward<FwdClassT>(object))
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) override { return (m_object.*m_member)(args...); }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) const override { return (m_object.*m_member)(args...); }

  function_info get_info() override
  {
    return {
      reinterpret_cast<std::uintptr_t>(&m_object), 0, function_source::member_internal, function_ownership::owned
    };
  }

private:
  ClassT m_object;
  MemPtrT m_member;
};

template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function_smart_pointer : function_base<ReturnT, ArgTs...>
{
  member_function_smart_pointer(const std::shared_ptr<ClassT>& object, MemPtrT member)
    : m_object(object)
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) override { return (m_object.get()->*m_member)(args...); }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) const override { return (m_object.get()->*m_member)(args...); }
  function_info get_info() override
  {
    return {
      reinterpret_cast<std::uintptr_t>(m_object.get()), 0, function_source::member_external, function_ownership::shared
    };
  }

private:
  std::shared_ptr<ClassT> m_object;
  MemPtrT m_member;
};

template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function_raw_pointer : function_base<ReturnT, ArgTs...>
{
  member_function_raw_pointer(ClassT* object, MemPtrT member)
    : m_object(object)
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) override { return (m_object->*m_member)(args...); }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) const override { return (m_object->*m_member)(args...); }
  function_info get_info() override
  {
    return {
      reinterpret_cast<std::uintptr_t>(m_object), 0, function_source::member_external, function_ownership::undefined
    };
  }

private:
  ClassT* m_object;
  MemPtrT m_member;
};

template<typename ReturnT, typename... ArgTs>
struct free_function : function_base<ReturnT, ArgTs...>
{
  using function_ptr_t = ReturnT (*)(ArgTs...);
  free_function(function_ptr_t pointer)
    : m_function_ptr(pointer)
  {}
  ReturnT operator()(in_place_forward_t<ArgTs>... args) override { return (*m_function_ptr)(args...); }
  ReturnT operator()(in_place_forward_t<ArgTs>... args) const override { return (*m_function_ptr)(args...); }
  function_info get_info() override
  {
    return { reinterpret_cast<std::uintptr_t>(nullptr),
             reinterpret_cast<std::uintptr_t>(m_function_ptr),
             function_source::free,
             function_ownership::shared };
  }

private:
  function_ptr_t m_function_ptr;
};

namespace sfinae {

template<typename T, typename ReturnT, typename... ArgTs>
struct generic_call_operator
{
  using type0 = ReturnT (T::*)(ArgTs...);
  using type1 = ReturnT (T::*)(ArgTs...) const;
  using type2 = ReturnT (T::*)(ArgTs...) volatile;
  using type3 = ReturnT (T::*)(ArgTs...) const volatile;
  using type4 = ReturnT (T::*)(ArgTs...) &;
  using type5 = ReturnT (T::*)(ArgTs...) const&;
  using type6 = ReturnT (T::*)(ArgTs...) volatile&;
  using type7 = ReturnT (T::*)(ArgTs...) const volatile&;
  using type8 = ReturnT (T::*)(ArgTs...) &&;
  using type9 = ReturnT (T::*)(ArgTs...) const&&;
  using type10 = ReturnT (T::*)(ArgTs...) volatile&&;
  using type11 = ReturnT (T::*)(ArgTs...) const volatile&&;
  // nice wall of overloads eh?
  static constexpr type0 check(type0);
  static constexpr type1 check(type1);
  static constexpr type2 check(type2);
  static constexpr type3 check(type3);
  static constexpr type4 check(type4);
  static constexpr type5 check(type5);
  static constexpr type6 check(type6);
  static constexpr type7 check(type7);
  static constexpr type8 check(type8);
  static constexpr type9 check(type9);
  static constexpr type10 check(type10);
  static constexpr type11 check(type11);
  using type = decltype(check(&T::operator()));
};

// clang-format off
template<typename T>
decltype(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>) is_function_pointer;

template<typename T>
decltype(std::is_reference_v<T> && std::is_function_v<std::remove_reference_t<T>>) is_function_reference;
// clang-format on

// this initializes deduction_guide for lambdas or functors
template<typename T>
struct deduction_guide
{
  using type = typename deduction_guide<decltype(&T::operator())>::type;
};

// this guides deduction for free/static-member function pointers
template<typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (*)(ArgTs...)>
{
  using type = ReturnT(ArgTs...);
};

// this guides deduction for free/static-member function references
template<typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (&)(ArgTs...)>
{
  using type = ReturnT(ArgTs...);
};

// this guides deduction for functors or mutable lambdas
template<typename ClassT, typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (ClassT::*)(ArgTs...)>
{
  using type = ReturnT(ArgTs...);
};

// this guides deduction for lambdas or const functors
template<typename ClassT, typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (ClassT::*)(ArgTs...) const>
{
  using type = ReturnT(ArgTs...);
};
} // namespace sfinae
} // namespace detail

struct empty_function
{};

template<typename>
struct function;

template<typename ReturnT, typename... ArgTs>
struct function<ReturnT(ArgTs...)>
{
  using function_type = ReturnT(ArgTs...);

  // copies/moves an object and holds a pointer to non-static member function of the held object
  template<typename ClassT, typename MemPtrT>
  function(ClassT&& object, MemPtrT member);

  // copies/moves an object and points to it's call operator `ClassT::operator()`
  template<typename ClassT>
  function(ClassT&& object);

  // points to an object and holds a pointer to non-static member function of the held object
  template<typename ClassT, typename MemPtrT>
  function(ClassT* object, MemPtrT member);

  // points to an object and points to it's call operator `ClassT::operator()`
  template<typename ClassT>
  function(ClassT* object);

  // points to an object and holds a pointer to non-static member function of the held object
  template<typename ClassT, typename MemPtrT>
  function(std::shared_ptr<ClassT> object, MemPtrT member);

  // points to an object and points to it's call operator `ClassT::operator()`
  template<typename ClassT>
  function(std::shared_ptr<ClassT> object);

  // points to a callable using a pointer to function
  function(function_type* function_pointer);

  // points to a callable using a reference to function
  function(function_type& function_reference);

  // default initialize to be an empty function<...>
  function()
    : m_empty(true)
  {}

  // copy construct
  function(const function<ReturnT(ArgTs...)>& other);

  // move construct
  function(function<ReturnT(ArgTs...)>&& other) noexcept;

  // copy assignment
  function<ReturnT(ArgTs...)>& operator=(const function<ReturnT(ArgTs...)>& rhs);

  // move assignment
  function<ReturnT(ArgTs...)>& operator=(function<ReturnT(ArgTs...)>&& rhs) noexcept;

  // call the stored function
  template<typename... FwdArgTs>
  ReturnT operator()(FwdArgTs&&... arguments);

  template<typename... FwdArgTs>
  ReturnT operator()(FwdArgTs&&... arguments) const;

  function_info get_info() const { return access()->get_info(); }

  // self-destruct sequence
  ~function() { destroy(); }

private:
  function_base<ReturnT, ArgTs...>* access() { return reinterpret_cast<function_base<ReturnT, ArgTs...>*>(&m_storage); }
  const function_base<ReturnT, ArgTs...>* access() const
  {
    return reinterpret_cast<const function_base<ReturnT, ArgTs...>*>(&m_storage);
  }
  void destroy()
  {
    if (!m_empty)
      access()->~function_base<ReturnT, ArgTs...>();
  }
  bool m_empty;
  std::aligned_storage_t<HANDLEBARS_FUNCTION_COMMON_MAX_SIZE> m_storage;
};

template<typename u, typename T>
function(u, T)->function<typename sfinae::deduction_guide<T>::type>;

template<typename T>
function(T)->function<typename sfinae::deduction_guide<T>::type>;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Implemenetation
///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename ReturnT, typename... ArgTs>
template<typename ClassT, typename MemPtrT>
function<ReturnT(ArgTs...)>::function(ClassT&& object, MemPtrT member)
  : m_empty(false)
{
  static_assert(sizeof(member_function<ClassT, MemPtrT, ReturnT, ArgTs...>) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access()) member_function<ClassT, MemPtrT, ReturnT, ArgTs...>(std::forward<ClassT>(object), member);
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT>
function<ReturnT(ArgTs...)>::function(ClassT&& object)
  : m_empty(false)
{
  using call_operator_ptr_t = typename sfinae::generic_call_operator<ClassT, ReturnT, ArgTs...>::type;
  static_assert(sizeof(member_function<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access())
    member_function<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>(std::forward<ClassT>(object), &ClassT::operator());
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT, typename MemPtrT>
function<ReturnT(ArgTs...)>::function(ClassT* object, MemPtrT member)
  : m_empty(false)
{
  static_assert(sizeof(member_function_raw_pointer<ClassT, MemPtrT, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access()) member_function_raw_pointer<ClassT, MemPtrT, ReturnT, ArgTs...>(object, member);
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT>
function<ReturnT(ArgTs...)>::function(ClassT* object)
  : m_empty(false)
{
  using call_operator_ptr_t = typename sfinae::generic_call_operator<ClassT, ReturnT, ArgTs...>::type;
  static_assert(sizeof(member_function_raw_pointer<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access())
    member_function_raw_pointer<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>(object, &ClassT::operator());
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT, typename MemPtrT>
function<ReturnT(ArgTs...)>::function(std::shared_ptr<ClassT> object, MemPtrT member)
  : m_empty(false)
{
  static_assert(sizeof(member_function_smart_pointer<ClassT, MemPtrT, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access()) member_function_smart_pointer<ClassT, MemPtrT, ReturnT, ArgTs...>(object, member);
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT>
function<ReturnT(ArgTs...)>::function(std::shared_ptr<ClassT> object)
  : m_empty(false)
{
  using call_operator_ptr_t = typename sfinae::generic_call_operator<ClassT, ReturnT, ArgTs...>::type;

  static_assert(sizeof(member_function_smart_pointer<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access())
    member_function_smart_pointer<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>(object, &ClassT::operator());
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>::function(function_type* function_pointer)
  : m_empty(false)
{
  static_assert(sizeof(function_type*) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE, HANDLEBARS_FUNCTION_ERROR);
  new (access()) free_function<ReturnT, ArgTs...>(function_pointer);
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>::function(function_type& function_reference)
  : m_empty(false)
{
  static_assert(sizeof(function_type*) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE, HANDLEBARS_FUNCTION_ERROR);
  new (access()) free_function<ReturnT, ArgTs...>(&function_reference);
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>::function(const function<ReturnT(ArgTs...)>& other)
{
  if (!other.m_empty) {
    m_storage = other.m_storage;
    m_empty = false;
  }
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>::function(function<ReturnT(ArgTs...)>&& other) noexcept
{
  if (other.m_empty) {
    m_storage = std::move(other.m_storage);
    other.m_empty = true;
    m_empty = false;
  }
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>&
function<ReturnT(ArgTs...)>::operator=(const function<ReturnT(ArgTs...)>& rhs)
{
  destroy();
  if (rhs.m_empty) {
    m_empty = true;
    return *this;
  } else {
    m_storage = rhs.m_storage;
    m_empty = false;
    return *this;
  }
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>&
function<ReturnT(ArgTs...)>::operator=(function<ReturnT(ArgTs...)>&& rhs) noexcept
{
  destroy();
  if (rhs.m_empty) {
    m_empty = true;
    return *this;
  } else {
    m_storage = std::move(rhs.m_storage);
    rhs.m_empty = true;
    m_empty = false;
    return *this;
  }
}

template<typename ReturnT, typename... ArgTs>
template<typename... FwdArgTs>
ReturnT
function<ReturnT(ArgTs...)>::operator()(FwdArgTs&&... arguments)
{
  if (m_empty)
    throw empty_function{};
  else
    return access()->operator()(std::forward<FwdArgTs>(arguments)...);
}

template<typename ReturnT, typename... ArgTs>
template<typename... FwdArgTs>
ReturnT
function<ReturnT(ArgTs...)>::operator()(FwdArgTs&&... arguments) const
{
  if (m_empty)
    throw empty_function{};
  else
    return access()->operator()(std::forward<FwdArgTs>(arguments)...);
}
}
