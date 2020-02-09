#pragma once

#include <higanbana/graphics/GraphicsCore.hpp>
#include <higanbana/core/system/heap_allocator.hpp>

namespace app
{
  class GPUBuffer
  {
    higanbana::Buffer buffer;
    higanbana::BufferSRV bufferSRV;
    higanbana::HeapAllocator allocator;
    public:
    GPUBuffer(higanbana::GpuGroup& group);
  };
}