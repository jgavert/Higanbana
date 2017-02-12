#pragma once
#include <string>
#include <array>

enum class CPUPageProperty
{
  Unknown,
  NotAvailable,
  WriteCombine,
  WriteBack
};

enum class HeapType
{
  Default,
  Upload,
  Readback,
  Custom
};

enum class MemoryPoolHint
{
  Host,
  Device
};

class HeapDescriptor
{
  std::string     m_name;
  CPUPageProperty m_cpuPage;
  HeapType        m_heapType;
  MemoryPoolHint  m_memPool;
  uint64_t        m_sizeInBytes;
  uint64_t        m_alignment;
  bool            m_onlyBuffers;
  bool            m_onlyNonRtDsTextures;
  bool            m_onlyRtDsTextures;
public:
  HeapDescriptor()
    : m_name("UnnamedHeap")
    , m_cpuPage(CPUPageProperty::Unknown)
    , m_heapType(HeapType::Default)
    , m_memPool(MemoryPoolHint::Device)
    , m_sizeInBytes(0)
    , m_alignment(64000)
    , m_onlyBuffers(false)
    , m_onlyNonRtDsTextures(false)
    , m_onlyRtDsTextures(false)
	{

	}

  HeapDescriptor& setName(std::string name)
  {
    m_name = name;
    return *this;
  }
  HeapDescriptor& setCPUPageProperty(CPUPageProperty prop)
  {
    m_cpuPage = prop;
    return *this;
  }
  HeapDescriptor& setHeapType(HeapType type)
  {
    m_heapType = type;
    return *this;
  }
  HeapDescriptor& setMemoryPoolHint(MemoryPoolHint pool)
  {
    m_memPool = pool;
    return *this;
  }
  HeapDescriptor& sizeInBytes(uint64_t size)
  {
    m_sizeInBytes = size;
    return *this;
  }
  HeapDescriptor& setHeapAlignment(int64_t alignment)
  {
    m_alignment = alignment;
    return *this;
  }
  HeapDescriptor& restrictOnlyBuffers()
  {
    m_onlyBuffers = true;
    return *this;
  }
  HeapDescriptor& restrictOnlyNonRtDsTextures()
  {
    m_onlyNonRtDsTextures = true;
    return *this;
  }
  HeapDescriptor& restrictOnlyRtDsTextures()
  {
    m_onlyRtDsTextures = true;
    return *this;
  }
};

