#pragma once

namespace faze
{
  int64_t roundUpMultipleInt(int64_t value, int64_t multiplier);
  size_t roundUpMultiple(size_t value, size_t powerof2);
  size_t roundUpMultiple2(size_t numToRound, size_t powerof2);
}