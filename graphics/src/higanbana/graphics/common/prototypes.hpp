#pragma once
#include "higanbana/graphics/common/resources.hpp"
#include "higanbana/graphics/common/resources/graphics_api.hpp"
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/datastructures/vector.hpp>
#include <string>
#include <memory>
#include <optional>

namespace higanbana
{
  struct GraphNodeTiming;
  struct GpuInfo;
  class GpuDevice;
  class GraphicsSurface;
  struct DeviceStatistics;

  // descriptors
  class ShaderArgumentsDescriptor;
  class SwapchainDescriptor;
  class ResourceDescriptor;

namespace desc
{
  class RaytracingAccelerationStructureInputs;
  struct RaytracingASPreBuildInfo;
}

  class Binding;
  class FileSystem;
  class Window;
  struct MemoryRequirements;
  struct ResourceHandle;
  struct ViewResourceHandle;
  class HandleManager;

  namespace backend
  {
    struct SharedHandle;
    struct TrackedState;
    struct RawView;
    struct GpuHeap;
    class CommandBuffer;
    struct HeapAllocation;
    class BarrierSolver;

    struct ConstantsBlock
    {
      uint64_t constantBlock = 0;
      uint8_t* ptr = nullptr;
      bool valid() const { return ptr != nullptr;}
    };

    class LinearConstantsAllocator
    {
    public:
      virtual ConstantsBlock allocate(size_t) = 0;
      virtual ~LinearConstantsAllocator() = default;
    };

    class ConstantsAllocator
    {
    public:
      virtual LinearConstantsAllocator* allocate(size_t) = 0;
      virtual void free(LinearConstantsAllocator*) = 0;
      virtual size_t size() = 0;
      virtual size_t max_size() = 0;
      virtual size_t size_allocated() = 0;
      virtual ~ConstantsAllocator() = default;
    };

    class SemaphoreImpl
    {
    public:
      virtual ~SemaphoreImpl() = default;
    };

    class TimelineSemaphoreImpl
    {
    public:
      virtual ~TimelineSemaphoreImpl() = default;
    };

    struct TimelineSemaphoreInfo
    {
      TimelineSemaphoreImpl* semaphore;
      uint64_t value;
    };

    class FenceImpl
    {
    public:
      virtual ~FenceImpl() = default;
    };

    namespace prototypes
    {
      class DeviceImpl;
    }

    class CommandBufferImpl
    {
    public:
      virtual void reserveConstants(size_t expectedTotalBytes) = 0;
      virtual void fillWith(std::shared_ptr<prototypes::DeviceImpl> device, higanbana::MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver) = 0;
      virtual bool readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl> device, vector<GraphNodeTiming>& nodes) = 0;

      virtual void beginConstantsDmaList(std::shared_ptr<prototypes::DeviceImpl> device) = 0;
      virtual void addConstants(CommandBufferImpl* list) = 0;
      virtual void endConstantsDmaList() = 0;
      virtual ~CommandBufferImpl() = default;
    };

    namespace prototypes
    {
      class GraphicsSurfaceImpl
      {
      public:
        virtual ~GraphicsSurfaceImpl() = default;
      };

      class SwapchainImpl
      {
      public:
        virtual int getCurrentPresentableImageIndex() = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl> acquireSemaphore() = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl> renderSemaphore() = 0;
        virtual ResourceDescriptor desc() = 0;
        virtual bool HDRSupport() = 0;
        virtual DisplayCurve displayCurve() = 0;
        virtual bool outOfDate() = 0;
        virtual ~SwapchainImpl() = default;
      };

      class DeviceImpl
      {
      public:
        // statistics
        virtual DeviceStatistics statsOfResourcesInUse() = 0;

        // utility
        virtual void waitGpuIdle() = 0;
        virtual MemoryRequirements getReqs(ResourceDescriptor desc) = 0;

