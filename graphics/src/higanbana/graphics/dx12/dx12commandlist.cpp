#include "higanbana/graphics/dx12/dx12commandlist.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/dx12device.hpp"
#include "higanbana/graphics/dx12/util/formats.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/command_packets.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"
#include "higanbana/graphics/common/barrier_solver.hpp"

#if !USE_PIX
#define USE_PIX 1
#endif
#include <WinPixEventRuntime/pix3.h>
#include <algorithm>

namespace higanbana
{
  namespace backend
  {
    DX12CommandList::DX12CommandList(
      std::shared_ptr<DX12CommandBuffer> buffer,
      std::shared_ptr<DX12UploadHeap> constants,
      std::shared_ptr<DX12UploadHeap> dynamicUpload,
      std::shared_ptr<DX12ReadbackHeap> readback,
      std::shared_ptr<DX12QueryHeap> queryheap,
      std::shared_ptr<DX12DynamicDescriptorHeap> descriptors,
      DX12CPUDescriptor nullBufferUAV,
      DX12CPUDescriptor nullBufferSRV)
      : m_buffer(buffer)
      , m_constants(constants)
      , m_upload(dynamicUpload)
      , m_readback(readback)
      , m_queryheap(queryheap)
      , m_descriptors(descriptors)
      , m_nullBufferUAV(nullBufferUAV)
      , m_nullBufferSRV(nullBufferSRV)
    {
      m_buffer->resetList();
      m_readback->reset();
      m_queryheap->reset();

      std::weak_ptr<DX12UploadHeap> consts = m_constants;
      std::weak_ptr<DX12DynamicDescriptorHeap> descriptrs = m_descriptors;
      std::weak_ptr<DX12ReadbackHeap> read = readback;

      m_freeResources = std::shared_ptr<FreeableResources>(new FreeableResources, [consts, descriptrs, read](FreeableResources* ptr)
      {
        if (auto constants = consts.lock())
        {
          for (auto&& it : ptr->uploadBlocks)
          {
            constants->release(it);
          }
        }

        if (auto descriptors = descriptrs.lock())
        {
          for (auto&& it : ptr->descriptorBlocks)
          {
            descriptors->release(it);
          }
        }

        // handle readbacks
        if (auto readback = read.lock())
        {
          if (!ptr->readbacks.empty())
          {
            readback->map();
            for (auto&& it : ptr->readbacks)
            {
              it.func(readback->getView(it.dataLocation));
            }
            readback->unmap();
          }
        }

        delete ptr;
      });
    }
    void handle(DX12Device* ptr, D3D12GraphicsCommandList* buffer, gfxpacket::RenderPassBegin& packet)
    {
      // set viewport and rendertargets
      uint2 size;

      auto rtvs = packet.rtvs.convertToMemView();

      auto& allRes = ptr->allResources();
      if (rtvs.size() > 0)
      {
        auto& d = allRes.tex[rtvs[0].resource].desc().desc;
        size = uint2{ d.width, d.height };
      }
      else if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        auto& d = allRes.tex[packet.dsv.resource].desc().desc;
        size = uint2{ d.width, d.height };
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

      //D3D12_CPU_DESCRIPTOR_HANDLE trtvs[8]{};
      unsigned maxSize = static_cast<unsigned>(std::min(8ull, rtvs.size()));
      //D3D12_CPU_DESCRIPTOR_HANDLE dsv;
      //D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;

      D3D12_RENDER_PASS_RENDER_TARGET_DESC rtvDesc[8]{};
      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsvDesc{};
      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* dsvDescPtr = nullptr;

      auto clearValues = packet.clearValues.convertToMemView();
      if (rtvs.size() > 0)
      {
        for (unsigned i = 0; i < maxSize; ++i)
        {
          auto& rtv = allRes.texRTV[rtvs[i]];
          //trtvs[i].ptr = rtv.native().cpu.ptr;
          rtvDesc[i].cpuDescriptor.ptr = rtv.native().cpu.ptr;
          if (rtvs[i].loadOp() == LoadOp::Load)
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
          else if (rtvs[i].loadOp() == LoadOp::DontCare)
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
          else if (rtvs[i].loadOp() == LoadOp::Clear)
          {
            rtvDesc[i].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Format = rtv.viewFormat();
            auto clearVal = clearValues[i];
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[0] = clearVal.x;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[1] = clearVal.y;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[2] = clearVal.z;
            rtvDesc[i].BeginningAccess.Clear.ClearValue.Color[3] = clearVal.w;
          }

          if (rtvs[i].storeOp() == StoreOp::DontCare)
          {
            rtvDesc[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
          }
          else if (rtvs[i].storeOp() == StoreOp::Store)
          {
            rtvDesc[i].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
          }
        }
      }
      if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        auto& ndsv = allRes.texDSV[packet.dsv];
        //dsv.ptr = ndsv.native().cpu.ptr;
        //dsvPtr = &dsv;

        dsvDesc.cpuDescriptor.ptr = ndsv.native().cpu.ptr;
        dsvDescPtr = &dsvDesc;
        if (packet.dsv.loadOp() == LoadOp::Load)
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;          
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE; 
        }
        else if (packet.dsv.loadOp() == LoadOp::DontCare)
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;          
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        else
        {
          dsvDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;          
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Format = ndsv.viewFormat();
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[0] = packet.clearDepth;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[1] = 0.f;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[2] = 0.f;
          dsvDesc.DepthBeginningAccess.Clear.ClearValue.Color[3] = 0.f;
          dsvDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[0] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[1] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[2] = 0.f;
          dsvDesc.StencilBeginningAccess.Clear.ClearValue.Color[3] = 0.f;
        }

        if (packet.dsv.storeOp() == StoreOp::DontCare)
        {
          dsvDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
          dsvDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        }
        else if (packet.dsv.storeOp() == StoreOp::Store)
        {
          dsvDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
          dsvDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
      }
      //buffer->OMSetRenderTargets(maxSize, rtvs, false, dsvPtr);

      buffer->BeginRenderPass(maxSize, rtvDesc, dsvDescPtr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
    }

/*
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

    D3D12_TEXTURE_COPY_LOCATION locationFromTexture(ID3D12Resource* tex,int mips, int mip, int slice)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      loc.SubresourceIndex = mips * slice + mip;
      loc.pResource = tex;
      return loc;
    }

    D3D12_TEXTURE_COPY_LOCATION locationFromDynamic(ID3D12Resource* upload, DX12DynamicBufferView& view, int width, int height, FormatType format)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      loc.PlacedFootprint.Footprint.Width = width;
      loc.PlacedFootprint.Footprint.Height = height;
      loc.PlacedFootprint.Footprint.Depth = 1;
      loc.PlacedFootprint.Footprint.Format = backend::formatTodxFormat(format).storage;
      loc.PlacedFootprint.Footprint.RowPitch = view.rowPitch(); // ???
      loc.PlacedFootprint.Offset = view.offset();
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
        HIGAN_ASSERT(newBlock, "What!");
        m_freeResources->uploadBlocks.push_back(newBlock);
        m_constantsAllocator = UploadLinearAllocator(newBlock);
        block = m_constantsAllocator.allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      }

      HIGAN_ASSERT(block, "What!");
      return block;
    }

    DynamicDescriptorBlock DX12CommandList::allocateDescriptors(size_t size)
    {
      auto block = m_descriptorAllocator.allocate(size);
      if (!block)
      {
        auto newBlock = m_descriptors->allocate(1024);
        HIGAN_ASSERT(newBlock, "What!");
        m_freeResources->descriptorBlocks.push_back(newBlock);
        m_descriptorAllocator = LinearDescriptorAllocator(newBlock);
        block = m_descriptorAllocator.allocate(size);
      }

      HIGAN_ASSERT(block, "What!");
      return block;
    }

    void DX12CommandList::handleBindings(DX12Device* dev, D3D12GraphicsCommandList* buffer, gfxpacket::ResourceBinding& ding)
    {
      auto pconstants = ding.constants.convertToMemView();
      if (pconstants.size() > 0)
      {
        auto block = allocateConstants(pconstants.size());
        memcpy(block.data(), pconstants.data(), pconstants.size());
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
      auto pviews = ding.resources.convertToMemView();
      if (pviews.size() > 0)
      {
        vector<D3D12_CPU_DESCRIPTOR_HANDLE> views;
        auto& ar = dev->allResources();
        for (auto&& handle : pviews)
        {
          switch (handle.type)
          {
            case ViewResourceType::BufferSRV:
            {
              views.push_back(ar.bufSRV[handle].native().cpu);
              break;
            }
            case ViewResourceType::BufferUAV:
            {
              views.push_back(ar.bufUAV[handle].native().cpu);
              break;
            }
            case ViewResourceType::DynamicBufferSRV:
            {
              views.push_back(ar.dynSRV[handle].native().cpu);
              break;
            }
            case ViewResourceType::TextureSRV:
            {
              views.push_back(ar.texSRV[handle].native().cpu);
              break;
            }
            case ViewResourceType::TextureUAV:
            {
              views.push_back(ar.texUAV[handle].native().cpu);
              break;
            }
            default:
              break;
          }
        }
        const unsigned viewsCount = static_cast<unsigned>(views.size());
        auto descriptors = allocateDescriptors(viewsCount);
        auto start = descriptors.offset(0);

        unsigned destSizes[1] = { viewsCount };
        vector<unsigned> viewSizes(viewsCount, 1);

        dev->m_device->CopyDescriptors(
          1, &(start.cpu), destSizes,
          viewsCount, reinterpret_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(views.data()), viewSizes.data(),
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

/*
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

        auto rbfunc = [queries = m_freeResources->queries, queryCallback, ticksPerSecond = m_queryheap->getGpuTicksPerSecond()](higanbana::MemView<uint8_t> view)
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

*/

    D3D12_RESOURCE_STATES translateAccessMask(AccessStage stage, AccessUsage usage)
    {
      D3D12_RESOURCE_STATES flags = D3D12_RESOURCE_STATE_COMMON;
      if (AccessUsage::Read == usage)
      {
        if (stage & AccessStage::Compute)               flags |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (stage & AccessStage::Graphics)              flags |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (stage & AccessStage::Transfer)              flags |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (stage & AccessStage::Index)                 flags |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
        if (stage & AccessStage::Indirect)              flags |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if (stage & AccessStage::Rendertarget)          flags |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (stage & AccessStage::DepthStencil)          flags |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if (stage & AccessStage::Present)               flags |= D3D12_RESOURCE_STATE_PRESENT;
        if (stage & AccessStage::Raytrace)              flags |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (stage & AccessStage::AccelerationStructure) flags |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
      }
      if (AccessUsage::Write == usage || AccessUsage::ReadWrite == usage)
      {
        if (stage & AccessStage::Compute)               flags |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (stage & AccessStage::Graphics)              flags |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (stage & AccessStage::Transfer)              flags |= D3D12_RESOURCE_STATE_COPY_DEST;
        if (stage & AccessStage::Index)                 flags |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS; //?
        if (stage & AccessStage::Indirect)              flags |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS; //?
        if (stage & AccessStage::Rendertarget)          flags |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (stage & AccessStage::DepthStencil)          flags |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (stage & AccessStage::Present)               flags |= D3D12_RESOURCE_STATE_PRESENT; //?
        if (stage & AccessStage::Raytrace)              flags |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (stage & AccessStage::AccelerationStructure) flags |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
      }
      return flags;
    }

    void addBarrier(DX12Device* device, D3D12GraphicsCommandList* buffer, MemoryBarriers mbarriers)
    {
      if (!mbarriers.buffers.empty() || !mbarriers.textures.empty())
      {
        vector<D3D12_RESOURCE_BARRIER> barriers;
        for (auto& buffer : mbarriers.buffers)
        {
          D3D12_RESOURCE_BARRIER_TYPE type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          auto flag = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          auto beforeState = translateAccessMask(buffer.before.stage, buffer.before.usage);
          auto afterState = translateAccessMask(buffer.after.stage, buffer.after.usage);
          if (beforeState == afterState)
            continue;
          auto& vbuffer = device->allResources().buf[buffer.handle];

          D3D12_RESOURCE_TRANSITION_BARRIER transition;
          transition.pResource = vbuffer.native();
          transition.StateBefore = beforeState;
          transition.StateAfter = afterState;
          transition.Subresource = 0;

          barriers.emplace_back(D3D12_RESOURCE_BARRIER{type, flag, transition});
        }
        for (auto& image : mbarriers.textures)
        {
          auto& tex = device->allResources().tex[image.handle];
          auto maxMip = tex.mipSize();

          D3D12_RESOURCE_BARRIER_TYPE type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          auto flag = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          auto beforeState = translateAccessMask(image.before.stage, image.before.usage);
          auto afterState = translateAccessMask(image.after.stage, image.after.usage);
          if (beforeState == afterState)
            continue;
          D3D12_RESOURCE_TRANSITION_BARRIER transition;
          transition.pResource = tex.native();
          transition.StateBefore = beforeState;
          transition.StateAfter = afterState;

          for (auto slice = image.startArr; slice < image.arrSize; ++slice)
          {
            for (auto mip = image.startMip; mip < image.mipSize; ++mip)
            {
              auto subresource = slice * maxMip + mip;
              transition.Subresource = subresource;
              barriers.emplace_back(D3D12_RESOURCE_BARRIER{type, flag, transition});
            }
          }
        }
#if 0
        for (int i = 0; i < barriers.size(); ++i)
        {
          auto& barrier = barriers[i];
          buffer->ResourceBarrier(1, &barrier);
        }
#else
        if (!barriers.empty())
          buffer->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
#endif
      }
    }
    void DX12CommandList::addCommands(DX12Device* device, D3D12GraphicsCommandList* buffer, backend::CommandBuffer& list, BarrierSolver& solver)
    {
      int drawIndex = 0;
      int framebuffer = 0;
      bool pixevent = false;
      ResourceHandle boundPipeline;
      std::string currentBlock;

      if (!m_buffer->dma())
      {
        ID3D12DescriptorHeap* heaps[] = { m_descriptors->native() };
        buffer->SetDescriptorHeaps(1, heaps);
      }

      for (auto iter = list.begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
      {
        auto* header = *iter;
        //HIGAN_ILOG("addCommandsVK", "type header %d", header->type);
        addBarrier(device, buffer, solver.runBarrier(drawIndex));
        switch (header->type)
        {
        case PacketType::RenderBlock:
        {
          auto block = header->data<gfxpacket::RenderBlock>().name.convertToMemView();
          currentBlock = std::string(block.data());
          if (pixevent)
          {
            PIXEndEvent(buffer);
          }
          PIXBeginEvent(buffer, PIX_COLOR_INDEX(drawIndex), block.data());
          pixevent = true;
          break;
        }
        case PacketType::PrepareForPresent:
        {
          break;
        }
        case PacketType::RenderpassBegin:
        {
          handle(device, buffer, header->data<gfxpacket::RenderPassBegin>());
          framebuffer++;
          break;
        }
        case PacketType::ScissorRect:
        {
          gfxpacket::ScissorRect& packet = header->data<gfxpacket::ScissorRect>();
          D3D12_RECT rect{};
          rect.bottom = packet.bottomright.y;
          rect.right = packet.bottomright.x;
          rect.top = packet.topleft.y;
          rect.left = packet.topleft.x;
          buffer->RSSetScissorRects(1, &rect);
          break;
        }
        case PacketType::GraphicsPipelineBind:
        {
          gfxpacket::GraphicsPipelineBind& packet = header->data<gfxpacket::GraphicsPipelineBind>();
          if (boundPipeline.id != packet.pipeline.id) {
            auto& pipe = device->allResources().pipelines[packet.pipeline];
            buffer->SetGraphicsRootSignature(pipe.root.Get());
            buffer->SetPipelineState(pipe.pipeline.Get());
            buffer->IASetPrimitiveTopology(pipe.primitive);
            boundPipeline = packet.pipeline;
          }
          break;
        }
        case PacketType::ComputePipelineBind:
        {
          gfxpacket::ComputePipelineBind& packet = header->data<gfxpacket::ComputePipelineBind>();
          if (boundPipeline.id != packet.pipeline.id) {
            auto& pipe = device->allResources().pipelines[packet.pipeline];
            buffer->SetComputeRootSignature(pipe.root.Get());
            buffer->SetPipelineState(pipe.pipeline.Get());
            boundPipeline = packet.pipeline;
          }

          break;
        }
        case PacketType::ResourceBinding:
        {
          gfxpacket::ResourceBinding& packet = header->data<gfxpacket::ResourceBinding>();
          handleBindings(device, buffer, packet);
          break;
        }
        case PacketType::Draw:
        {
          auto params = header->data<gfxpacket::Draw>();
          buffer->DrawInstanced(params.vertexCountPerInstance, params.instanceCount, params.startVertex, params.startInstance);
          break;
        }
        case PacketType::DrawIndexed:
        {
          auto params = header->data<gfxpacket::DrawIndexed>();
          D3D12_INDEX_BUFFER_VIEW ib{};
          if (params.indexbuffer.type == ViewResourceType::BufferIBV)
          {
            auto& ibv = device->allResources().bufIBV[params.indexbuffer];
            auto& buf = device->allResources().buf[params.indexbuffer.resource];
            ib.BufferLocation = ibv.ref()->GetGPUVirtualAddress();
            ib.Format = formatTodxFormat(buf.desc().desc.format).view;
            ib.SizeInBytes = buf.desc().desc.width * formatSizeInfo(buf.desc().desc.format).pixelSize;
          }
          else
          {
            auto& ibv = device->allResources().dynSRV[params.indexbuffer];
            ib = ibv.indexBufferView();
          }
          buffer->IASetIndexBuffer(&ib);

          buffer->DrawIndexedInstanced(params.IndexCountPerInstance, params.instanceCount, params.StartIndexLocation, params.BaseVertexLocation, params.StartInstanceLocation);
          break;
        }
        case PacketType::Dispatch:
        {
          auto params = header->data<gfxpacket::Dispatch>();
          buffer->Dispatch(params.groups.x, params.groups.y, params.groups.z);
          break;
        }
        case PacketType::BufferCopy:
        {
          auto params = header->data<gfxpacket::BufferCopy>();
          auto dst = device->allResources().buf[params.dst].native();
          auto src = device->allResources().buf[params.src].native();
          buffer->CopyBufferRegion(dst, params.dstOffset, src, params.srcOffset, params.numBytes);
          break;
        }
        case PacketType::UpdateTexture:
        {
          auto params = header->data<gfxpacket::UpdateTexture>();
          auto texture = device->allResources().tex[params.tex];
          auto dynamic = device->allResources().dynSRV[params.dynamic];

          D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromTexture(texture.native(), params.allMips, params.mip, params.slice);
          D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromDynamic(m_upload->native(), dynamic, params.width, params.height, texture.desc().desc.format);
          buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
          break;
        }
        case PacketType::RenderpassEnd:
        {
          buffer->EndRenderPass();
          break;
        }
        default:
          break;
        }
        drawIndex++;
      }
      if (pixevent)
      {
        PIXEndEvent(buffer);
      }
    }

    void DX12CommandList::processRenderpasses(DX12Device* dev, backend::CommandBuffer& list)
    {
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;
      // find all renderpasses & compile all missing graphics pipelines
      for (auto iter = list.begin(); (*iter)->type != backend::PacketType::EndOfPackets; iter++)
      {
        switch ((*iter)->type)
        {
          case PacketType::RenderpassBegin:
          {
            rpbegin = (*iter);
            break;
          }
          case PacketType::ComputePipelineBind:
          {
            gfxpacket::ComputePipelineBind& packet = (*iter)->data<gfxpacket::ComputePipelineBind>();
            auto oldPipe = dev->updatePipeline(packet.pipeline);
            if (oldPipe)
            {
              m_freeResources->pipelines.push_back(oldPipe.value());
            }
            break;
          }
          case PacketType::GraphicsPipelineBind:
          {
            gfxpacket::GraphicsPipelineBind& packet = (*iter)->data<gfxpacket::GraphicsPipelineBind>();
            auto oldPipe = dev->updatePipeline(packet.pipeline, rpbegin->data<gfxpacket::RenderPassBegin>());
            if (oldPipe)
            {
              m_freeResources->pipelines.push_back(oldPipe.value());
            }
            break;
          }
          default:
            break;
        }
      }
    }
    // implementations
    void DX12CommandList::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, backend::CommandBuffer& buffer, BarrierSolver& solver)
    {
      DX12Device* dev = static_cast<DX12Device*>(device.get());
      processRenderpasses(dev, buffer);
      addCommands(dev, m_buffer->list(), buffer, solver);
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