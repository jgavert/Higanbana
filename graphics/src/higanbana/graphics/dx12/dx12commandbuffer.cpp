#include "higanbana/graphics/dx12/dx12resources.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/util/formats.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"

#if 0
#undef PROFILE
#define PROFILE
#include <WinPixEventRuntime/pix3.h>
#undef PROFILE
#endif

#include <algorithm>

namespace higanbana
{
  namespace backend
  {
    DX12CommandBuffer::DX12CommandBuffer(ComPtr<D3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> commandListAllocator, bool isDma)
      : commandList(commandList)
      , commandListAllocator(commandListAllocator)
      , dmaList(isDma)
    {
    }

    D3D12GraphicsCommandList* DX12CommandBuffer::list()
    {
      return commandList.Get();
    }

    void DX12CommandBuffer::closeList()
    {
      commandList->Close();
      closedList = true;
    }

    void DX12CommandBuffer::resetList()
    {
      if (!closedList)
        commandList->Close();
      commandListAllocator->Reset();
      commandList->Reset(commandListAllocator.Get(), nullptr);
      closedList = false;
    }

    bool DX12CommandBuffer::closed() const
    {
      return closedList;
    }

    bool DX12CommandBuffer::dma() const
    {
      return dmaList;
    }
  }
}
#endif