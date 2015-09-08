#pragma once
#include "bitfield.hpp"
#include <memory>
#include <cassert>

namespace faze
{
  template <typename T, size_t tsize>
  class _TagTable
  {
  private:
    Bitfield<tsize> m_bitfield;

  public:
    _TagTable() {}

    void insert(size_t id)
    {
      assert(id < tsize * 128);
      m_bitfield.setIdxBit(id);
    }

    void remove(size_t id)
    {
      m_bitfield.clearIdxBit(id);
    }

    void toggle(size_t id)
    {
      m_bitfield.toggleIdxBit(id);
    }

    bool check(size_t id)
    {
      return m_bitfield.checkIdxBit(id);
    }

    bool checkAndDisable(size_t id)
    {
      bool ret = m_bitfield.checkIdxBit(id);
      m_bitfield.clearIdxBit(id);
      return ret;
    }

    size_t size() const
    {
      return tsize;
    }

    Bitfield<tsize>& getBitfield()
    {
      return m_bitfield;
    }
  };
}

