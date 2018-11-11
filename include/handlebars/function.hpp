#pragma once

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef HANDLEBARS_FUNCTION_COMMON_MAX_SIZE
#define HANDLEBARS_FUNCTION_COMMON_MAX_SIZE 64
#endif

#define HANDLEBARS_FUNCTION_ERROR                                                                                      \
  "handlebars::function cannot hold a callable this large, try adjusting macro 'HANDLEBARS_FUNCTION_COMMON_MAX_SIZE' " \
  "before including this file"

namespace handlebars {
inline namespace detail {
template<typename ReturnT, typename... ArgTs>
struct function_base
{
  virtual ~function_base() {}
  virtual ReturnT operator()(ArgTs&&...) = 0;
};

// a member function of an instance of a class
template<typename ClassT, typename MemPtrT, typename ReturnT, typename... ArgTs>
struct member_function : function_base<ReturnT, ArgTs...>
{
  member_function(ClassT&& object, MemPtrT member)
    : m_object(std::forward<ClassT>(object))
    , m_member(member)
  {}
  ReturnT operator()(ArgTs&&... args) override { return (m_object.*m_member)(std::forward<ArgTs>(args)...); }

private:
  ClassT m_object;
  MemPtrT m_member;
};

// any free function with assignable to ReturnT(*)(ArgTs...)
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
}

namespace sfinae {

template<typename T, typename ReturnT, typename... ArgTs>
struct has_const_call_operator
{
  struct yes
  {};
  struct no
  {};
  static constexpr yes check(ReturnT (T::*)(ArgTs...) const) { return {}; }
  static constexpr no check(...) { return {}; }
  static constexpr bool doublecheck(yes) { return true; }
  static constexpr bool doublecheck(no) { return false; }
  constexpr operator bool() const { return doublecheck(check(&T::operator())); }
};
}

struct empty_function
{};

template<typename>
struct function;

template<typename ReturnT, typename... ArgTs>
struct function<ReturnT(ArgTs...)>
{
  using function_pointer_type = ReturnT (*)(ArgTs...);

  function()
    : m_empty(true)
  {}

  template<typename ClassT, typename MemPtrT>
  function(ClassT&& object, MemPtrT member)
    : m_empty(false)
  {
    static_assert(sizeof(member_function<ClassT, MemPtrT, ReturnT, ArgTs...>) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                  HANDLEBARS_FUNCTION_ERROR);
    new (access()) member_function<ClassT, MemPtrT, ReturnT, ArgTs...>(std::forward<ClassT>(object), member);
  }

  function(function_pointer_type function_pointer)
    : m_empty(false)
  {
    static_assert(sizeof(function_pointer_type) <= HANDLEBARS_FUNCTION_COMMON_MAX_SIZE, HANDLEBARS_FUNCTION_ERROR);
    new (access()) free_function<ReturnT, ArgTs...>(function_pointer);
  }

  template<typename ClassT>
  function(ClassT&& object)
    : m_empty(false)
  {
    if constexpr (sfinae::has_const_call_operator<ClassT, ReturnT, ArgTs...>{}) {
      using member_ptr_t = ReturnT (ClassT::*)(ArgTs...) const;
      static_assert(sizeof(member_function<ClassT, member_ptr_t, ReturnT, ArgTs...>) <=
                      HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                    HANDLEBARS_FUNCTION_ERROR);
      new (access())
        member_function<ClassT, member_ptr_t, ReturnT, ArgTs...>(std::forward<ClassT>(object), &ClassT::operator());
    } else {
      using member_ptr_t = ReturnT (ClassT::*)(ArgTs...);
      static_assert(sizeof(member_function<ClassT, member_ptr_t, ReturnT, ArgTs...>) <=
                      HANDLEBARS_FUNCTION_COMMON_MAX_SIZE,
                    HANDLEBARS_FUNCTION_ERROR);
      new (access())
        member_function<ClassT, member_ptr_t, ReturnT, ArgTs...>(std::forward<ClassT>(object), &ClassT::operator());
    }
  }

  function(const function<ReturnT(ArgTs...)>& other)
  {
    if (!other.m_empty) {
      m_storage = other.m_storage;
      m_empty = false;
    }
  }

  function(function<ReturnT(ArgTs...)>&& other) noexcept
  {
    if (other.m_empty) {
      m_storage = std::move(other.m_storage);
      other.m_empty = true;
      m_empty = false;
    }
  }

  function<ReturnT(ArgTs...)>& operator=(const function<ReturnT(ArgTs...)>& rhs)
  {
    if (rhs.m_empty)
      return *this;
    m_storage = rhs.m_storage;
    m_empty = false;
    return *this;
  }

  function<ReturnT(ArgTs...)>& operator=(function<ReturnT(ArgTs...)>&& rhs) noexcept
  {
    if (rhs.m_empty)
      return *this;
    m_storage = std::move(rhs.m_storage);
    rhs.m_empty = true;
    m_empty = false;
    return *this;
  }

  ReturnT operator()(ArgTs&&... arguments)
  {
    if (m_empty)
      throw empty_function{};
    else
      return access()->operator()(std::forward<ArgTs>(arguments)...);
  }

  ~function()
  {
    if (!m_empty)
      access()->~function_base<ReturnT, ArgTs...>();
  }

private:
  function_base<ReturnT, ArgTs...>* access() { return reinterpret_cast<function_base<ReturnT, ArgTs...>*>(&m_storage); }
  bool m_empty;
  std::aligned_storage_t<HANDLEBARS_FUNCTION_COMMON_MAX_SIZE> m_storage;
};
}