#include "graphics/dx12/dx12resources.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "graphics/dx12/util/dxDependencySolver.hpp"
#include "graphics/dx12/util/formats.hpp"
#include <graphics/common/texture.hpp>
#include <graphics/common/command_buffer.hpp>

#if 0
#undef PROFILE
#define PROFILE
#include <WinPixEventRuntime/pix3.h>
#undef PROFILE
#endif

#include <algorithm>

namespace faze
{
  namespace backend
  {
    DX12CommandBuffer::DX12CommandBuffer(ComPtr<D3D12GraphicsCommandList> commandList, ComPtr<ID3D12CommandAllocator> commandListAllocator, bool isDma)
      : commandList(commandList)
      , commandListAllocator(commandListAllocator)
      , m_solver(std::make_shared<DX12DependencySolver>())
      , dmaList(isDma)
    {
    }

    D3D12GraphicsCommandList* DX12CommandBuffer::list()
    {
      return commandList.Get();
    }

    DX12DependencySolver* DX12CommandBuffer::solver()
    {
      return m_solver.get();
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
      m_solver->reset();
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
/*
    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::RenderpassBegin& packet)
    {
      // set viewport and rendertargets
      uint2 size;
      if (packet.rtvs.size() > 0)
      {
        size = uint2{ packet.rtvs[0].desc().desc.width, packet.rtvs[0].desc().desc.height };
      }
      else if (packet.dsvs.size() > 0)
      {
        size = uint2{ packet.dsvs[0].desc().desc.width, packet.dsvs[0].desc().desc.height };
      }
      else
      {
        return;
      }
      D3D12_VIEWPORT port{};
      port.Width = float(size.x);
      port.Height = float(size.y);
      port.MinDepth = D3D12_MIN_DEPTH;
      port.MaxDepth = D3D12_MAX_DEPTH;
      buffer->RSSetViewports(1, &port);

      D3D12_RECT rect{};
      rect.bottom = size.y;
      rect.right = size.x;
      buffer->RSSetScissorRects(1, &rect);

      D3D12_CPU_DESCRIPTOR_HANDLE rtvs[8]{};
      unsigned maxSize = static_cast<unsigned>(std::min(8ull, packet.rtvs.size()));
      D3D12_CPU_DESCRIPTOR_HANDLE dsv;
      D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;

      D3D12_RENDER_PASS_RENDER_TARGET_DESC rtvDesc[8]{};
      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsvDesc{};
      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* dsvDescPtr = nullptr;
      if (packet.rtvs.size() > 0)
      {
        for (unsigned i = 0; i < maxSize; ++i)
        {
          rtvs[i].ptr = packet.rtvs[i].view().view;
          rtvDesc[i].cpuDescriptor.ptr = packet.rtvs[i].view().view;
          if (packet.rtvs[i].loadOp() == LoadOp::Load)
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
          else if (packet.rtvs[i].loadOp() == LoadOp::DontCare)
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
          else if (packet.rtvs[i].loadOp() == LoadOp::Clear)
          {
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Format = formatTodxFormat(packet.rtvs[i].format()).view;
            auto clearVal = packet.rtvs[i].clearVal();
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[0] = clearVal.x;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[1] = clearVal.y;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[2] = clearVal.z;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[3] = clearVal.w;
          }

          if (packet.rtvs[i].storeOp() == StoreOp::DontCare)
          {
            rtvDesc[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
          }
          else if (packet.rtvs[i].storeOp() == StoreOp::Store)
          {
            rtvDesc[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
          }
        }
      }
      if (packet.dsvs.size() > 0)
      {
        dsv.ptr = packet.dsvs[0].view().view;
        dsvPtr = &dsv;

        dsvDesc.cpuDescriptor.ptr = packet.dsvs[0].view().view;
        dsvDescPtr = &dsvDesc;
        if (packet.dsvs[0].loadOp() == LoadOp::Load)
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;          
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE; 
        }
        else if (packet.dsvs[0].loadOp() == LoadOp::DontCare)
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;          
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        else
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;          
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Format = formatTodxFormat(packet.dsvs[0].format()).view;
          auto clearVal = packet.dsvs[0].clearVal();
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[0] = clearVal.x;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[1] = clearVal.y;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[2] = clearVal.z;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[3] = clearVal.w;
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[0] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[1] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[2] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[3] = 0.f;
        }

        if (packet.dsvs[0].storeOp() == StoreOp::DontCare)
        {
          dsvDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
          dsvDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        }
        else if (packet.dsvs[0].storeOp() == StoreOp::Store)
        {
          dsvDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
          dsvDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
      }
      //buffer->OMSetRenderTargets(maxSize, rtvs, false, dsvPtr);

      buffer->BeginRenderPass(maxSize, rtvDesc, dsvDescPtr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
    }

    void handle(D3D12GraphicsCommandList* buffer, size_t hash, gfxpacket::GraphicsPipelineBind& pipelines)
    {
      for (auto&& it : *pipelines.pipeline.m_pipelines)
      {
        if (it.hash == hash)
        {
          auto pipeline = std::static_pointer_cast<DX12Pipeline>(it.pipeline);
          buffer->SetGraphicsRootSignature(pipeline->root.Get());
          buffer->SetPipelineState(pipeline->pipeline.Get());
          buffer->IASetPrimitiveTopology(pipeline->primitive);
        }
      }
    }
    */

    D3D12_TEXTURE_COPY_LOCATION locationFromTexture(TrackedState tex, int mip, int slice)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      loc.SubresourceIndex = tex.totalMipLevels() * slice + mip;
      loc.pResource = reinterpret_cast<ID3D12Resource*>(tex.resPtr);
      return loc;
    }

