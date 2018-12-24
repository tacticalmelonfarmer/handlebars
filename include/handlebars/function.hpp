#pragma once

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef HANDLEBARS_FUNCTION_COMMON_MAX_SIZE
#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE 48
#endif

#define HANDLEBARS_FUNCTION_ERROR                                                                                      \
  "`handlebars::function` cannot hold a callable this large, try adjusting macro "                                     \
  "`HANDLEBARS_FUNCTION_COMMON_MAX_SIZE` "                                                                             \
  "before including this file"

namespace handlebars {
inline namespace detail {
template<typename ReturnT, typename... ArgTs>
struct function_base
{
  virtual ~function_base() {}
  virtual ReturnT operator()(ArgTs&&...) = 0;
};

// a member function of an instance of a class COPIED INTO FUNCTION
template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function : function_base<ReturnT, ArgTs...>
{
  member_function(ClassT&& object, MemPtrT member)
    : m_object(std::forward<ClassT>(object))
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(ArgTs&&... args) override { return (m_object.*m_member)(std::forward<ArgTs>(args)...); }

private:
  ClassT m_object;
  MemPtrT m_member;
};

// a member function of an instance of a class SAFELY POINTED TO
template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function_reference : function_base<ReturnT, ArgTs...>
{
  member_function_reference(std::shared_ptr<ClassT> object, MemPtrT member)
    : m_object(object)
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(ArgTs&&... args) override { return (m_object.get()->*m_member)(std::forward<ArgTs>(args)...); }

private:
  std::shared_ptr<ClassT> m_object;
  MemPtrT m_member;
};

// a member function of an instance of a class -->UNSAFELY<-- POINTED TO
template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function_pointer : function_base<ReturnT, ArgTs...>
{
  member_function_pointer(ClassT* object, MemPtrT member)
    : m_object(object)
    , m_member(member)
  {
    static_assert(std::is_member_function_pointer_v<MemPtrT>,
                  "`MemPtrT member` must be a pointer to member function of `ClassT`");
  }
  ReturnT operator()(ArgTs&&... args) override { return (m_object->*m_member)(std::forward<ArgTs>(args)...); }

private:
  ClassT* m_object;
  MemPtrT m_member;
};

// any function with an address assignable to ReturnT(*)(ArgTs...)
template<typename ReturnT, typename... ArgTs>
struct free_function : function_base<ReturnT, ArgTs...>
{
  using function_ptr_t = ReturnT (*)(ArgTs...);
  free_function(function_ptr_t pointer)
    : m_function_ptr(pointer)
  {}
  ReturnT operator()(ArgTs&&... args) override { return (*m_function_ptr)(std::forward<ArgTs>(args)...); }

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
  static constexpr type0 check(type0) { return {}; }
  static constexpr type1 check(type1) { return {}; }
  static constexpr type2 check(type2) { return {}; }
  static constexpr type3 check(type3) { return {}; }
  static constexpr type4 check(type4) { return {}; }
  static constexpr type5 check(type5) { return {}; }
  static constexpr type6 check(type6) { return {}; }
  static constexpr type7 check(type7) { return {}; }
  static constexpr type8 check(type8) { return {}; }
  static constexpr type9 check(type9) { return {}; }
  static constexpr type10 check(type10) { return {}; }
  static constexpr type11 check(type11) { return {}; }
  using type = decltype(check(&T::operator()));
};

// clang-format off
template<typename T>
decltype(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>) is_function_pointer;

template<typename T>
decltype(std::is_reference_v<T> && std::is_function_v<std::remove_reference_t<T>>) is_function_reference;
// clang-format on

template<typename T>
struct deduction_guide
{
  using type = typename deduction_guide<decltype(&T::operator())>::type;
};

template<typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (*)(ArgTs...)>
{
  using type = ReturnT(ArgTs...);
};

template<typename ClassT, typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (ClassT::*)(ArgTs...)>
{
  using type = ReturnT(ArgTs...);
};

template<typename ClassT, typename ReturnT, typename... ArgTs>
struct deduction_guide<ReturnT (ClassT::*)(ArgTs...) const>
{
  using type = ReturnT(ArgTs...);
};
}
}

struct empty_function
{};

template<typename>
struct function;

template<typename ReturnT, typename... ArgTs>
struct function<ReturnT(ArgTs...)>
{
  using function_pointer_type = ReturnT (*)(ArgTs...);

  // copies an object and holds a pointer to member function of the copy
  template<typename ClassT, typename MemPtrT>
  function(ClassT&& object, MemPtrT member);

  // copies an object and uses its call operator with matching signature
  template<typename ClassT>
  function(ClassT&& object);

  // holds a pointer to an object and a pointer to its member function
  template<typename ClassT, typename MemPtrT>
  function(std::shared_ptr<ClassT> object, MemPtrT member);

