#pragma once
#include "higanbana/graphics/common/heap_descriptor.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"
#include "higanbana/graphics/common/pipeline_descriptor.hpp"
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/datastructures/proxy.hpp>
#include <string>
#include <memory>

namespace higanbana
{
  struct GpuInfo;
  class GpuDevice;
  class GraphicsSurface;

  // descriptors
  class SwapchainDescriptor;
  class ResourceDescriptor;
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

    class SemaphoreImpl
    {
    public:
      virtual ~SemaphoreImpl() = default;
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
      virtual void fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::CommandBuffer& list, BarrierSolver& solver) = 0;
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
        virtual ~SwapchainImpl() = default;
      };

      // questionable
      /*
      class DynamicBufferViewImpl
      {
      public:
        virtual ~DynamicBufferViewImpl() = default;
        virtual backend::RawView view() = 0;
        virtual int rowPitch() = 0;
        virtual uint64_t offset() = 0;
        virtual uint64_t size() = 0;
      };
      */

      class DeviceImpl
      {
      public:

        // utility
        virtual void collectTrash() = 0;
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

        //create/destroy pairs
        virtual void createHeap(ResourceHandle handle, HeapDescriptor desc) = 0;
  
        virtual void createBuffer(ResourceHandle handle, ResourceDescriptor& desc) = 0;
        virtual void createBuffer(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void createBufferView(ViewResourceHandle handle, ResourceHandle buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;
        virtual void createTexture(ResourceHandle handle, ResourceDescriptor& desc) = 0;
        virtual void createTexture(ResourceHandle handle, HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void createTextureView(ViewResourceHandle handle, ResourceHandle texture, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;

        virtual std::shared_ptr<backend::SemaphoreImpl> createSharedSemaphore() = 0;

        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(std::shared_ptr<backend::SemaphoreImpl> semaphore) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(HeapAllocation allocation) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(ResourceHandle resource) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openForInteropt(ResourceHandle resource) = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<backend::SharedHandle> handle) = 0;
        virtual void createHeapFromHandle(ResourceHandle handle, std::shared_ptr<SharedHandle> shared) = 0;
        virtual void createBufferFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, HeapAllocation heapAllocation, ResourceDescriptor& desc) = 0;
        virtual void createTextureFromHandle(ResourceHandle handle, std::shared_ptr<backend::SharedHandle> shared, ResourceDescriptor& desc) = 0;

        // create dynamic resources
        virtual void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, FormatType format) = 0;
        virtual void dynamic(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned stride) = 0;
        virtual void dynamicImage(ViewResourceHandle handle, MemView<uint8_t> bytes, unsigned rowPitch) = 0;

        // commandlist things and gpu-cpu/gpu-gpu synchronization primitives
        virtual std::shared_ptr<backend::CommandBufferImpl> createDMAList() = 0;
        virtual std::shared_ptr<backend::CommandBufferImpl> createComputeList() = 0;
        virtual std::shared_ptr<backend::CommandBufferImpl> createGraphicsList() = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl>     createSemaphore() = 0;
        virtual std::shared_ptr<backend::FenceImpl>         createFence() = 0;

        virtual void submitDMA(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<std::shared_ptr<backend::FenceImpl>> fence) = 0;

        virtual void submitCompute(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<std::shared_ptr<backend::FenceImpl>> fence) = 0;

        virtual void submitGraphics(
          MemView<std::shared_ptr<backend::CommandBufferImpl>> lists,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> wait,
          MemView<std::shared_ptr<backend::SemaphoreImpl>> signal,
          MemView<std::shared_ptr<backend::FenceImpl>> fence) = 0;

        virtual void waitFence(std::shared_ptr<backend::FenceImpl> fence) = 0;
        virtual bool checkFence(std::shared_ptr<backend::FenceImpl> fence) = 0;

        virtual void present(std::shared_ptr<SwapchainImpl> swapchain, std::shared_ptr<backend::SemaphoreImpl> renderingFinished) = 0;
      };

      class SubsystemImpl
      {
      public:
        virtual std::string gfxApi() = 0;
        virtual vector<GpuInfo> availableGpus() = 0;
        virtual std::shared_ptr<prototypes::DeviceImpl> createGpuDevice(FileSystem& fs, GpuInfo gpu) = 0;
        virtual GraphicsSurface createSurface(Window& window) = 0;
        virtual ~SubsystemImpl() {}
      };
    }
  }
}