    D3D12_TEXTURE_COPY_LOCATION locationFromDynamic(ID3D12Resource* upload, DynamicBufferView& view, Texture& ref)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      loc.PlacedFootprint.Footprint.Width = ref.desc().desc.width;
      loc.PlacedFootprint.Footprint.Height = ref.desc().desc.height;
      loc.PlacedFootprint.Footprint.Depth = ref.desc().desc.depth;
      loc.PlacedFootprint.Footprint.Format = backend::formatTodxFormat(ref.desc().desc.format).storage;
      //loc.PlacedFootprint.Footprint.RowPitch = view.rowPitch(); // ???
      //loc.PlacedFootprint.Offset = view.offset(); // ???
      loc.pResource = upload;
      return loc;
    }

    D3D12_TEXTURE_COPY_LOCATION locationFromReadback(ID3D12Resource* readback, int3 src, FormatType type, size_t rowPitch, uint64_t offset)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      loc.PlacedFootprint.Footprint.Width = src.x;
      loc.PlacedFootprint.Footprint.Height = src.y;
      loc.PlacedFootprint.Footprint.Depth = src.z;
      loc.PlacedFootprint.Footprint.Format = backend::formatTodxFormat(type).storage;
      loc.PlacedFootprint.Footprint.RowPitch = static_cast<unsigned>(rowPitch); // ???
      loc.PlacedFootprint.Offset = offset; // ???
      loc.pResource = readback;
      return loc;
    }

    /*
    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::UpdateTexture& packet, ID3D12Resource* upload)
    {
      D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromTexture(packet.dst.dependency(), packet.mip, packet.slice);
      D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromDynamic(upload, packet.src, packet.dst);
      buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::BufferCopy& packet)
    {
      buffer->CopyResource(reinterpret_cast<ID3D12Resource*>(packet.target.resPtr), reinterpret_cast<ID3D12Resource*>(packet.source.resPtr));
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::BufferCpuToGpuCopy& packet, ID3D12Resource* upload)
    {
      buffer->CopyBufferRegion(reinterpret_cast<ID3D12Resource*>(packet.target.resPtr), 0, upload, packet.offset, packet.size);
    }

    D3D12_BOX toBox(Box box)
    {
      D3D12_BOX asd;
      asd.left = box.leftTopFront.x;
      asd.top = box.leftTopFront.y;
      asd.front = box.leftTopFront.z;
      asd.right = box.rightBottomBack.x;
      asd.bottom = box.rightBottomBack.y;
      asd.back = box.rightBottomBack.z;
      return asd;
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::TextureCopy& packet)
    {
      auto src = locationFromTexture(packet.source, 0, 0);
      auto dst = locationFromTexture(packet.target, 0, 0);
      for (int slice = packet.range.sliceOffset; slice < packet.range.arraySize; ++slice)
      {
        for (int mip = packet.range.mipOffset; mip < packet.range.mipLevels; ++mip)
        {
          auto subresourceIndex = slice * packet.source.totalMipLevels() + mip;
          dst.SubresourceIndex = subresourceIndex;
          src.SubresourceIndex = subresourceIndex;
          buffer->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
      }
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::ReadbackTexture& packet, DX12ReadbackHeap* readback, FreeableResources* free)
    {
      auto dim = sub(packet.srcbox.rightBottomBack, packet.srcbox.leftTopFront);
      int size = calculatePitch(dim, packet.format);
      auto rb = readback->allocate(size);

      int rowPitch = calculatePitch(int3(dim.x, 1, 1), packet.format);
      int slicePitch = calculatePitch(dim, packet.format);
      const auto requiredRowPitch = roundUpMultiplePowerOf2(rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
      D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromReadback(readback->native(), dim, packet.format, requiredRowPitch, rb.offset);
      D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromTexture(packet.target, packet.resource.mipLevel, packet.resource.arraySlice);
      D3D12_BOX box = toBox(packet.srcbox);
      buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);

      auto rbFunc = [fun = packet.func, rowPitch, slicePitch, dim](MemView<uint8_t> view)
      {
        SubresourceData data(view, rowPitch, slicePitch, dim);
        fun(data);
      };
      free->readbacks.push_back(DX12ReadbackLambda{ rb, rbFunc });
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::Readback& packet, DX12ReadbackHeap* readback, FreeableResources* free)
    {
      auto rb = readback->allocate(packet.size);
      free->readbacks.push_back(DX12ReadbackLambda{ rb, packet.func });
      buffer->CopyBufferRegion(readback->native(), rb.offset, reinterpret_cast<ID3D12Resource*>(packet.target.resPtr), packet.offset, packet.size);
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::ComputePipelineBind& packet)
    {
      auto pipeline = std::static_pointer_cast<DX12Pipeline>(packet.pipeline.impl);
      buffer->SetComputeRootSignature(pipeline->root.Get());
      buffer->SetPipelineState(pipeline->pipeline.Get());
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::SetScissorRect& packet)
    {
      D3D12_RECT rect{};
      rect.bottom = packet.bottomright.y;
      rect.right = packet.bottomright.x;
      rect.top = packet.topleft.y;
      rect.left = packet.topleft.x;
      buffer->RSSetScissorRects(1, &rect);
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::Draw& packet)
    {
      buffer->DrawInstanced(packet.vertexCountPerInstance, packet.instanceCount, packet.startVertex, packet.startInstance);
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::DrawIndexed& packet)
    {
      auto& buf = packet.ib;
      auto* ptr = static_cast<DX12Buffer*>(buf.buffer().native().get());
      D3D12_INDEX_BUFFER_VIEW ib{};
      ib.BufferLocation = ptr->native()->GetGPUVirtualAddress();
      ib.Format = formatTodxFormat(buf.desc().desc.format).view;
      ib.SizeInBytes = buf.desc().desc.width * formatSizeInfo(buf.desc().desc.format).pixelSize;
      buffer->IASetIndexBuffer(&ib);

      buffer->DrawIndexedInstanced(packet.IndexCountPerInstance, packet.instanceCount, packet.StartIndexLocation, packet.BaseVertexLocation, packet.StartInstanceLocation);
    }

    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::DrawDynamicIndexed& packet)
    {
      auto& buf = packet.ib;
      auto* ptr = static_cast<DX12DynamicBufferView*>(buf.native().get());
      D3D12_INDEX_BUFFER_VIEW ib = ptr->indexBufferView();
      buffer->IASetIndexBuffer(&ib);

      buffer->DrawIndexedInstanced(packet.IndexCountPerInstance, packet.instanceCount, packet.StartIndexLocation, packet.BaseVertexLocation, packet.StartInstanceLocation);
    }
    void handle(D3D12GraphicsCommandList* buffer, gfxpacket::Dispatch& packet)
    {
      buffer->Dispatch(packet.groups.x, packet.groups.y, packet.groups.z);
    }
*/
    UploadBlock DX12CommandList::allocateConstants(size_t size)
    {
      auto block = m_constantsAllocator.allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      if (!block)
      {
        auto newBlock = m_constants->allocate(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 16);
        F_ASSERT(newBlock, "What!");
        m_freeResources->uploadBlocks.push_back(newBlock);
        m_constantsAllocator = UploadLinearAllocator(newBlock);
        block = m_constantsAllocator.allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      }

      F_ASSERT(block, "What!");
      return block;
    }

    DynamicDescriptorBlock DX12CommandList::allocateDescriptors(size_t size)
    {
      auto block = m_descriptorAllocator.allocate(size);
      if (!block)
      {
        auto newBlock = m_descriptors->allocate(1024);
        F_ASSERT(newBlock, "What!");
        m_freeResources->descriptorBlocks.push_back(newBlock);
        m_descriptorAllocator = LinearDescriptorAllocator(newBlock);
        block = m_descriptorAllocator.allocate(size);
      }

      F_ASSERT(block, "What!");
      return block;
    }
