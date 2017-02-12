#pragma once
#include "core/src/system/PageAllocator.hpp"
#include "core/src/global_debug.hpp"
#include "vk_specifics/VulkanHeap.hpp"
#include "HeapDescriptor.hpp"

#include <mutex>
#include <memory>

class ResourceHeap 
{
  MemoryHeapImpl                       m_resource;
  std::shared_ptr<faze::PageAllocator> m_pages; // 16*128 pages, enough? hopefully. atleast simple.

public:

  ResourceHeap(MemoryHeapImpl impl)
    : m_resource{ impl }
    , m_pages{std::make_shared<faze::PageAllocator>(static_cast<int>(impl.desc().m_alignment)
                                                  , impl.desc().m_sizeInBytes/impl.desc().m_alignment)}
  {}

  faze::PageBlock allocate(uint64_t sizeInBytes)
  {
    auto block = m_pages->allocate(sizeInBytes);
    
    return block;
  }

  void release(faze::PageBlock block)
  {
    m_pages->release(block);
  }

  MemoryHeapImpl& impl()
  {
    return m_resource;
  }

  HeapDescriptor desc()
  {
    return m_resource.desc();
  }

  bool isValid() const
  {
    return true;
  }
};


