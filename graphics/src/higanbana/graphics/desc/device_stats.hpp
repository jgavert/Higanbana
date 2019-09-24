#pragma once

#include <cstdint>

namespace higanbana
{
  struct DeviceStatistics
  {
    uint64_t constantsUploadMemoryInUse;
    uint64_t maxConstantsUploadMemory;
    uint64_t genericUploadMemoryInUse;
    uint64_t maxGenericUploadMemory;
    bool descriptorsInShaderArguments;
    uint64_t descriptorsAllocated;
    uint64_t maxDescriptors;
    uint64_t commandlistsOnGpu;
    uint64_t gpuMemoryAllocated;
  };
}