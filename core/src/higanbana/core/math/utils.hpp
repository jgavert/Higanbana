#pragma once
#include "higanbana/core/global_debug.hpp"
#include <cstdint>
namespace higanbana
{
  /*
  uint64_t divideRoundUp(uint64_t val, uint64_t multiplier);
  uint64_t roundUpMultiple(uint64_t val, uint64_t multiplier);
  int64_t roundUpMultipleInt(int64_t value, int64_t multiplier);
  size_t roundUpMultiplePowerOf(size_t value, size_t powerof2);
  size_t roundUpMultiplePowerOf2(size_t numToRound, size_t powerof2);
  */
	inline uint64_t divideRoundUp(uint64_t val, uint64_t multiplier)
	{
		return val / multiplier + ((val % multiplier == 0) ? 0 : 1);
  }
  
	inline uint64_t roundUpMultiple(uint64_t val, uint64_t multiplier)
	{
		return divideRoundUp(val, multiplier) * multiplier;
	}

  inline int64_t roundUpMultipleInt(int64_t value, int64_t multiplier)
  {
    return value + ((multiplier - (value % multiplier)) % multiplier);
  }

  inline size_t roundUpMultiplePowerOf(size_t value, size_t multiple)
  {
    HIGAN_ASSERT(multiple, "multiple needs to be power of 2 was %d", multiple);
    return ((value + multiple - 1) / multiple) * multiple;
  }

  inline size_t roundUpMultiplePowerOf2(size_t numToRound, size_t multiple)
  {
    HIGAN_ASSERT(multiple && ((multiple & (multiple - 1)) == 0), "multiple needs to be power of 2 was %d", multiple);
    return (numToRound + multiple - 1) & ~(multiple - 1);
  }
}