        // swapchain
        virtual std::shared_ptr<SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) = 0;
        virtual void adjustSwapchain(std::shared_ptr<prototypes::SwapchainImpl> sc, SwapchainDescriptor descriptor) = 0;
        virtual int fetchSwapchainTextures(std::shared_ptr<prototypes::SwapchainImpl> sc, vector<ResourceHandle>& handles) = 0;
        virtual int tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) = 0;
        virtual int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) = 0;

        virtual void releaseHandle(ResourceHandle handle) = 0;
        virtual void releaseViewHandle(ViewResourceHandle handle) = 0;
        // pipeline related
        virtual void createRenderpass(ResourceHandle handle) = 0;
        virtual void createPipeline(ResourceHandle handle, GraphicsPipelineDescriptor desc) = 0;
        virtual void createPipeline(ResourceHandle handle, ComputePipelineDescriptor desc) = 0;
        virtual void createPipeline(ResourceHandle handle, RaytracingPipelineDescriptor desc) = 0;

        //create/destroy pairs
        virtual void createHeap(ResourceHandle handle, HeapDescriptor desc) = 0;
  
        virtual void createBuffer(ResourceHandle handle, ResourceDescriptor& desc) = 0;
        virtual void createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;
        virtual void createTexture(ResourceHandle handle, ResourceDescriptor& desc) = 0;
        virtual void createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void createTextureView(ViewResourceHandle handle, ResourceHandle texture, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;

        // descriptors sets or ShaderArguments
        virtual void createShaderArgumentsLayout(ResourceHandle handle, ShaderArgumentsLayoutDescriptor& desc) = 0;
        virtual void createShaderArguments(ResourceHandle handle, ShaderArgumentsDescriptor& binding) = 0;

        virtual std::shared_ptr<backend::TimelineSemaphoreImpl> createSharedSemaphore() = 0;

        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(std::shared_ptr<backend::TimelineSemaphoreImpl> semaphore) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(HeapAllocation allocation) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(ResourceHandle resource) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openForInteropt(ResourceHandle resource) = 0;
        virtual std::shared_ptr<backend::TimelineSemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<backend::SharedHandle> handle) = 0;
        virtual void createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared) = 0;
        virtual void createBufferFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, HeapAllocation heapAllocation, ResourceDescriptor& desc) = 0;
        virtual void createTextureFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, ResourceDescriptor& desc) = 0;

        // create dynamic resources
        virtual size_t availableDynamicMemory() = 0;
        virtual void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, FormatType format) = 0;
        virtual void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned stride) = 0;
        virtual void dynamicImage(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned rowPitch) = 0;

        // create readback resources
        virtual void readbackBuffer(ResourceHandle readback, size_t bytes) = 0;
        virtual MemView<uint8_t> mapReadback(ResourceHandle readback) = 0;
        virtual void unmapReadback(ResourceHandle readback) = 0;

        // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
        virtual std::shared_ptr<backend::CommandBufferImpl>     createDMAList() = 0;
        virtual std::shared_ptr<backend::CommandBufferImpl>     createComputeList() = 0;
        virtual std::shared_ptr<backend::CommandBufferImpl>     createGraphicsList() = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl>         createSemaphore() = 0;
        virtual std::shared_ptr<backend::FenceImpl>             createFence() = 0;
        virtual std::shared_ptr<backend::TimelineSemaphoreImpl> createTimelineSemaphore() = 0;
        virtual std::shared_ptr<backend::ConstantsAllocator>    createConstantsAllocator(size_t size) = 0;

        virtual void submitDMA(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<TimelineSemaphoreInfo> waitTimelines,
          MemView<TimelineSemaphoreInfo> signaltimelines,
          std::optional<std::shared_ptr<FenceImpl>> fence) = 0;

        virtual void submitCompute(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<TimelineSemaphoreInfo> waitTimelines,
          MemView<TimelineSemaphoreInfo> signaltimelines,
          std::optional<std::shared_ptr<FenceImpl>> fence) = 0;

        virtual void submitGraphics(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<TimelineSemaphoreInfo> waitTimelines,
          MemView<TimelineSemaphoreInfo> signaltimelines,
          std::optional<std::shared_ptr<FenceImpl>> fence) = 0;

        virtual void waitFence(std::shared_ptr<backend::FenceImpl> fence) = 0;
        virtual bool checkFence(std::shared_ptr<backend::FenceImpl> fence) = 0;
        virtual uint64_t completedValue(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema) = 0;
        virtual void waitTimeline(std::shared_ptr<backend::TimelineSemaphoreImpl> tlSema, uint64_t value) = 0;

        virtual void present(std::shared_ptr<SwapchainImpl> swapchain, std::shared_ptr<backend::SemaphoreImpl> renderingFinished, int index) = 0;

        // raytracing
        virtual desc::RaytracingASPreBuildInfo accelerationStructurePrebuildInfo(const desc::RaytracingAccelerationStructureInputs& desc) = 0;
      };

      class SubsystemImpl
      {
      public:
        virtual std::string gfxApi() = 0;
        virtual vector<GpuInfo> availableGpus(VendorID vendor) = 0;
        virtual std::shared_ptr<prototypes::DeviceImpl> createGpuDevice(FileSystem& fs, GpuInfo gpu) = 0;
        virtual GraphicsSurface createSurface(Window& window) = 0;
        virtual ~SubsystemImpl() {}
      };
    }
  }
}