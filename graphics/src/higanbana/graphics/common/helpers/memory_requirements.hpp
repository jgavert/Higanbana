#pragma once

#include <cstdint>

namespace higanbana
{
struct MemoryRequirements
{
  size_t alignment;
  size_t bytes;
  int64_t heapType;
};
}