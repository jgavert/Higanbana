#pragma once
#include "resources.hpp"
#include "heap_descriptor.hpp"
#include "intermediatelist.hpp"
#include <string>

namespace faze
{
  struct GpuInfo;
  class GpuDevice;
  class GraphicsSurface;
  namespace backend
  {
    struct GpuHeap;

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
        virtual ResourceDescriptor desc() = 0;
        virtual ~SwapchainImpl() = default;
      };

      class TextureImpl
      {
      public:
        virtual ~TextureImpl() = default;
      };
      class TextureViewImpl
      {
      public:
        virtual ~TextureViewImpl() = default;
      };
      class BufferImpl
      {
      public:
        virtual ~BufferImpl() = default;
      };
      class BufferViewImpl
      {
      public:
        virtual ~BufferViewImpl() = default;
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

        // commandlist things
        virtual void submit(backend::IntermediateList& list) = 0;
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