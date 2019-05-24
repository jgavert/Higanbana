#pragma once
#include <cstdint>

namespace faze
{
  int64_t packInt64(int32_t first, int32_t second);
  void unpackInt64(int64_t val, int32_t& first, int32_t& second);
}