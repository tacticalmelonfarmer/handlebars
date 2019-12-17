#pragma once
/*
  here is a type that adapts a std::array to have FILO stack-like bahaviour, with a fixed size
*/

#include <atomic>
#include <array>
#include <type_traits>

namespace tmf {

template<typename T, size_t Capacity>
struct static_stack
{
  struct iterator
  {
    // default construct to an invalid state
    iterator()
      : container{ nullptr }
      , offset{ Capacity }
    {}

    // construct with contianer pointer and offset
    iterator(static_stack<T, Capacity>* init_container, size_t init_offset)
      : container{ init_container }
      , offset{ init_offset }
    {}

    bool operator==(const iterator& other)
    {
      if (other.container == container && other.offset == offset)
        return true;
      else
        return false;
    }

    bool operator!=(const iterator& other)
    {
      if (other.container == container && other.offset == offset)
        return false;
      else
        return true;
    }

    iterator& operator+=(size_t add_offset)
    {
      if ((offset + add_offset) > Capacity)
        offset = Capacity - 1;
      else
        offset += add_offset;
      return *this;
    }

    iterator& operator-=(size_t sub_offset)
    {
      if (offset < sub_offset)
        offset = 0;
      else
        offset -= sub_offset;
      return *this;
    }

    iterator& operator++() { return this->operator+=(1); }
    iterator operator++(int)
    {
      iterator result{ container, offset };
      this->operator+=(1);
      return result;
    }

    iterator& operator--() { return this->operator-=(1); }
    iterator operator--(int)
    {
      iterator result{ container, offset };
      this->operator-=(1);
      return result;
    }

    iterator operator+(size_t add_offset)
    {
      iterator result{ container, offset };
      result += add_offset;
      return result;
    }

    iterator operator-(size_t sub_offset)
    {
      iterator result{ container, offset };
      result -= sub_offset;
      return result;
    }

    operator T*()
    {
      if (container == nullptr || offset >= container->stack_size) {
        return nullptr; // invlaid iterator
      }
      if (offset < container->stack_size) {
        return &(container->internal_array[offset]);
      }
      return nullptr;
    }

    T* operator->()
    {
      return this->operator T*();
    }

  private:
    static_stack<T, Capacity>* container;
    size_t offset;
  };

  template<typename U>
  bool push(U&& new_value)
  {
    if (stack_size < Capacity) {
      ::new (&(internal_array[stack_size])) T{ std::forward<U>(new_value) };
      stack_size += 1;
      return true;
    }
    return false;
  }

  // this returns a pointer to the the top of the stack, null if empty
  T* top()
  {
    if (stack_size == 0) {
      return nullptr;
    } else {
      return &(internal_array[stack_size - 1]);
    }
  }

  // remove the top of the stack calling its destructor and decrementing the size
  bool pop()
  {
    if (stack_size == 0) {
      return false;
    }
    // decrement size and destroy the element
    internal_array[--stack_size].~T();
    return true;
  }

  T* operator[](size_t index)
  {
    if (index < stack_size) {
      return &internal_array[index];
    } else {
      return nullptr;
    }
  }

  void clear()
  {
    for (auto& element : *this) {
      element.~T();
    }
    stack_size = 0;
  }

  static_stack()
    : stack_size{ 0 }
    , internal_array{}
  {}

  iterator begin() { return iterator{ this, 0 }; }

  iterator end() { return iterator{ this, 0 + stack_size }; }

  size_t size() const 
  { return stack_size; }

private:
  std::array<T, Capacity> internal_array;
  std::atomic<size_t> stack_size;
};
}