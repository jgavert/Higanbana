#pragma once
#include "bitfield.hpp"
#include <memory>
#include <array>
#include <cassert>

namespace higanbana
{
  typedef size_t Id;

  template <typename T, size_t t_size, size_t r_size>
  class _SparseTable
  {
  private:
    std::unique_ptr<std::array<T, t_size>> m_array;
    Bitfield<r_size> m_bitfield;

  public:
    _SparseTable() :m_array(std::make_unique<std::array<T, t_size>>()) {}

    void insert_m(Id id, T&& component)
    {
      assert(id < t_size);
      (*m_array)[id] = std::move(component);
      m_bitfield.setIdxBit(id);
    }

    void insert(Id id, T component)
    {
      assert(id < t_size);
      (*m_array)[id] = std::move(component);
      m_bitfield.setIdxBit(id);
    }

    void remove(Id id)
    {
      m_bitfield.clearIdxBit(id);
    }

    bool check(Id id)
    {
      return m_bitfield.checkIdxBit(id);
    }

    inline T& get(Id id)
    {
      return (*m_array)[id];
    }

    size_t size() const
    {
      return t_size;
    }

    Bitfield<r_size>& getBitfield()
    {
      return m_bitfield;
    }

    inline char* getPtr()
    {
      return reinterpret_cast<char*>(m_array.Get());
    }
  };
}
