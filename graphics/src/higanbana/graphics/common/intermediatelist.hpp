#pragma once
#include "higanbana/graphics/common/allocators.hpp"
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/math/utils.hpp>
#include <higanbana/core/global_debug.hpp>
#include <memory>

namespace higanbana
{
  namespace backend
  {
    class CommandPacket
    {
    private:
      CommandPacket * m_nextPacket = nullptr;
    public:

      enum class PacketType
      {
        QueryCounters,
        RenderBlock,
        BufferCopy,
        TextureCopy,
        TextureAdvCopy,
        TextureToBufferCopy,
        BufferCpuToGpuCopy,
        Readback,
        ReadbackTexture,
        UpdateTexture,
        Dispatch,
        ClearRT,
        PrepareForPresent,
        RenderpassBegin,
        RenderpassEnd,
        GraphicsPipelineBind,
        ComputePipelineBind,
        ResourceBinding,
        Draw,
        DrawIndexed,
        DrawDynamicIndexed,
        SetScissorRect,
        PrepareForQueueSwitch
      };

      CommandPacket();
      void setNextPacket(CommandPacket* packet);
      CommandPacket* nextPacket();

      virtual PacketType type() = 0;
      virtual ~CommandPacket() {}
    };

    // Average size estimation for a single continuos list is small.
    // These can be linked together to form one big list.
    class ListAllocator
    {
      LinearAllocator& allocator;
      vector<std::unique_ptr<uint8_t[]>>& memories;
      int& activeMemory;

      uintptr_t privateAllocate(size_t size)
      {
        auto offset = allocator.allocate(size, 16); // sizeof(uint32_t)*4
        if (offset == -1)
        {
          int64_t refSize = 1024;
          if (static_cast<int64_t>(size) > refSize)
          {
            refSize = roundUpMultipleInt(static_cast<int64_t>(size), 1024);
          }
          activeMemory = static_cast<int>(memories.size());
          memories.emplace_back(std::make_unique<uint8_t[]>(refSize));
          allocator.reset();
          allocator.resize(refSize);
          offset = allocator.allocate(size, 16);
        }
        F_ASSERT(offset != -1, "should always be fine here");
        return static_cast<uintptr_t>(offset);
      }
    public:
      ListAllocator(LinearAllocator& allocator, vector<std::unique_ptr<uint8_t[]>>& memories, int& activeMemory)
        : allocator(allocator)
        , memories(memories)
        , activeMemory(activeMemory)
      {
      }

      template <typename T>
      T* allocate(size_t count = 1)
      {
        auto size = sizeof(T)*count;
        if (size == 0)
        {
          return nullptr;
        }
        auto allocation = privateAllocate(size);
        return reinterpret_cast<T*>(allocation + memories[activeMemory].get());
      }
    };

    class IntermediateList
    {
      CommandPacket* m_firstPacket = nullptr;
      CommandPacket* m_lastPacket = nullptr;
      size_t m_size = 0;
      vector<std::unique_ptr<uint8_t[]>> m_memory;
      LinearAllocator m_allocator;
      int m_activeMemory = -1;
    public:
      IntermediateList();
      IntermediateList(size_t size);
      IntermediateList(std::unique_ptr<uint8_t[]> memory, size_t size);
      IntermediateList(IntermediateList&& obj);
      IntermediateList& operator=(IntermediateList&& obj);
      IntermediateList(const IntermediateList& obj) = delete;
      IntermediateList& operator=(const IntermediateList& obj) = delete;

      void append(IntermediateList&& other);

      ~IntermediateList();
      void clear();
      size_t size() const { return m_size; }

      template <typename Type, typename... Args>
      Type& insert(Args&&... args)
      {
        Type* ptr = allocator().allocate<Type>();
        ptr = new (ptr) Type(allocator(), std::forward<Args>(args)...);
        if (m_lastPacket != nullptr)
        {
          m_lastPacket->setNextPacket(ptr);
        }
        if (m_firstPacket == nullptr)
        {
          m_firstPacket = ptr;
        }
        m_lastPacket = ptr;
        m_size++;
        return *ptr;
      }

      ListAllocator allocator() { return ListAllocator(m_allocator, m_memory, m_activeMemory); }

      class iterator {
      public:
        typedef typename std::ptrdiff_t difference_type;
        typedef CommandPacket* value_type;
        typedef std::forward_iterator_tag iterator_category; //or another tag
      private:
        CommandPacket * item = nullptr;
      public:

        iterator() {}
        iterator(CommandPacket* item)
          : item(item)
        {}
        iterator(const iterator& iter)
          : item(iter.item)
        {
        }
        ~iterator() {}

        iterator& operator=(const iterator& o) = default;
        bool operator==(const iterator& o) const
        {
          return o.item == item;
        }
        bool operator!=(const iterator& o) const
        {
          return o.item != item;
        }

        iterator& operator++()
        {
          item = item->nextPacket();
          return *this;
        }

        value_type operator*() const
        {
          return item;
        }
        value_type operator->() const
        {
          return item;
        }
      };
      class const_iterator {
      public:
        typedef typename std::ptrdiff_t difference_type;
        typedef CommandPacket* value_type;
        typedef std::forward_iterator_tag iterator_category; //or another tag
      private:
        CommandPacket * item = nullptr;
      public:

        const_iterator() {}
        const_iterator(CommandPacket* item)
          : item(item)
        {}
        const_iterator(const const_iterator& o)
        {
          item = o.item;
        }
        const_iterator(const iterator& o)
        {
          item = *o;
        }
        ~const_iterator() {}

        const_iterator& operator=(const const_iterator&) = default;
        bool operator==(const const_iterator& o) const
        {
          return o.item == item;
        }
        bool operator!=(const const_iterator& o) const
        {
          return o.item != item;
        }

        const_iterator& operator++()
        {
          item = item->nextPacket();
          return *this;
        }

        value_type operator*() const
        {
          return item;
        }
        value_type operator->() const
        {
          return item;
        }
      };

      iterator begin()
      {
        return iterator(m_firstPacket);
      }
      const_iterator begin() const
      {
        return const_iterator(m_firstPacket);
      }
      const_iterator cbegin() const
      {
        return const_iterator(m_firstPacket);
      }
      iterator end()
      {
        return iterator(nullptr);
      }
      const_iterator end() const
      {
        return const_iterator(nullptr);
      }
      const_iterator cend() const
      {
        return const_iterator(nullptr);
      }

      iterator last()
      {
        return iterator(m_lastPacket);
      }
      const_iterator last() const
      {
        return const_iterator(m_lastPacket);
      }
      const_iterator clast() const
      {
        return const_iterator(m_lastPacket);
      }
    };
  }
}