/*
    void DX12CommandList::handleBindings(DX12Device* dev, D3D12GraphicsCommandList* buffer, gfxpacket::ResourceBinding& ding)
    {
      if (ding.constants.size() > 0)
      {
        auto block = allocateConstants(ding.constants.size());
        memcpy(block.data(), ding.constants.data(), ding.constants.size());
        if (ding.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics)
        {
          buffer->SetGraphicsRootConstantBufferView(0, block.gpuVirtualAddress());
        }
        else
        {
          buffer->SetComputeRootConstantBufferView(0, block.gpuVirtualAddress());
        }
      }
      UINT rootIndex = 1;
      if (ding.views.size() > 0)
      {
        const unsigned viewsCount = static_cast<unsigned>(ding.views.size());
        auto descriptors = allocateDescriptors(viewsCount);
        auto start = descriptors.offset(0);

        unsigned destSizes[1] = { viewsCount };
        vector<unsigned> viewSizes(viewsCount, 1);

        dev->m_device->CopyDescriptors(
          1, &(start.cpu), destSizes,
          viewsCount, reinterpret_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(ding.views.data()), viewSizes.data(),
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        if (ding.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics)
        {
          buffer->SetGraphicsRootDescriptorTable(rootIndex, start.gpu);
        }
        else
        {
          buffer->SetComputeRootDescriptorTable(rootIndex, start.gpu);
        }
      }
    }

    void handle(ID3D12GraphicsCommandList* buffer, gfxpacket::ClearRT& packet)
    {
      auto view = std::static_pointer_cast<DX12TextureView>(packet.rtv.native());
      auto texture = std::static_pointer_cast<DX12Texture>(packet.rtv.texture().native());
      float rgba[4]{ packet.color.x, packet.color.y, packet.color.z, packet.color.w };
      buffer->ClearRenderTargetView(view->native().cpu, rgba, 0, nullptr);
    }

    void DX12CommandList::addCommands(DX12Device* dev, D3D12GraphicsCommandList* buffer, DX12DependencySolver* solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;

      if (!m_buffer->dma())
      {
        ID3D12DescriptorHeap* heaps[] = { m_descriptors->native() };
        buffer->SetDescriptorHeaps(1, heaps);
      }

      size_t currentActiveSubpassHash = 0;
      bool startedPixBeginEvent = false;

      CommandPacket* subpass = nullptr;
      //bool subpassCommandsHandled = false;

/*
      auto subpassClears = [&]()
      {
        if (!subpassCommandsHandled)
        {
          auto& ref = packetRef(gfxpacket::RenderpassBegin, subpass);
          for (auto&& rtv : ref.rtvs)
          {
            if (rtv.loadOp() == LoadOp::Clear)
            {
              FLOAT color[4] = { 0.f, 0.f, 0.f, 0.f };
              buffer->ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE{ rtv.view().view }, color, 0, nullptr);
            }
          }
          for (auto&& dsv : ref.dsvs)
          {
            if (dsv.loadOp() == LoadOp::Clear)
            {
              buffer->ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE{ dsv.view().view }, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
            }
          }
          subpassCommandsHandled = true;
        }
      };
      */
