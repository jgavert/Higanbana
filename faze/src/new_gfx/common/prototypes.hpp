#pragma once
#include "resources.hpp"
#include "heap_descriptor.hpp"
#include "intermediatelist.hpp"
#include "core/src/system/memview.hpp"

#include <string>

namespace faze
{
  struct GpuInfo;
  class GpuDevice;
  class GraphicsSurface;
  namespace backend
  {
    struct GpuHeap;

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

      class GraphicsPipelineImpl
      {
      public:
        virtual ~GraphicsPipelineImpl() = default;
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

      class HeapImpl
      {
      public:
        virtual ~HeapImpl() = default;
      };

      class DeviceImpl
      {
      public:

        // utility
        virtual void waitGpuIdle() = 0;
        virtual MemoryRequirements getReqs(ResourceDescriptor desc) = 0;

        // swapchain
        virtual std::shared_ptr<SwapchainImpl> createSwapchain(GraphicsSurface& surface, PresentMode mode, FormatType format, int bufferCount) = 0;
        virtual void adjustSwapchain(std::shared_ptr<SwapchainImpl> sc, PresentMode mode, FormatType format, int bufferCount) = 0;
        virtual void destroySwapchain(std::shared_ptr<SwapchainImpl> sc) = 0;
        virtual vector<std::shared_ptr<TextureImpl>> getSwapchainTextures(std::shared_ptr<SwapchainImpl> sc) = 0;
        virtual int acquirePresentableImage(std::shared_ptr<prototypes::SwapchainImpl> swapchain) = 0;

        // pipeline related
        virtual std::shared_ptr<RenderpassImpl> createRenderpass() = 0;
        virtual std::shared_ptr<GraphicsPipelineImpl> createGraphicsPipeline(GraphicsPipelineDescriptor descriptor) = 0;

        //create/destroy pairs
        virtual GpuHeap createHeap(HeapDescriptor desc) = 0;
        virtual void destroyHeap(GpuHeap heap) = 0;

        virtual std::shared_ptr<BufferImpl> createBuffer(HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void destroyBuffer(std::shared_ptr<BufferImpl> buffer) = 0;
        virtual std::shared_ptr<BufferViewImpl> createBufferView(std::shared_ptr<BufferImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;
        virtual void destroyBufferView(std::shared_ptr<BufferViewImpl> buffer) = 0;

        virtual std::shared_ptr<TextureImpl> createTexture(HeapAllocation allocation, ResourceDescriptor& desc) = 0;
        virtual void destroyTexture(std::shared_ptr<TextureImpl> buffer) = 0;
        virtual std::shared_ptr<TextureViewImpl> createTextureView(std::shared_ptr<TextureImpl> buffer, ResourceDescriptor& desc, ShaderViewDescriptor& viewDesc) = 0;
        virtual void destroyTextureView(std::shared_ptr<TextureViewImpl> buffer) = 0;

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