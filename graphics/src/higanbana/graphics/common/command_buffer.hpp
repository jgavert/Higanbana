#pragma once
#include <cstdio>
#include <memory>
#include <cstddef>
#include <cstring>
#include <vector>
#include <type_traits>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/global_debug.hpp>

#define VERIFY_PACKETS_ONE_BY_BY 0

// #define SIZE_DEBUG
namespace higanbana
{
  namespace backend
  {
    enum PacketType : uint32_t
    {
      Invalid = 0,
      RenderBlock,
      ReleaseFromQueue,
      BufferCopy,
      ReadbackBuffer,
      DynamicBufferCopy,
      UpdateTexture,
      TextureToBufferCopy,
      BufferToTextureCopy,
      TextureToTextureCopy,
      Dispatch,
      PrepareForPresent,
      RenderpassBegin,
      RenderpassEnd,
      GraphicsPipelineBind,
      ComputePipelineBind,
      ResourceBindingGraphics,
      ResourceBindingCompute,
      Draw,
      DrawIndexed,
      DispatchMesh,
      ScissorRect,
      ReadbackShaderDebug,
      EndOfPackets,
    };

    // data is expected o be in memory after the header
    template <typename T>
    struct PacketVectorHeader
    {
      uint16_t beginOffset;
      uint16_t elements;

      MemView<T> convertToMemView() const
      {
        T* ptr = reinterpret_cast<T*>(reinterpret_cast<size_t>(this) + beginOffset);
        if (elements == 0) return MemView<T>(nullptr, 0);
        return MemView<T>(ptr, elements);
      }
    };

    class CommandBuffer
    {
      vector<uint8_t> m_data;
      size_t m_totalSize;
      size_t m_usedSize;
      size_t m_packetBeingCreated;
      size_t m_packets = 0;

    public:
      // commandbuffer header
      struct PacketHeader
      {
        PacketType type : 8;
        uint32_t offsetFromThis : 24; // offset from start of header
    #if defined(SIZE_DEBUG)
        size_t length; // length of this packet, basically debug feature
    #endif
        template <typename T>
        T& data() 
        {
          HIGAN_ASSERT(type == T::type, "Uh oh, this is not allowed =D. header and requested data type's differ.");
          return *reinterpret_cast<T*>(this + 1);
        }
      };
    private:

      PacketHeader packetBeingCreated()
      {
        PacketHeader headr;
        memcpy(&headr, m_data.data()+m_packetBeingCreated, sizeof(PacketHeader));
        return headr;
      }

      void doubleSize()
      {
        auto newSize = m_totalSize * 4;
        m_data.resize(newSize);
        m_totalSize = newSize;
      }

      uint8_t* allocate(size_t size)
      {
        auto current = m_usedSize;
        while (m_usedSize + size >= m_totalSize)
        {
          doubleSize();
        }
        m_usedSize += size;
        //printf("allocated %zu size %zu\n", current, size);
        return &m_data.at(current);
      }

      PacketHeader beginNewPacket(PacketType type)
      {
        // patch current EOP to be actual packet header.
        auto hdr = packetBeingCreated();
        hdr.type = type;
        return hdr;
      }

      void newHeader()
      {
        uint8_t* ptr = allocate(sizeof(PacketHeader));
        size_t offset = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_data.data());
        m_packetBeingCreated = offset;
        PacketHeader header = {};
        header.type = PacketType::EndOfPackets;
    #if defined(size_debug)
        header.length = 0;
    #endif
        header.offsetFromThis = 0;
        memcpy(ptr, &header, sizeof(PacketHeader));
      }

      void endNewPacket(PacketHeader header)
      {
        // patch old packet
        size_t currentTop = m_usedSize;
        size_t thisPacket = m_packetBeingCreated;
        size_t diff = currentTop - thisPacket;
        header.offsetFromThis = static_cast<unsigned>(diff);
    #if defined(SIZE_DEBUG)
        header.length = m_usedSize - header.length;
    #endif
        memcpy(m_data.data()+m_packetBeingCreated, &header, sizeof(PacketHeader));
        m_packets++;
        // create new EOP
        newHeader();
      }

      void initialize()
      {
        if (m_totalSize < sizeof(PacketHeader))
        {
          m_data.resize(sizeof(PacketHeader));
          m_totalSize = m_data.size();
        }
        newHeader();
      }

    public:
      class CommandBufferIterator
      {
        PacketHeader* m_current;
      public:
        CommandBufferIterator() = default;
        CommandBufferIterator(const CommandBufferIterator&) = default;
        CommandBufferIterator& operator=(const CommandBufferIterator&) = default;

        CommandBufferIterator(PacketHeader* start)
            : m_current{ start }
        {
        }

        CommandBufferIterator& operator++(int)
        {
          size_t nextHeaderAddr = reinterpret_cast<size_t>(m_current) + m_current->offsetFromThis;
          m_current = reinterpret_cast<PacketHeader*>(nextHeaderAddr);
          return *this;
        }

        PacketHeader* operator*()
        {
          return m_current;
        }

        bool operator==(const CommandBufferIterator& it)
        {
          return m_current == it.m_current;
        }

        bool operator!=(const CommandBufferIterator& it)
        {
          return !operator==(it);
        }
      };
      CommandBuffer(size_t size = 10)
        : m_data(size)
        , m_totalSize(size)
        , m_usedSize(0)
      {
        // minimum requirements is one packet which indicates end of packets.
        initialize();
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "sanity check");
      }