/*
      DX12Query activeQuery{};

      std::function<void(MemView<std::pair<std::string, double>>)> queryCallback;
      bool queriesWanted = false;

      DX12Query fullFrameQuery{};
      if (!m_buffer->dma())
      {
        fullFrameQuery = m_queryheap->allocate();
        buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, fullFrameQuery.beginIndex);
        m_freeResources->queries.push_back(QueryBracket{ fullFrameQuery , "Commandlist" });
      }

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
        case CommandPacket::PacketType::RenderBlock:
        {
          if (!m_buffer->dma())
          {
            auto p = packetRef(gfxpacket::RenderBlock, packet);
            if (!startedPixBeginEvent)
            {
              startedPixBeginEvent = true;
            }
            else
            {
#if defined(PROFILE)
              PIXEndEvent(buffer);
#endif
              buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, activeQuery.endIndex);
            }
#if defined(PROFILE)
            PIXBeginEvent(buffer, 0, p.name.c_str());
#endif
            activeQuery = m_queryheap->allocate();
            buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, activeQuery.beginIndex);
            m_freeResources->queries.push_back(QueryBracket{ activeQuery , p.name });
          }
          break;
        }
        case CommandPacket::PacketType::UpdateTexture:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::UpdateTexture, packet), m_upload->native());
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::TextureCopy:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::TextureCopy, packet));
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::BufferCopy:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::BufferCopy, packet));
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::BufferCpuToGpuCopy:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::BufferCpuToGpuCopy, packet), m_upload->native());
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::ReadbackTexture:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::ReadbackTexture, packet), m_readback.get(), m_freeResources.get());
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::Readback:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::Readback, packet), m_readback.get(), m_freeResources.get());
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::RenderpassBegin:
        {
          solver->runBarrier(buffer, drawIndex);
          drawIndex++;
          currentActiveSubpassHash = (packetRef(gfxpacket::RenderpassBegin, packet)).hash;
          handle(buffer, packetRef(gfxpacket::RenderpassBegin, packet));
          subpass = packet;
          //subpassCommandsHandled = false;
          break;
        }
        case CommandPacket::PacketType::RenderpassEnd:
        {
          buffer->EndRenderPass();
          break;
        }
        case CommandPacket::PacketType::ResourceBinding:
        {
          solver->runBarrier(buffer, drawIndex);
          handleBindings(dev, buffer, packetRef(gfxpacket::ResourceBinding, packet));
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::SetScissorRect:
        {
          handle(buffer, packetRef(gfxpacket::SetScissorRect, packet));
          break;
        }
        case CommandPacket::PacketType::DrawIndexed:
        {
          handle(buffer, packetRef(gfxpacket::DrawIndexed, packet));
          break;
        }
        case CommandPacket::PacketType::DrawDynamicIndexed:
        {
          handle(buffer, packetRef(gfxpacket::DrawDynamicIndexed, packet));
          break;
        }
        case CommandPacket::PacketType::Draw:
        {
          handle(buffer, packetRef(gfxpacket::Draw, packet));
          break;
        }
        case CommandPacket::PacketType::Dispatch:
        {
          handle(buffer, packetRef(gfxpacket::Dispatch, packet));
          break;
        }
        case CommandPacket::PacketType::GraphicsPipelineBind:
        {
          if (currentActiveSubpassHash == 0)
            break;
          handle(buffer, currentActiveSubpassHash, packetRef(gfxpacket::GraphicsPipelineBind, packet));
          break;
        }
        case CommandPacket::PacketType::ComputePipelineBind:
        {
          handle(buffer, packetRef(gfxpacket::ComputePipelineBind, packet));
          break;
        }
        case CommandPacket::PacketType::ClearRT:
        {
          solver->runBarrier(buffer, drawIndex);
          handle(buffer, packetRef(gfxpacket::ClearRT, packet));
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          solver->runBarrier(buffer, drawIndex);
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::PrepareForQueueSwitch:
        {
          solver->runBarrier(buffer, drawIndex);
          drawIndex++;
          break;
        }
        case CommandPacket::PacketType::QueryCounters:
        {
          if (!m_buffer->dma())
          {
            queryCallback = (packetRef(gfxpacket::QueryCounters, packet)).func;
            queriesWanted = true;
          }
          break;
        }
        default:
          break;
        }
      }

      if (startedPixBeginEvent)
      {
#if defined(PROFILE)
        PIXEndEvent(buffer);
#endif
        buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, activeQuery.endIndex);
      }
      if (!m_buffer->dma())
      {
        buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, fullFrameQuery.endIndex);
      }
      if (queriesWanted)
      {
        auto rb = m_readback->allocate(m_queryheap->size()*m_queryheap->counterSize());

        auto rbfunc = [queries = m_freeResources->queries, queryCallback, ticksPerSecond = m_queryheap->getGpuTicksPerSecond()](faze::MemView<uint8_t> view)
        {
          auto properView = reinterpret_memView<uint64_t>(view);
          vector<std::pair<std::string, double>> data;
          for (auto&& it : queries)
          {
            auto begin = properView[it.query.beginIndex];
            auto end = properView[it.query.endIndex];
            double delta = static_cast<double>(end - begin);
            data.emplace_back(it.name, (delta / double(ticksPerSecond)) * 1000.0);
            //queryCallback(it.name.c_str(), static_cast<uint64_t>((delta / double(ticksPerSecond)) * 10000000.0));
          }
          queryCallback(data);
        };

        m_freeResources->readbacks.push_back(DX12ReadbackLambda{ rb, rbfunc });
        buffer->ResolveQueryData(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, 0, static_cast<UINT>(m_queryheap->size()), m_readback->native(), rb.offset);
      }
    }

    void DX12CommandList::addDepedencyDataAndSolve(DX12DependencySolver* solver, backend::IntermediateList& list)
    {
      int drawIndex = 0;

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
        case CommandPacket::PacketType::UpdateTexture:
        {
          auto& p = packetRef(gfxpacket::UpdateTexture, packet);
          drawIndex = solver->addDrawCall(packet->type());
          SubresourceRange range{};
          range.mipLevels = 1;
          range.arraySize = 1;
          range.mipOffset = static_cast<int16_t>(p.mip);
          range.sliceOffset = static_cast<int16_t>(p.slice);
          solver->addResource(drawIndex, p.dst.dependency(), D3D12_RESOURCE_STATE_COPY_DEST, range);
          break;
        }
        case CommandPacket::PacketType::Readback:
        {
          auto& p = packetRef(gfxpacket::Readback, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.target, D3D12_RESOURCE_STATE_COPY_SOURCE);
          break;
        }
        case CommandPacket::PacketType::ReadbackTexture:
        {
          auto& p = packetRef(gfxpacket::ReadbackTexture, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.target, D3D12_RESOURCE_STATE_COPY_SOURCE);
          break;
        }
        case CommandPacket::PacketType::BufferCpuToGpuCopy:
        {
          auto& p = packetRef(gfxpacket::BufferCpuToGpuCopy, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.target, D3D12_RESOURCE_STATE_COPY_DEST);
          break;
        }
        case CommandPacket::PacketType::TextureCopy:
        {
          auto& p = packetRef(gfxpacket::TextureCopy, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.target, D3D12_RESOURCE_STATE_COPY_DEST);
          solver->addResource(drawIndex, p.source, D3D12_RESOURCE_STATE_COPY_SOURCE);
          break;
        }
        case CommandPacket::PacketType::BufferCopy:
        {
          auto& p = packetRef(gfxpacket::BufferCopy, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.target, D3D12_RESOURCE_STATE_COPY_DEST);
          solver->addResource(drawIndex, p.source, D3D12_RESOURCE_STATE_COPY_SOURCE);
          break;
        }
        case CommandPacket::PacketType::ClearRT:
        {
          auto& p = packetRef(gfxpacket::ClearRT, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.rtv.dependency(), D3D12_RESOURCE_STATE_RENDER_TARGET);
          break;
        }
        case CommandPacket::PacketType::RenderpassBegin:
        {
          drawIndex = solver->addDrawCall(packet->type());
          auto& s = packetRef(gfxpacket::RenderpassBegin, packet);
          for (auto&& it : s.rtvs)
          {
            solver->addResource(drawIndex, it.dependency(), D3D12_RESOURCE_STATE_RENDER_TARGET);
          }
          for (auto&& it : s.dsvs)
          {
            solver->addResource(drawIndex, it.dependency(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
          }
          break;
        }
        case CommandPacket::PacketType::ResourceBinding:
        {
          drawIndex = solver->addDrawCall(packet->type());
          auto& p = packetRef(gfxpacket::ResourceBinding, packet);

          auto readState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

          if (p.graphicsBinding == gfxpacket::ResourceBinding::BindingType::Graphics)
          {
            readState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
          }

          for (auto&& it : p.resources)
          {
            if (it.readonly)
            {
              solver->addResource(drawIndex, it, readState);
            }
            else
            {
              solver->addResource(drawIndex, it, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            }
          }

          break;
        }
        case CommandPacket::PacketType::DrawIndexed:
        {
          auto& p = packetRef(gfxpacket::DrawIndexed, packet);
          solver->addResource(drawIndex, p.ib.dependency(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
          break;
        }
        case CommandPacket::PacketType::PrepareForPresent:
        {
          auto& p = packetRef(gfxpacket::PrepareForPresent, packet);
          drawIndex = solver->addDrawCall(packet->type());
          solver->addResource(drawIndex, p.texture.dependency(), D3D12_RESOURCE_STATE_PRESENT);
          break;
        }
        case CommandPacket::PacketType::PrepareForQueueSwitch:
        {
          auto& p = packetRef(gfxpacket::PrepareForQueueSwitch, packet);
          drawIndex = solver->addDrawCall(packet->type());
          for (auto&& res : p.deps)
          {
            if (!reinterpret_cast<DX12ResourceState*>(res.statePtr)->commonStateOptimisation)
            {
              solver->addResource(drawIndex, res, D3D12_RESOURCE_STATE_COMMON);
            }
          }
          break;
        }
        default:
          break;
        }
      }
      solver->makeAllBarriers();
    }

    void DX12CommandList::processRenderpasses(DX12Device* dev, backend::IntermediateList& list)
    {
      gfxpacket::RenderpassBegin* activeRenderpass = nullptr;

      for (CommandPacket* packet : list)
      {
        switch (packet->type())
        {
        case CommandPacket::PacketType::RenderpassBegin:
        {
          activeRenderpass = packetPtr(gfxpacket::RenderpassBegin, packet);
          break;
        }
        case CommandPacket::PacketType::ComputePipelineBind:
        {
          auto& ref = packetRef(gfxpacket::ComputePipelineBind, packet);
          dev->updatePipeline(ref.pipeline);
          break;
        }
        case CommandPacket::PacketType::GraphicsPipelineBind:
        {
          auto& ref = packetRef(gfxpacket::GraphicsPipelineBind, packet);
          F_ASSERT(activeRenderpass, "nullptr");
          dev->updatePipeline(ref.pipeline, *activeRenderpass);
          break;
        }
        default:
          break;
        }
      }
    }
*/
    // implementations
    void DX12CommandList::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::CommandBuffer& buffer, BarrierSolver& solver)
    {
      DX12Device* dev = static_cast<DX12Device*>(device.get());
#if 0
      addDepedencyDataAndSolve(m_buffer->solver(), list);
      processRenderpasses(dev, list);
      addCommands(dev, m_buffer->list(), m_buffer->solver(), list);
#elif 0
      DX12DependencySolver solver;
      addDepedencyDataAndSolve(&solver, list);
      processRenderpasses(dev, list);
      addCommands(dev, m_buffer->list(), &solver, list);
#endif

      m_buffer->closeList();
    }
  }
}
#endif