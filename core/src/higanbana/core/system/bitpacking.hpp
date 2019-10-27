#pragma once
#include <cstdint>

namespace higanbana
{
  template <typename Type, typename Type2>
  Type packValue(Type2 value, int offset, int bits)
  {
    Type val = static_cast<Type>(value);
    Type mask = (1 << bits) - 1;
    val = val & mask;
    val = val << offset;
    return val;
  }

  template <typename Type, typename Type2>
  Type unpackValue(Type2 value, int offset, int bits)
  {
    Type2 val = value >> offset;
    Type2 mask = (1 << bits) - 1;
    val = val & mask;
    return static_cast<Type>(val);
  }
  int64_t packInt64(int32_t first, int32_t second);
  void unpackInt64(int64_t val, int32_t& first, int32_t& second);
}