#include "higanbana/core/system/bitpacking.hpp"

namespace higanbana
{
  int64_t packInt64(int32_t first, int32_t second)
  {
    int64_t val = first;
    val = val << 32;
    val += second;
    return val;
  }

  void unpackInt64(int64_t val, int32_t& first, int32_t& second)
  {
    first = static_cast<int32_t>(val >> 32);
    second = static_cast<int32_t>(val);
  }
}