#pragma once
#include <cstdio>
#include <memory>
#include <cstddef>
#include <cstring>
#include <vector>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <higanbana/core/global_debug.hpp>

// #define SIZE_DEBUG
namespace higanbana
{
  namespace backend
  {
    enum PacketType : uint32_t
    {
      EndOfPackets = 0,
      RenderBlock,
      ReleaseFromQueue,
      BufferCopy,
      Dispatch,
      PrepareForPresent,
      RenderpassBegin,
      RenderpassEnd,
      GraphicsPipelineBind,
      ComputePipelineBind,
      ResourceBinding,
      Draw,
      DrawIndexed,
      Count
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
          return *reinterpret_cast<T*>(this + 1);
        }
      };
    private:

      size_t m_packets = 0;
      PacketHeader* m_packetBeingCreated = nullptr;

      uint8_t* allocate(size_t size)
      {
        auto current = m_usedSize;
        m_usedSize += size;
        if (m_usedSize > m_totalSize)
          return nullptr;
        //printf("allocated %zu size %zu\n", current, size);
        return &m_data[current];
      }

      void beginNewPacket(PacketType type)
      {
        // patch current EOP to be actual packet header.
        m_packetBeingCreated->type = type;
      }

      void newHeader()
      {
        auto ptr = allocate(sizeof(PacketHeader));
        m_packetBeingCreated = reinterpret_cast<PacketHeader*>(ptr);
        m_packetBeingCreated->type = static_cast<PacketType>(0);
    #if defined(SIZE_DEBUG)
        m_packetBeingCreated->length = 0;
    #endif
        m_packetBeingCreated->offsetFromThis = 0;
      }

      void endNewPacket()
      {
        // patch old packet
        size_t currentTop = reinterpret_cast<size_t>(&m_data[m_usedSize]);
        size_t thisPacket = reinterpret_cast<size_t>(m_packetBeingCreated);
        size_t diff = currentTop - thisPacket;
        m_packetBeingCreated->offsetFromThis = static_cast<unsigned>(diff);
    #if defined(SIZE_DEBUG)
        m_packetBeingCreated->length = m_usedSize - m_packetBeingCreated->length;
    #endif
        m_packets++;
        // create new EOP
        newHeader();
      }

      void initialize()
      {
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
        /*
        CommandBufferIterator operator++(int)
        {
          CommandBufferIterator tmp(*this);
          operator++();
          return tmp;
        }*/

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
      }

      CommandBuffer(const CommandBuffer& other)
        : m_data(other.m_data)
        , m_totalSize(other.m_totalSize)
        , m_usedSize(other.m_usedSize)
        , m_packetBeingCreated(reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]))
        , m_packets(other.m_packets)
      {
      }

      CommandBuffer(CommandBuffer&& other)
        : m_data(std::move(other.m_data))
        , m_totalSize(std::move(other.m_totalSize))
        , m_usedSize(std::move(other.m_usedSize))
        , m_packetBeingCreated(reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]))
        , m_packets(other.m_packets)
      {
        other.m_data.clear();
        other.m_totalSize = 0;
        other.m_usedSize = 0;
        other.m_packetBeingCreated = nullptr;
        other.m_packets = 0;
      }

      CommandBuffer& operator=(const CommandBuffer& other)
      {
        m_data = other.m_data;
        m_totalSize = other.m_totalSize;
        m_usedSize = other.m_usedSize;
        m_packetBeingCreated = reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]);
        m_packets = other.m_packets;
        return *this;
      }

      CommandBuffer& operator=(CommandBuffer&& other)
      {
        m_data = std::move(other.m_data);
        m_totalSize = std::move(other.m_totalSize);
        m_usedSize = std::move(other.m_usedSize);
        m_packetBeingCreated = reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]);
        m_packets = other.m_packets;
        other.m_data.clear();
        other.m_totalSize = 0;
        other.m_usedSize = 0;
        other.m_packetBeingCreated = nullptr;
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
        return CommandBufferIterator(m_packetBeingCreated);
      }

      void reset()
      {
        m_usedSize = 0;
        initialize();
      }

      template <typename Object>
      void allocateElements(PacketVectorHeader<Object>& header, size_t elements)
      {
        auto ptr = allocate(sizeof(Object) * elements);
        header = PacketVectorHeader<Object>{};
        header.beginOffset = 0;
        header.elements = 0;
        if (ptr)
        {
          header.beginOffset = static_cast<uint32_t>(reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(&header));
          header.elements = static_cast<uint32_t>(elements);
        }
      }

      template <typename Packet, typename... Args>
      void insert(Args&&... args)
      {
        beginNewPacket(Packet::type);
        auto ptr = allocate(sizeof(Packet));
        Packet* packet = reinterpret_cast<Packet*>(ptr);
        Packet::constructor(*this, packet, std::forward<Args>(args)...);
        endNewPacket();
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
        PacketHeader* correctLastPacket = reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]);
        F_ASSERT(m_packetBeingCreated == correctLastPacket, "sanity check");
        auto newSize = m_data.size() + other.sizeBytes();
        m_data.resize(newSize);
        // find new end.. since we just resized and invalidated all pointers.
        m_packetBeingCreated = reinterpret_cast<PacketHeader*>(&m_data[m_usedSize - sizeof(PacketHeader)]);
        F_ASSERT(m_packetBeingCreated->type == PacketType::EndOfPackets, "Enforced EOP");
        memcpy(m_packetBeingCreated, other.m_data.data(), other.sizeBytes());
        m_packetBeingCreated = reinterpret_cast<PacketHeader*>(&m_data[m_usedSize + other.m_usedSize - sizeof(PacketHeader) - sizeof(PacketHeader)]);
        F_ASSERT(m_packetBeingCreated->type == PacketType::EndOfPackets, "Enforced EOP");
        m_usedSize += other.m_usedSize - sizeof(PacketHeader);
        m_totalSize += other.m_usedSize - sizeof(PacketHeader);
        m_packets += other.m_packets;
      }
    };


    /*
    struct sample_packet
    {
      int secretData1;
      int secretData2;
      bool importantBoolean;

      static constexpr const PacketType type = PacketType::SP_PACKET;

      static void constructor(CommandBuffer&, sample_packet* packet, int a, int b, bool c)
      {
        packet->secretData1 = a;
        packet->secretData2 = b;
        packet->importantBoolean = c;
      }
    };

    struct sample_vectorPacket
    {
      // vectors of stuff?????????????
      PacketVectorHeader<int> manyInts; /// ???
      PacketVectorHeader<int> differentInts; /// ???

      // constructors
      static constexpr const PacketType type = PacketType::SP_VECTORPACKET;
      static void constructor(CommandBuffer& buffer, sample_vectorPacket* packet, MemView<int> a, MemView<int> b)
      {
        buffer.allocateElements<int>(packet->manyInts, a.size());
        auto spn = packet->manyInts.convertToMemView();
        memcpy(spn.data(), a.data(), a.size_bytes());

        buffer.allocateElements<int>(packet->differentInts, b.size());
        spn = packet->manyInts.convertToMemView();
        memcpy(spn.data(), b.data(), b.size_bytes());
      }
    };
    */
  }
}