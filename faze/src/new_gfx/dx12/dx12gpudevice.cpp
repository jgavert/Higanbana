#if defined(PLATFORM_WINDOWS)
#include "dx12resources.hpp"

#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12Device::DX12Device(GpuInfo info, ComPtr<ID3D12Device> device)
      : m_info(info)
      , m_device(device)
      , m_nodeMask(1 << m_info.id)
    {
      D3D12_COMMAND_QUEUE_DESC desc{};
      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
      desc.NodeMask = m_nodeMask;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_graphicsQueue.GetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_dmaQueue.GetAddressOf()));

      desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
      desc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
      desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
      m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(m_computeQueue.GetAddressOf()));

      ComPtr<ID3D12Fence> fence;
      m_device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
      m_deviceFence = DX12Fence(fence);
    }

    DX12Device::~DX12Device()
    {
      waitGpuIdle();
      /*
      ComPtr<ID3D12DebugDevice> debugInterface;
      m_device.As(&debugInterface);
      debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
      */
    }

    void DX12Device::waitGpuIdle()
    {
      m_graphicsQueue->Signal(m_deviceFence.fence.Get(), m_deviceFence.start());
      m_deviceFence.waitTillReady();
    }

    GpuHeap DX12Device::createHeap(HeapDescriptor heapDesc)
    {
      auto desc = heapDesc.desc;
      D3D12_HEAP_DESC dxdesc{};



      ID3D12Heap* heap;
      m_device->CreateHeap(&dxdesc, IID_PPV_ARGS(&heap));
      return GpuHeap(std::make_shared<DX12Heap>(heap), std::move(heapDesc));
    }

    void DX12Device::destroyHeap(GpuHeap heap)
    {
      auto native = std::static_pointer_cast<DX12Heap>(heap.impl);
      native->native()->Release();
    }

    void DX12Device::createBuffer(ResourceDescriptor )
    {

    }

    void DX12Device::createBufferView(ShaderViewDescriptor )
    {

    }
  }
}
#endif