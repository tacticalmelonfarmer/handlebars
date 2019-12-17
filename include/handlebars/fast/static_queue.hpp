#pragma once
/*
  here is a type that adapts a std::array to have FIFO queue-like behaviour, with certain restrictions.
  when an element is popped from front of queue its storage becomes recycled storage and the begin iterator is moved
  forward. when an element is attemting to occupy past the last element of the internal array the structure will check
  for recycled storage if none is available, this is an error

  after enough usage the recycled storage will reset itself, along with begin and end iterators, which is intended and
  unavoidable

  common name: cyclical buffer, ring buffer
*/

#include <array>
#include <atomic>

namespace tmf {

template<typename T, size_t Capacity>
struct static_queue
{
  struct iterator
  {
    // default construct to an invalid state
    iterator()
      : container{ nullptr }
      , offset{ Capacity }
    {}

    // construct with container pointer and offset
    iterator(static_queue<T, Capacity>* init_container, size_t init_offset)
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
      if (container == nullptr || offset >= container->queue_size) {
        return nullptr; // invlaid iterator
      }

      size_t recycled_size = 0;
      if (offset < container->actual_begin) {
        if (container->recycling) {
          recycled_size = container->actual_begin + container->queue_size - Capacity;
          if (offset < recycled_size) {
            return &(container->internal_array[offset]); // valid!
          } else {
            return nullptr; // invlaid iterator
          }
        } else {
          return nullptr; // invalid iterator
        }
      } else {
        if (offset < Capacity) {
          return &(container->internal_array[offset]); // valid!
        } else {
          return nullptr;
        }
      }
    }

    T* operator->() { return this->operator T*(); }

  private:
    static_queue<T, Capacity>* container;
    size_t offset;
  };

  template<typename U>
  bool push(U&& new_value)
  {
    size_t insert_index = actual_begin + queue_size;
    if (insert_index < Capacity) {
      ::new (&internal_array[insert_index]) U{ std::forward<U>(new_value) };
      queue_size += 1;
      return true;
    } else {
      if (recycling) {
        insert_index = insert_index - Capacity;
        if (insert_index < actual_begin) {
          // container has available space!
          ::new (&internal_array[insert_index]) U{ std::forward<U>(new_value) };
          queue_size += 1;
          return true;
        } else {
          return false; // container is full
        }
      } else {
        return false; // container is full
      }
    }
  }

  // this returns the end of the queue, which can also be thought of as the last person in line
  T* last()
  {
    if (queue_size == 0) {
      return nullptr;
    } else {
      if (recycling) {
        return &internal_array[actual_begin + queue_size - Capacity];
      } else {
        return &internal_array[actual_begin + queue_size];
      }
    }
  }

  T* operator[](size_t index)
  {
    if (queue_size == 0) {
      return nullptr;
    } else {
      if (recycling) {
        return &internal_array[actual_begin + index - Capacity];
      } else {
        return &internal_array[actual_begin + index];
      }
    }
  }

  // this returns the "front" of the queue, which can also be thought of as the next person in line
  T* next()
  {
    if (queue_size == 0) {
      return nullptr;
    } else {
      return &(internal_array[actual_begin]);
    }
  }

  // remove the front of the queue calling its destructor
  bool pop()
  {
    if (queue_size == 0) {
      return false;
    }
    internal_array[actual_begin].~T();
    if (actual_begin == 0) {
      recycling = true;
    }
    ++actual_begin;
    --queue_size;
    if (actual_begin == Capacity) {
      actual_begin = 0;
      recycling = false;
    }
    return true;
  }

  void clear()
  {
    for (auto&& element : *this) {
      element.~T();
    }
    actual_begin = 0;
    recycling = false;
    queue_size = 0;
  }

  // default construct with an empty recycled section
  static_queue()
    : actual_begin{ 0 }
    , recycling{ false }
    , queue_size{ 0 }
    , internal_array{}
  {}

  iterator begin() { return iterator{ this, actual_begin }; }

  iterator end()
  {
    if (recycling) {
      return iterator{ this, (actual_begin + queue_size) - Capacity };
    } else {
      return iterator{ this, actual_begin + queue_size };
    }
  }

  size_t size() const { return queue_size; }

private:
  std::array<T, Capacity> internal_array;
  size_t actual_begin;
  std::atomic<size_t> queue_size;
  bool recycling;
};
}