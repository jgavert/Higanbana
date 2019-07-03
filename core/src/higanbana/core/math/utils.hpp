#pragma once
#include <cstdint>
namespace higanbana
{
  uint64_t divideRoundUp(uint64_t val, uint64_t multiplier);
  uint64_t roundUpMultiple(uint64_t val, uint64_t multiplier);
  int64_t roundUpMultipleInt(int64_t value, int64_t multiplier);
  size_t roundUpMultiplePowerOf(size_t value, size_t powerof2);
  size_t roundUpMultiplePowerOf2(size_t numToRound, size_t powerof2);
}