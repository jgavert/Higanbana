#include "higanbana/core/sort/radix_sort.hpp"

namespace higanbana
{
void radix_sort_8bits(vector<unsigned>& data) {
  auto copy = data;
  vector<unsigned>* originalPtr = &data;
  vector<unsigned>* read = &data;
  vector<unsigned>* output = &copy;
  constexpr const unsigned radix_size = 256;
  
  for (int bitOffset = 0; bitOffset < 32; bitOffset += 8) {
    unsigned mask = (radix_size-1) << bitOffset;
    unsigned counts[radix_size]{};

    // count digits
    for (unsigned i = 0; i < read->size(); i++) {
      auto val = ((*read)[i] & mask) >> bitOffset;
      counts[val]++;
    }
    // prefix sum
    unsigned sum = 0;
    for (unsigned i = 0; i < radix_size; i++) {
      counts[i] += sum;
      sum = counts[i];
    }
    // write copy 
    for (unsigned i = read->size(); i > 0; i--) {
      auto actualValue = (*read)[i-1];
      auto val = (actualValue & mask) >> bitOffset;
      int index = --counts[val];
      (*output)[index] = actualValue;
    }
    std::swap(read, output);
  }
  // write output
  if (read != originalPtr)
    std::copy(read->begin(), read->end(), originalPtr->begin());
}
void radix_sort_12bits(vector<unsigned>& data) {
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
}
}