  // holds pointer to an object and uses its call operator with matching signature
  template<typename ClassT>
  function(std::shared_ptr<ClassT> object);

  // holds a free function or static member function pointer
  function(function_pointer_type function_pointer);

  // default initialize to be an empty function<...>
  function()
    : m_empty(true)
  {}

  // copy from another function<...>
  function(const function<ReturnT(ArgTs...)>& other)
  {
    if (!other.m_empty) {
      m_storage = other.m_storage;
      m_empty = false;
    }
  }

  // move from another function<...>
  function(function<ReturnT(ArgTs...)>&& other) noexcept
  {
    if (other.m_empty) {
      m_storage = std::move(other.m_storage);
      other.m_empty = true;
      m_empty = false;
    }
  }

  // copy assignment
  function<ReturnT(ArgTs...)>& operator=(const function<ReturnT(ArgTs...)>& rhs)
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

  // move assignment
  function<ReturnT(ArgTs...)>& operator=(function<ReturnT(ArgTs...)>&& rhs) noexcept
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

  // call the stored function
  ReturnT operator()(ArgTs&&... arguments)
  {
    if (m_empty)
      throw empty_function{};
    else
      return access()->operator()(std::forward<ArgTs>(arguments)...);
  }

  // self-destruct sequence
  ~function() { destroy(); }

private:
  function_base<ReturnT, ArgTs...>* access() { return reinterpret_cast<function_base<ReturnT, ArgTs...>*>(&m_storage); }
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
  if constexpr (std::is_pointer_v<ClassT>) { // this branch is unsafe, use std::shared_ptr<...>
    using nonptr_class_t = std::remove_pointer_t<ClassT>;
    static_assert(sizeof(member_function_reference<nonptr_class_t, MemPtrT, ReturnT, ArgTs...>) <=
                    HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                  HANDLEBARS_FUNCTION_ERROR);
    new (access())
      member_function_pointer<nonptr_class_t, MemPtrT, ReturnT, ArgTs...>(std::forward<ClassT>(object), member);
  } else {
    static_assert(sizeof(member_function<ClassT, MemPtrT, ReturnT, ArgTs...>) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                  HANDLEBARS_FUNCTION_ERROR);
    new (access()) member_function<ClassT, MemPtrT, ReturnT, ArgTs...>(std::forward<ClassT>(object), member);
  }
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT>
function<ReturnT(ArgTs...)>::function(ClassT&& object)
  : m_empty(false)
{
  if constexpr (std::is_pointer_v<ClassT>) { // this branch is unsafe, use std::shared_ptr<...>
    using nonptr_class_t = std::remove_pointer_t<ClassT>;
    using call_operator_ptr_t = typename sfinae::generic_call_operator<nonptr_class_t, ReturnT, ArgTs...>::type;
    static_assert(sizeof(member_function_reference<nonptr_class_t, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                    HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                  HANDLEBARS_FUNCTION_ERROR);
    new (access()) member_function_pointer<nonptr_class_t, call_operator_ptr_t, ReturnT, ArgTs...>(
      std::forward<ClassT>(object), &nonptr_class_t::operator());
  } else {
    using call_operator_ptr_t = typename sfinae::generic_call_operator<ClassT, ReturnT, ArgTs...>::type;
    static_assert(sizeof(member_function<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                    HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                  HANDLEBARS_FUNCTION_ERROR);
    new (access()) member_function<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>(std::forward<ClassT>(object),
                                                                                   &ClassT::operator());
  }
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT, typename MemPtrT>
function<ReturnT(ArgTs...)>::function(std::shared_ptr<ClassT> object, MemPtrT member)
  : m_empty(false)
{
  static_assert(sizeof(member_function_reference<ClassT, MemPtrT, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access()) member_function_reference<ClassT, MemPtrT, ReturnT, ArgTs...>(object, member);
}

template<typename ReturnT, typename... ArgTs>
template<typename ClassT>
function<ReturnT(ArgTs...)>::function(std::shared_ptr<ClassT> object)
  : m_empty(false)
{
  using call_operator_ptr_t = typename sfinae::generic_call_operator<ClassT, ReturnT, ArgTs...>::type;

  static_assert(sizeof(member_function_reference<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>) <=
                  HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                HANDLEBARS_FUNCTION_ERROR);
  new (access()) member_function_reference<ClassT, call_operator_ptr_t, ReturnT, ArgTs...>(object, &ClassT::operator());
}

template<typename ReturnT, typename... ArgTs>
function<ReturnT(ArgTs...)>::function(function_pointer_type function_pointer)
  : m_empty(false)
{
  static_assert(sizeof(function_pointer_type) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE, HANDLEBARS_FUNCTION_ERROR);
  new (access()) free_function<ReturnT, ArgTs...>(function_pointer);
}
}