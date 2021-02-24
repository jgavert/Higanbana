#pragma once

namespace higanbana
{
namespace desc
{
  class RaytracingAccelerationStructureInputs;
}

  struct GpuInfo;

  struct MemoryRequirements;

  struct GpuHeapAllocation;
  class FixedSizeAllocator;

  class SwapchainDescriptor;
  class ResourceDescriptor;
  class ShaderViewDescriptor;
  class ComputePipelineDescriptor;
  class ComputePipeline;
  class GraphicsPipelineDescriptor;
  class GraphicsPipeline;
  class RaytracingPipelineDescriptor;
  enum class FormatType;

  struct ResourceHandle;

  class Buffer;
  class SharedBuffer;
  class Texture;
  class SharedTexture;

  class BufferSRV;
  class BufferUAV;
  class BufferIBV;
  class AccelerationStructure;
  class TextureSRV;
  class TextureUAV;
  class TextureRTV;
  class TextureDSV;
  class DynamicBufferView;
  class ReadbackData;

  class ShaderArgumentsLayoutDescriptor;
  class ShaderArgumentsLayout;
  class ShaderArguments;

  class Swapchain;
  class Renderpass;
  class GraphicsSurface;
  class Window;
  class CommandGraph;
  class CpuImage;

  class GpuSemaphore;
  class GpuSharedSemaphore;

  class HeapDescriptor;
  struct ResourceState;
  struct TextureResourceState;

  struct ReadbackPromise;
  class ReadbackFuture;

  class CommandGraph;
  class CommandGraphNode;

  namespace backend
  {
    class FenceImpl;
    class SemaphoreImpl;
    class CommandBufferImpl;
    class IntermediateList;
    class CommandBuffer;
    class BarrierSolver;
    namespace prototypes
    {
      class DeviceImpl;
      class SubsystemImpl;
      class HeapImpl;
      class BufferImpl;
      class TextureImpl;
      class BufferViewImpl;
      class TextureViewImpl;
      class SwapchainImpl;
      class GraphicsSurfaceImpl;
    }

    struct SharedHandle;

    struct GpuHeap;
    struct HeapAllocation;
    class HeapManager;

    struct QueueStates;
  }
}