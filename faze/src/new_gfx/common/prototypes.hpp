#pragma once
#include "heap_descriptor.hpp"
#include "core/src/system/memview.hpp"
#include "core/src/datastructures/proxy.hpp"

#include "faze/src/new_gfx/common/resource_descriptor.hpp"
#include "faze/src/new_gfx/common/pipeline_descriptor.hpp"
#include "faze/src/new_gfx/common/descriptor_layout.hpp"

#include <string>
#include <memory>

namespace faze
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

  namespace backend
  {
    struct SharedHandle;
    struct TrackedState;
    struct RawView;
    struct GpuHeap;
    class IntermediateList;
    struct HeapAllocation;

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
      virtual void fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::IntermediateList& list) = 0;
      virtual ~CommandBufferImpl() = default;
    };

    namespace prototypes
    {
      class RenderpassImpl
      {
      public:
        virtual ~RenderpassImpl() = default;
      };

      class PipelineImpl
      {
      public:
        virtual ~PipelineImpl() = default;
      };

      class DescriptorLayoutImpl
      {
      public:
        virtual ~DescriptorLayoutImpl() = default;
      };

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

      class TextureImpl
      {
      public:
        virtual ~TextureImpl() = default;
        virtual backend::TrackedState dependency() = 0;
      };
      class TextureViewImpl
      {
      public:
        virtual ~TextureViewImpl() = default;
        virtual backend::RawView view() = 0;
      };
      class BufferImpl
      {
      public:
        virtual ~BufferImpl() = default;
        virtual backend::TrackedState dependency() = 0;
      };
      class BufferViewImpl
      {
      public:
        virtual ~BufferViewImpl() = default;
        virtual backend::RawView view() = 0;
      };

      class DynamicBufferViewImpl
      {
      public:
        virtual ~DynamicBufferViewImpl() = default;
        virtual backend::RawView view() = 0;
        virtual int rowPitch() = 0;
        virtual uint64_t offset() = 0;
        virtual uint64_t size() = 0;
      };

      class HeapImpl
      {
      public:
        virtual ~HeapImpl() = default;
      };

      class DeviceImpl
      {
      public:

        // utility
        virtual void collectTrash() = 0;
        virtual void waitGpuIdle() = 0;
        virtual MemoryRequirements getReqs(ResourceDescriptor desc) = 0;

        // swapchain
        virtual std::shared_ptr<SwapchainImpl> createSwapchain(GraphicsSurface& surface, SwapchainDescriptor descriptor) = 0;
        virtual void adjustSwapchain(std::shared_ptr<SwapchainImpl> sc, SwapchainDescriptor descriptor) = 0;
        virtual vector<std::shared_ptr<TextureImpl>> getSwapchainTextures(std::shared_ptr<SwapchainImpl> sc) = 0;
        virtual int tryAcquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) = 0;
        virtual int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) = 0;

        // pipeline related
        virtual std::shared_ptr<RenderpassImpl> createRenderpass() = 0;
        virtual std::shared_ptr<PipelineImpl> createPipeline() = 0;

        // descriptor stuff
        virtual std::shared_ptr<DescriptorLayoutImpl> createDescriptorLayout(GraphicsPipelineDescriptor desc) = 0;
        virtual std::shared_ptr<DescriptorLayoutImpl> createDescriptorLayout(ComputePipelineDescriptor desc) = 0;

        //create/destroy pairs
        virtual GpuHeap createHeap(HeapDescriptor desc) = 0;

        virtual std::shared_ptr<BufferImpl> createBuffer(ResourceDescriptor& desc) = 0;
        virtual std::shared_ptr<BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual std::shared_ptr<BufferViewImpl> createBufferView(std::shared_ptr<BufferImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;
        virtual std::shared_ptr<TextureImpl> createTexture(ResourceDescriptor& desc) = 0;
        virtual std::shared_ptr<TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual std::shared_ptr<TextureViewImpl> createTextureView(std::shared_ptr<TextureImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;

        virtual std::shared_ptr<backend::SemaphoreImpl> createSharedSemaphore() = 0;

        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(std::shared_ptr<backend::SemaphoreImpl> semaphore) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(HeapAllocation allocation) = 0;
        virtual std::shared_ptr<backend::SharedHandle> openSharedHandle(std::shared_ptr<TextureImpl> resource) = 0;
        virtual std::shared_ptr<backend::SemaphoreImpl> createSemaphoreFromHandle(std::shared_ptr<backend::SharedHandle> handle) = 0;
        virtual std::shared_ptr<BufferImpl> createBufferFromHandle(std::shared_ptr<backend::SharedHandle> handle, HeapAllocation heapAllocation, ResourceDescriptor& desc) = 0;
        virtual std::shared_ptr<TextureImpl> createTextureFromHandle(std::shared_ptr<backend::SharedHandle> handle, ResourceDescriptor& desc) = 0;

        // create dynamic resources
        virtual std::shared_ptr<DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, FormatType format) = 0;
        virtual std::shared_ptr<DynamicBufferViewImpl> dynamic(MemView<uint8_t> bytes, unsigned stride) = 0;

        virtual std::shared_ptr<DynamicBufferViewImpl> dynamicImage(MemView<uint8_t> bytes, unsigned rowPitch) = 0;

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
        virtual GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu) = 0;
        virtual GraphicsSurface createSurface(Window& window) = 0;
        virtual ~SubsystemImpl() {}
      };
    }
  }
}