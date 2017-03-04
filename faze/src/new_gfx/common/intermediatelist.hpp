#pragma once
#include "core/src/datastructures/proxy.hpp"
#include "core/src/global_debug.hpp"
#include "allocators.hpp"
#include <memory>

namespace faze
{
  namespace backend
  {
    class CommandPacket
    {
    private:
      CommandPacket* m_nextPacket = nullptr;
    public:

      enum class PacketType
      {
        BufferCopy,
        ClearRTV,
        PrepareForPresent
      };

      CommandPacket();
      void setNextPacket(CommandPacket* packet);
      CommandPacket* nextPacket();

      virtual PacketType type() = 0;
      virtual ~CommandPacket() {}
    };

    // Average size estimation for a single continuos list is small.
    // These can be linked together to form one big list.
    class IntermediateList
    {
    private:
      LinearAllocator m_allocator;
      CommandPacket* m_firstPacket = nullptr;
      CommandPacket* m_lastPacket = nullptr;
      size_t m_size = 0;
    public:
      IntermediateList(LinearAllocator&& allocator);
      IntermediateList(IntermediateList&& obj) = default;
      IntermediateList(const IntermediateList& obj) = delete;

      IntermediateList& operator=(IntermediateList&& obj) = default;
      IntermediateList& operator=(const IntermediateList& obj) = delete;

      ~IntermediateList();
      void clear();
      size_t size() const { return m_size; }

      template <typename Type, typename... Args>
      Type& insert(Args&&... args)
      {
        Type* ptr = m_allocator.alloc<Type>();
        ptr = new (ptr) Type(m_allocator, std::forward<Args>(args)...);
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

      LinearAllocator& allocator() {return m_allocator; }

      class iterator {
      public:
        typedef typename std::ptrdiff_t difference_type;
        typedef typename CommandPacket* value_type;
        typedef std::forward_iterator_tag iterator_category; //or another tag
      private:
        CommandPacket* item = nullptr;
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
        typedef typename CommandPacket* value_type;
        typedef std::forward_iterator_tag iterator_category; //or another tag
      private:
        CommandPacket* item = nullptr;
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
        return iterator(m_lastPacket);
      }
      const_iterator end() const
      {
        return const_iterator(m_lastPacket);
      }
      const_iterator cend() const
      {
        return const_iterator(m_lastPacket);
      }
    };
  }
}
