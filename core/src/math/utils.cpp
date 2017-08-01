#include "utils.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  int64_t roundUpMultipleInt(int64_t value, int64_t multiplier)
  {
    return value + ((multiplier - (value % multiplier)) % multiplier);
  }

  size_t roundUpMultiple(size_t value, size_t multiple)
  {
    F_ASSERT(multiple, "multiple needs to be power of 2 was %d", multiple);
    return ((value + multiple - 1) / multiple) * multiple;
  }

  size_t roundUpMultiple2(size_t numToRound, size_t multiple)
  {
    F_ASSERT(multiple && ((multiple & (multiple - 1)) == 0), "multiple needs to be power of 2 was %d", multiple);
    return (numToRound + multiple - 1) & ~(multiple - 1);
  }
}