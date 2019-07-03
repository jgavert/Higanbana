#pragma once

namespace higanbana
{
  template <typename T, T NullValue = nullptr>
  struct MovePtr
  {
    T ptr = NullValue;
    static_assert(std::is_pointer<T>::value, "T must be a pointer type.");

    MovePtr(T ptr)
      : ptr(ptr)
    {
    }

    MovePtr(MovePtr&& obj)
      : ptr(obj.ptr)
    {
      obj.ptr = NullValue;
    }

    MovePtr(const MovePtr& obj) = delete;

    MovePtr& operator=(MovePtr&& obj)
    {
      if (&obj != this)
      {
        ptr = obj.ptr;
        obj.ptr = NullValue;
      }
      return *this;
    }

    MovePtr& operator=(const MovePtr& obj) = delete;

    MovePtr& operator=(std::nullptr_t)
    {
      ptr = NullValue;
      return *this;
    }

    ~MovePtr()
    {
      ptr = NullValue;
    }

    T get() { return ptr; }
    const T get() const { return ptr; }

    operator T() { return get(); }
    operator const T() const { return get(); }

    T operator->() { return get(); }
    const T operator->() const { return get(); }
  };
}