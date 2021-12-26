#include "higanbana/core/sort/radix_sort_coro.hpp"

namespace higanbana
{
css::Task<void> radix_sort_8bits_task(vector<unsigned>& data) {
  auto copy = data;
  constexpr const unsigned radix_size = 256;
  
  for (int bitOffset = 0; bitOffset < 32; bitOffset += 8) {
    unsigned mask = (radix_size-1) << bitOffset;
    unsigned counts[radix_size]{};

    // count digits
    for (unsigned i = 0; i < data.size(); i++) {
      auto val = (data[i] & mask) >> bitOffset;
      counts[val]++;
    }
    // prefix sum
    unsigned sum = 0;
    for (unsigned i = 0; i < radix_size; i++) {
      counts[i] += sum;
      sum = counts[i];
    }
    // write copy 
    for (unsigned i = data.size(); i > 0; i--) {
      auto actualValue = data[i-1];
      auto val = (actualValue & mask) >> bitOffset;
      int index = --counts[val];
      copy[index] = actualValue;
    }
    // write output
    std::copy(copy.begin(), copy.end(), data.begin());
  }
  co_return;
}

css::Task<void> radix_sort_count(unsigned* ptr, unsigned size, unsigned* counts, unsigned bitCount, unsigned bitOffset) {
  unsigned mask = ((1 << bitCount) - 1) << bitOffset;

  for (int i = 0; i < (1 << bitCount); i++) {
    counts[i] = 0;
  }

  for (int i = 0; i < size; i++) {
    auto val = (ptr[i] & mask) >> bitOffset;
    counts[val]++;
  }
  co_return;
}

css::Task<void> radix_sort_write_val(unsigned* output, unsigned* ptr, unsigned size, unsigned* counts, unsigned bitCount, unsigned bitOffset) {
  unsigned mask = ((1 << bitCount) - 1) << bitOffset;

  // write copy 
  for (unsigned i = size; i > 0; i--) {
    auto actualValue = ptr[i-1];
    auto val = (actualValue & mask) >> bitOffset;
    int index = --counts[val];
    output[index] = actualValue;
  }

  co_return;
}
}