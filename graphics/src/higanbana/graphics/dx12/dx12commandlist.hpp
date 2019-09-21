#pragma once
#include "higanbana/graphics/dx12/dx12resources.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/common/command_packets.hpp"

namespace higanbana
{
  namespace backend
  {
    class DX12CommandList : public CommandBufferImpl
    {
      std::shared_ptr<DX12CommandBuffer> m_buffer;
      std::shared_ptr<DX12UploadHeap> m_constants;
      std::shared_ptr<DX12UploadHeap> m_upload;
      std::shared_ptr<DX12ReadbackHeap> m_readback;
      std::shared_ptr<DX12QueryHeap> m_queryheap;
      std::shared_ptr<DX12DynamicDescriptorHeap> m_descriptors;
      DX12CPUDescriptor m_nullBufferUAV;
      DX12CPUDescriptor m_nullBufferSRV;
      vector<DX12Query> queries;
      ReadbackBlock readback;

      UploadLinearAllocator m_constantsAllocator;
      LinearDescriptorAllocator m_descriptorAllocator;

      std::shared_ptr<FreeableResources> m_freeResources;

      UploadBlock allocateConstants(size_t size);
      DynamicDescriptorBlock allocateDescriptors(size_t size);
      void handleBindings(DX12Device* dev, D3D12GraphicsCommandList*, gfxpacket::ResourceBinding& binding);
      void addDepedencyDataAndSolve(DX12DependencySolver* solver, backend::IntermediateList& list);
      void addCommands(DX12Device* device, D3D12GraphicsCommandList* buffer, backend::CommandBuffer& list, BarrierSolver& solver);
      void processRenderpasses(DX12Device* dev, backend::CommandBuffer& list);

    public:
      DX12CommandList(
        std::shared_ptr<DX12CommandBuffer> buffer,
        std::shared_ptr<DX12UploadHeap> constants,
        std::shared_ptr<DX12UploadHeap> dynamicUpload,
        std::shared_ptr<DX12ReadbackHeap> readback,
        std::shared_ptr<DX12QueryHeap> queryheap,
        std::shared_ptr<DX12DynamicDescriptorHeap> descriptors,
        DX12CPUDescriptor nullBufferUAV,
        DX12CPUDescriptor nullBufferSRV);

      void fillWith(std::shared_ptr<prototypes::DeviceImpl>, backend::CommandBuffer&, BarrierSolver& solver) override;
      void readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl>, vector<GraphNodeTiming>& nodes) override;

      bool closed() const
      {
        return m_buffer->closed();
      }

      ID3D12GraphicsCommandList* list()
      {
        return m_buffer->list();
      }
    };
  }
}
#endif