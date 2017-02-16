#pragma once
#if defined(PLATFORM_WINDOWS)
#include "faze/src/new_gfx/common/prototypes.hpp"
#include "faze/src/new_gfx/common/resources.hpp"

#include "dx12.hpp"

namespace faze
{
  namespace backend
  {
    class DX12Fence
    {
    public:
      ComPtr<ID3D12Fence> fence = nullptr;
      std::shared_ptr<HANDLE> handle = nullptr;
      uint64_t value = 0;

      DX12Fence()
      {}

      DX12Fence(ComPtr<ID3D12Fence> fence)
        : fence(fence)
        , handle(std::shared_ptr<HANDLE>(new HANDLE(CreateEventExA(nullptr, nullptr, 0, EVENT_ALL_ACCESS)), [](HANDLE* ptr)
            {
              CloseHandle(*ptr);
              delete ptr;
            }))
      {
      }
      
      uint64_t start()
      {
        return ++value;
      }

      bool hasCompleted()
      {
        auto val = fence->GetCompletedValue();
        return val == value;
      }

      void waitTillReady(DWORD dwMilliseconds = INFINITE)
      {
        if (hasCompleted())
          return;
        fence->SetEventOnCompletion(value, *handle);
        DWORD result = WaitForSingleObject(*handle, dwMilliseconds);
        F_ASSERT(WAIT_OBJECT_0 == result, "Fence wait failed.");
      }
    };

    // implementations
    class DX12Buffer : public prototypes::BufferImpl
    {
    private:
      ID3D12Resource* resource;

    public:
      DX12Buffer()
      {}
      DX12Buffer(ID3D12Resource* resource)
        : resource(resource)
      {}
      ID3D12Resource* native()
      {
        return resource;
      }
    };

    class DX12Heap : public prototypes::HeapImpl
    {
    private:
      ID3D12Heap* heap;

    public:
      DX12Heap()
      {}
      DX12Heap(ID3D12Heap* heap)
        : heap(heap)
      {}
      ID3D12Heap* native()
      {
        return heap;
      }
    };

    class DX12Device : public prototypes::DeviceImpl
    {
    private:
      GpuInfo m_info;
      ComPtr<ID3D12Device> m_device;
      UINT m_nodeMask;
      ComPtr<ID3D12CommandQueue> m_graphicsQueue;
      ComPtr<ID3D12CommandQueue> m_dmaQueue;
      ComPtr<ID3D12CommandQueue> m_computeQueue;
      DX12Fence m_deviceFence;
    public:
      DX12Device(GpuInfo info, ComPtr<ID3D12Device> device);
      ~DX12Device();

      D3D12_RESOURCE_DESC fillBufferInfo(ResourceDescriptor descriptor);
      D3D12_RESOURCE_DESC fillTextureInfo(ResourceDescriptor descriptor);

      // impl
      void waitGpuIdle() override;
      MemoryRequirements getReqs(ResourceDescriptor desc) override;

      GpuHeap createHeap(HeapDescriptor desc) override;
      void destroyHeap(GpuHeap heap) override;
      backend::BufferData createBuffer(HeapAllocation allocation, ResourceDescriptor desc) override;
      void destroyBuffer(Buffer buffer) override;
      void createBufferView(ShaderViewDescriptor desc) override;
    };

    class DX12Subsystem : public prototypes::SubsystemImpl
    {
      vector<GpuInfo> infos;
      ComPtr<IDXGIFactory5> pFactory;
      std::vector<IDXGIAdapter3*> vAdapters;
    public:
      DX12Subsystem(const char* appName, unsigned appVersion, const char* engineName, unsigned engineVersion);
      std::string gfxApi() override;
      vector<GpuInfo> availableGpus() override;
      GpuDevice createGpuDevice(FileSystem& fs, GpuInfo gpu) override;
    };
  }
}
#endif