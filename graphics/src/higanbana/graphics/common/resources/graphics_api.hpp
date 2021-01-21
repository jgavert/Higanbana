#pragma once

#include <cstdint>

namespace higanbana
{
  enum class GraphicsApi
  {
	  All,
    Vulkan,
    DX12,
  };

  const char* toString(GraphicsApi api);
  enum class QueryDevicesMode
  {
    UseCached,
    SlowQuery
  };

  enum class DeviceType
  {
    Unknown,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu
  };

  enum class VendorID
  {
    Unknown,
    All,
    Amd, // = 4098,
    Nvidia, // = 4318,
    Intel // = 32902
  };
  const char* toString(VendorID vendor);

  enum QueueType : uint32_t
  {
    Unknown = 0,
    Dma = 0x1,
    Compute = 0x2 | QueueType::Dma,
    Graphics = 0x4 | QueueType::Compute,
    External = 0x8
  };
  const char* toString(QueueType queue);

  enum class ThreadedSubmission
  {
    Sequenced,
    Parallel,
    ParallelUnsequenced,
    Unsequenced
  };

  enum class PresentMode
  {
    Unknown,
    Immediate, // Fullscreen Immediate - ?? Use for Windowed
    Mailbox, // Fullscreen Mailbox - hack ?? also suggested for Windowed
    Fifo, // fallback for Relaxed
    FifoRelaxed // Fullscreen V-Sync
  };

  enum class Colorspace
  {
    BT709,
    BT2020
  };

  enum DisplayCurve
  {
    sRGB = 0,	// The display expects an sRGB signal.
    ST2084,		// The display expects an HDR10 signal.
    None		// The display expects a linear signal.
  };

  const char* presentModeToStr(PresentMode mode);
  const char* colorspaceToStr(Colorspace mode);
  const char* displayCurveToStr(DisplayCurve mode);
  
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
}