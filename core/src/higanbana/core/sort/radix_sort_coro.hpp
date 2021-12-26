#pragma once
#include "higanbana/core/datastructures/vector.hpp"
#include <css/task.hpp>

namespace higanbana
{
css::Task<void> radix_sort_8bits_task(vector<unsigned>& data);
css::Task<void> radix_sort_count(unsigned* ptr, unsigned size, unsigned* counts, unsigned bitCount, unsigned bitOffset);
css::Task<void> radix_sort_write_val(unsigned* output, unsigned* ptr, unsigned size, unsigned* counts, unsigned bitCount, unsigned bitOffset);

template<unsigned radixBits, unsigned taskSplit>
css::Task<void> radix_sort_task(vector<unsigned>& data) {
  auto copy = data;
  unsigned* counts = reinterpret_cast<unsigned*>(css::s_stealPool->localAllocate(taskSplit * (1 << radixBits) * sizeof(unsigned)));
  
  for (unsigned bitOffset = 0; bitOffset < 32; bitOffset += radixBits) {
    unsigned bitsToHandle = std::min(32u - bitOffset, radixBits);
    unsigned mask = ((1 << bitsToHandle) - 1) << bitOffset;
    unsigned countsSize = (1 << bitsToHandle);

    size_t indicesHandled = 0;
    size_t splitTo = (data.size()+taskSplit) / taskSplit;
    for (int t = 0; t < taskSplit; t++) {
      size_t portion = std::min(splitTo, data.size() - indicesHandled);
      co_await radix_sort_count(data.data() + indicesHandled, portion, counts + t * countsSize, bitsToHandle, bitOffset);
      indicesHandled += portion;
    }

    // prefix sum
    unsigned sum = 0;
    for (unsigned i = 0; i < countsSize; i++) {
      for (unsigned t = 0; t < taskSplit; t++) {
        counts[t * countsSize + i] += sum;
        sum = counts[t * countsSize + i];
      }
    }
    // write copy threaded
    indicesHandled = 0;
    for (int t = 0; t < taskSplit; t++) {
      size_t portion = std::min(splitTo, data.size() - indicesHandled);
      co_await radix_sort_write_val(copy.data(), data.data() + indicesHandled, portion, counts + t * countsSize, bitsToHandle, bitOffset);
      indicesHandled += portion;
    }

    // write output
    std::copy(copy.begin(), copy.end(), data.begin());
  }
  css::s_stealPool->localFree(counts, taskSplit * (1 << radixBits) * sizeof(unsigned));
  co_return;
}

template<unsigned radixBits, unsigned taskSplit>
css::Task<void> radix_sort_task_fast(vector<unsigned>& data) {
  auto copy = data;
  vector<unsigned>* originalPtr = &data;
  vector<unsigned>* read = &data;
  vector<unsigned>* output = &copy;
  const size_t dataSize = data.size();
  unsigned* counts = reinterpret_cast<unsigned*>(css::s_stealPool->localAllocate(taskSplit * (1 << radixBits) * sizeof(unsigned)));
  
  for (unsigned bitOffset = 0; bitOffset < 32; bitOffset += radixBits) {
    unsigned bitsToHandle = std::min(32u - bitOffset, radixBits);
    unsigned mask = ((1 << bitsToHandle) - 1) << bitOffset;
    unsigned countsSize = (1 << bitsToHandle);

    size_t indicesHandled = 0;
    size_t splitTo = (dataSize+taskSplit) / taskSplit;
    vector<css::Task<void>> runningTasks;
    for (int t = 0; t < taskSplit; t++) {
      size_t portion = std::min(splitTo, dataSize - indicesHandled);
      runningTasks.emplace_back(radix_sort_count(read->data() + indicesHandled, portion, counts + t * countsSize, bitsToHandle, bitOffset));
      indicesHandled += portion;
    }
    for (auto&& task : runningTasks) {
      co_await task;
    }
    runningTasks.clear();

    // prefix sum
    unsigned sum = 0;
    for (unsigned i = 0; i < countsSize; i++) {
      for (unsigned t = 0; t < taskSplit; t++) {
        counts[t * countsSize + i] += sum;
        sum = counts[t * countsSize + i];
      }
    }
    // write copy threaded
    indicesHandled = 0;
    for (int t = 0; t < taskSplit; t++) {
      size_t portion = std::min(splitTo, dataSize - indicesHandled);
      runningTasks.emplace_back(radix_sort_write_val(output->data(), read->data() + indicesHandled, portion, counts + t * countsSize, bitsToHandle, bitOffset));
      indicesHandled += portion;
    }
    for (auto&& task : runningTasks) {
      co_await task;
    }
    runningTasks.clear();

    // write output
    std::swap(read, output);
    //std::copy(copy.begin(), copy.end(), data.begin());
  }
  css::s_stealPool->localFree(counts, taskSplit * (1 << radixBits) * sizeof(unsigned));
  // write output
  if (read != originalPtr)
    std::copy(read->begin(), read->end(), originalPtr->begin());
  co_return;
}
}