      CommandBuffer(CommandBuffer&& other) noexcept
        : m_data(std::move(other.m_data))
        , m_totalSize(std::move(other.m_totalSize))
        , m_usedSize(std::move(other.m_usedSize))
        , m_packetBeingCreated(std::move(other.m_packetBeingCreated))
        , m_packets(other.m_packets)
      {
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "sanity check");
        other.m_data.clear();
        other.m_totalSize = 0;
        other.m_usedSize = 0;
        other.m_packetBeingCreated = 0;
        other.m_packets = 0;
      }

      CommandBuffer& operator=(CommandBuffer&& other) noexcept
      {
        m_data = std::move(other.m_data);
        m_totalSize = std::move(other.m_totalSize);
        m_usedSize = std::move(other.m_usedSize);
        m_packetBeingCreated = std::move(other.m_packetBeingCreated);
        m_packets = other.m_packets;
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "sanity check");
        other.m_data.clear();
        other.m_totalSize = 0;
        other.m_usedSize = 0;
        other.m_packetBeingCreated = 0;
        other.m_packets = 0;
        return *this;
      }

      size_t size() const
      {
        return m_packets;
      }

      size_t sizeBytes() const
      {
        return m_usedSize;
      }

      size_t maxSizeBytes() const
      {
        return m_data.size();
      }
      CommandBufferIterator begin()
      {
        return CommandBufferIterator(reinterpret_cast<PacketHeader*>(&m_data[0]));
      }

      CommandBufferIterator end()
      {
        return CommandBufferIterator(reinterpret_cast<PacketHeader*>(m_data.data()+m_packetBeingCreated));
      }

      void reset()
      {
        m_usedSize = 0;
        initialize();
      }

      template <typename Object>
      uint8_t* allocateElements2(size_t elements)
      {
        auto ptr = allocate(sizeof(Object) * elements);
        return ptr;
      }

      template <typename Object,typename PacketType>
      uint8_t* allocateElements(PacketVectorHeader<Object>& hdr, size_t elements, PacketType& packetBegin)
      {
        auto offsetWithinStruct = reinterpret_cast<size_t>(&hdr) - reinterpret_cast<size_t>(&packetBegin);
        auto actualHdrAddress = m_packetBeingCreated + sizeof(PacketHeader) + offsetWithinStruct;
        auto ptr = allocate(sizeof(Object) * elements);
        hdr.beginOffset = 0;
        hdr.elements = 0;
        if (ptr)
        {
          auto allocPtr = reinterpret_cast<size_t>(ptr);
          auto packetPtr = reinterpret_cast<size_t>(m_data.data()+actualHdrAddress);
          hdr.beginOffset = static_cast<uint32_t>(allocPtr - packetPtr); // how many bytes from packetvectorheader to start of allocPtr
          hdr.elements = static_cast<uint32_t>(elements);
        }
        return ptr;
      }

      template <typename Packet, typename... Args>
      void insert(Args&&... args)
      {
        static_assert(std::is_standard_layout<Packet>::value, "Packets have to be in standard layout...");
        //static_assert(std::is_trivially_copyable<Packet>::value, "Packets have to be trivially copyable..."); 
        auto hdr = beginNewPacket(Packet::type);
        auto ptr = allocate(sizeof(Packet));
        size_t offset = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_data.data());
        Packet packet = {};
        Packet::constructor(*this, packet, std::forward<Args>(args)...);
        memcpy(m_data.data()+offset, &packet, sizeof(Packet));
        endNewPacket(hdr);
      }

      template <typename Func>
      void foreach(Func&& func)
      {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(&m_data[0]);
    #if defined(SIZE_DEBUG)
        printf("%zu header info: %d %zu %u\n",reinterpret_cast<size_t>(header), header->type, header->length, header->offsetFromThis);
    #endif
        while (header->type != PacketType::EndOfPackets)
        {
          func(header->type);
          size_t nextHeaderAddr = reinterpret_cast<size_t>(header) + header->offsetFromThis;
          header = reinterpret_cast<PacketHeader*>(nextHeaderAddr);
    #if defined(SIZE_DEBUG)
          printf("%zu header info: %d %zu %u\n",reinterpret_cast<size_t>(header), header->type, header->length, header->offsetFromThis);
    #endif
        }
      }

      void append(const CommandBuffer& other)
      {
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "sanity check");
        auto newSize = m_data.size() + other.sizeBytes();
        m_data.resize(newSize);
#if VERIFY_PACKETS_ONE_BY_BY
        auto copy = m_data;
        for (int i = 0; i < m_usedSize; ++i)
        {
          HIGAN_ASSERT(copy[i] == m_data[i], "Data should be equal");
        }
#endif
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "Enforced EOP");
        memcpy(m_data.data()+m_packetBeingCreated, other.m_data.data(), other.sizeBytes());
#if VERIFY_PACKETS_ONE_BY_BY
        auto copyOther = other.m_data;
        for (int i = 0; i < other.m_usedSize; ++i)
        {
          auto offset = m_packetBeingCreated + i;
          HIGAN_ASSERT(copyOther[i] == m_data[offset], "Data should be equal");
        }
        for (int i = 0; i < m_packetBeingCreated; ++i)
        {
          HIGAN_ASSERT(copy[i] == m_data[i], "Data should be equal");
        }
#endif
        m_packetBeingCreated = m_packetBeingCreated + other.m_packetBeingCreated; 
        HIGAN_ASSERT(packetBeingCreated().type == PacketType::EndOfPackets, "Enforced EOP");
        m_usedSize += other.m_usedSize - sizeof(PacketHeader);
        m_totalSize += other.m_usedSize - sizeof(PacketHeader);
        m_packets += other.m_packets;
      }
    };
  }
}