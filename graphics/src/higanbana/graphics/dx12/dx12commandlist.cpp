#include "higanbana/graphics/dx12/dx12commandlist.hpp"
#if defined(HIGANBANA_PLATFORM_WINDOWS)
#include "higanbana/graphics/dx12/dx12device.hpp"
#include "higanbana/graphics/dx12/util/formats.hpp"
#include "higanbana/graphics/common/texture.hpp"
#include "higanbana/graphics/common/command_buffer.hpp"
#include "higanbana/graphics/common/command_packets.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"
#include "higanbana/graphics/common/barrier_solver.hpp"
#include <higanbana/core/profiling/profiling.hpp>

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
      QueueType type,
      std::shared_ptr<DX12CommandBuffer> buffer,
      std::shared_ptr<DX12ConstantsHeap> constants,
      std::shared_ptr<DX12ReadbackHeap> readback,
      std::shared_ptr<DX12QueryHeap> queryheap,
      std::shared_ptr<DX12DynamicDescriptorHeap> descriptors,
      DX12CPUDescriptor nullBufferUAV,
      DX12CPUDescriptor nullBufferSRV,
      DX12GPUDescriptor shaderDebugTable)
      : m_type(type)
      , m_buffer(buffer)
      , m_constants(constants)
      , m_readback(readback)
      , m_queryheap(queryheap)
      , m_descriptors(descriptors)
      , m_nullBufferUAV(nullBufferUAV)
      , m_nullBufferSRV(nullBufferSRV)
      , m_shaderDebugTable(shaderDebugTable)
    {
      m_readback->reset();
      m_queryheap->reset();

      std::weak_ptr<DX12ConstantsHeap> consts = m_constants;
      std::weak_ptr<DX12DynamicDescriptorHeap> descriptrs = m_descriptors;
      std::weak_ptr<DX12ReadbackHeap> read = readback;

      m_freeResources = std::shared_ptr<FreeableResources>(new FreeableResources, [consts, descriptrs, read](FreeableResources* ptr)
      {
        if (auto constants = consts.lock())
        {
          if (!ptr->uploadBlocks.empty())
            constants->releaseVector(ptr->uploadBlocks);
        }

        if (auto descriptors = descriptrs.lock())
        {
          for (auto&& it : ptr->descriptorBlocks)
          {
            descriptors->release(it);
          }
        }

        delete ptr;
      });
    }
    void handle(DX12Device* ptr, D3D12GraphicsCommandList* buffer, gfxpacket::RenderPassBegin& packet)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      // set viewport and rendertargets
      uint2 size;

      auto rtvs = packet.rtvs.convertToMemView();

      auto& allRes = ptr->allResources();
      if (rtvs.size() > 0)
      {
        auto& d = allRes.tex[rtvs[0].resourceHandle()].desc().desc;
        size = uint2{ d.width, d.height };
      }
      else if (packet.dsv.id != ViewResourceHandle::InvalidViewId)
      {
        auto& d = allRes.tex[packet.dsv.resourceHandle()].desc().desc;
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

    D3D12_TEXTURE_COPY_LOCATION locationFromBuffer(ID3D12Resource* buf, uint32_t offset, uint32_t rowPitch, int width, int height, FormatType format)
    {
      D3D12_TEXTURE_COPY_LOCATION loc{};
      loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      loc.PlacedFootprint.Footprint.Width = width;
      loc.PlacedFootprint.Footprint.Height = height;
      loc.PlacedFootprint.Footprint.Depth = 1;
      loc.PlacedFootprint.Footprint.Format = backend::formatTodxFormat(format).storage;
      loc.PlacedFootprint.Footprint.RowPitch = rowPitch;
      loc.PlacedFootprint.Offset = offset;
      loc.pResource = buf;
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
    UploadBlockGPU DX12CommandList::allocateConstants(size_t size)
    {
      auto block = m_constantsAllocator.allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      if (!block)
      {
        HIGAN_ASSERT(false, "never should come here");
        auto newBlock = m_constants->allocate(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 256 * 4, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        HIGAN_ASSERT(newBlock && newBlock.block.offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "What!");
        m_freeResources->uploadBlocks.push_back(newBlock);
        m_constantsAllocator = UploadLinearAllocatorGPU(newBlock);
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
    void DX12CommandList::handleBindings(DX12Device* dev, D3D12GraphicsCommandList* buffer, gfxpacket::ResourceBindingGraphics& ding)
    {
      auto pconstants = ding.constantsView();
      if (pconstants.size() > 0)
      {
        auto block = allocateConstants(pconstants.size());
        memcpy(block.data(), pconstants.data(), pconstants.size());
        buffer->SetGraphicsRootConstantBufferView(0, block.gpuVirtualAddress());
      }
      auto tables = ding.resourcesView();
      UINT rootIndex = 2; // 0 is constants, 1 is shader debug uav table, starting from 2 is normal tables
      // just bind shader debug always to root slot 1 ...?
      buffer->SetGraphicsRootDescriptorTable(1, m_shaderDebugTable.gpu);
      int tableIndex = 0;
      for (auto& table : tables)
      {
        if (m_boundSets[tableIndex] != table)
        {
          auto& btable = dev->allResources().shaArgs[table];
          buffer->SetGraphicsRootDescriptorTable(rootIndex, btable.descriptorTable.offset(0).gpu);
          m_boundSets[tableIndex] = table; 
        }
        rootIndex++; tableIndex++;
      }
    }

    void DX12CommandList::handleBindings(DX12Device* dev, D3D12GraphicsCommandList* buffer, gfxpacket::ResourceBindingCompute& ding)
    {
      auto pconstants = ding.constantsView();
      if (pconstants.size() > 0)
      {
        auto block = allocateConstants(pconstants.size());
        memcpy(block.data(), pconstants.data(), pconstants.size());
        buffer->SetComputeRootConstantBufferView(0, block.gpuVirtualAddress());
      }
      auto tables = ding.resourcesView();
      UINT rootIndex = 2; // 0 is constants, 1 is shader debug uav table, starting from 2 is normal tables
      // just bind shader debug always to root slot 1 ...?
      buffer->SetComputeRootDescriptorTable(1, m_shaderDebugTable.gpu);
      int tableIndex = 0;
      for (auto& table : tables)
      {
        if (m_boundSets[tableIndex] != table)
        {
          auto& btable = dev->allResources().shaArgs[table];
          buffer->SetComputeRootDescriptorTable(rootIndex, btable.descriptorTable.offset(0).gpu);
          m_boundSets[tableIndex] = table; 
        }
        rootIndex++; tableIndex++;
      }
    }

    std::string stateToString(D3D12_RESOURCE_STATES state) {
      std::string resultingString = "";
      if (state == D3D12_RESOURCE_STATE_COMMON) return "D3D12_RESOURCE_STATE_COMMON";
      if (state == D3D12_RESOURCE_STATE_GENERIC_READ) return "D3D12_RESOURCE_STATE_GENERIC_READ";
      if ((state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        resultingString += "D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | ";
      if ((state & D3D12_RESOURCE_STATE_INDEX_BUFFER) == D3D12_RESOURCE_STATE_INDEX_BUFFER)
        resultingString += "D3D12_RESOURCE_STATE_INDEX_BUFFER | ";
      if ((state & D3D12_RESOURCE_STATE_RENDER_TARGET) == D3D12_RESOURCE_STATE_RENDER_TARGET)
        resultingString += "D3D12_RESOURCE_STATE_RENDER_TARGET | ";
      if ((state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        resultingString += "D3D12_RESOURCE_STATE_UNORDERED_ACCESS | ";
      if ((state & D3D12_RESOURCE_STATE_DEPTH_WRITE) == D3D12_RESOURCE_STATE_DEPTH_WRITE)
        resultingString += "D3D12_RESOURCE_STATE_DEPTH_WRITE | ";
      if ((state & D3D12_RESOURCE_STATE_DEPTH_READ) == D3D12_RESOURCE_STATE_DEPTH_READ)
        resultingString += "D3D12_RESOURCE_STATE_DEPTH_READ | ";
      if ((state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        resultingString += "D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | ";
      if ((state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        resultingString += "D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | ";
      if ((state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT) == D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
        resultingString += "D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | ";
      auto WTF = state & D3D12_RESOURCE_STATE_COPY_DEST;
      auto WTF2 = WTF == D3D12_RESOURCE_STATE_COPY_DEST;
      if (WTF2)
        resultingString += "D3D12_RESOURCE_STATE_COPY_DEST | ";
      if ((state & D3D12_RESOURCE_STATE_COPY_SOURCE) == D3D12_RESOURCE_STATE_COPY_SOURCE)
        resultingString += "D3D12_RESOURCE_STATE_COPY_SOURCE | ";
      if ((state & D3D12_RESOURCE_STATE_RESOLVE_DEST) == D3D12_RESOURCE_STATE_RESOLVE_DEST)
        resultingString += "D3D12_RESOURCE_STATE_RESOLVE_DEST | ";
      if ((state & D3D12_RESOURCE_STATE_RESOLVE_SOURCE) == D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
        resultingString += "D3D12_RESOURCE_STATE_RESOLVE_SOURCE | ";
      if ((state & D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
        resultingString += "D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE | ";
      if ((state & D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE) == D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE)
        resultingString += "D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE | ";
      if ((state & D3D12_RESOURCE_STATE_PREDICATION) == D3D12_RESOURCE_STATE_PREDICATION)
        resultingString += "D3D12_RESOURCE_STATE_PREDICATION | ";
      return resultingString;
    }

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
        if (stage & AccessStage::ShadingRateSource)     flags |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
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
        if (stage & AccessStage::ShadingRateSource)     flags |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
      }
      return flags;
    }

    void DX12CommandList::addBarrier(DX12Device* device, D3D12GraphicsCommandList* buffer, MemoryBarriers mbarriers)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      if (!mbarriers.buffers.empty() || !mbarriers.textures.empty())
      {
        m_barriers.clear();
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

          m_barriers.emplace_back(D3D12_RESOURCE_BARRIER{type, flag, transition});
        }
        for (auto& image : mbarriers.textures)
        {
          auto& tex = device->allResources().tex[image.handle];
          auto maxMip = tex.mipSize();

          D3D12_RESOURCE_BARRIER_TYPE type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
          auto flag = D3D12_RESOURCE_BARRIER_FLAG_NONE;
          auto beforeState = translateAccessMask(image.before.stage, image.before.usage);
          auto afterState = translateAccessMask(image.after.stage, image.after.usage);
          auto barrierBef = stateToString(beforeState);
          auto barrierAft = stateToString(afterState);
          
          //HIGAN_LOGi("tex: %zd m: %d-%d s: %d-%d transition: %s -> %s...", image.handle.id, image.startMip, image.mipSize, image.startArr, image.arrSize, barrierBef.c_str(), barrierAft.c_str());
          if (beforeState == afterState) {
            //HIGAN_LOGi("skipped\n");
            continue;
          }
          //HIGAN_LOGi("doing \n");
          D3D12_RESOURCE_TRANSITION_BARRIER transition;
          transition.pResource = tex.native();
          transition.StateBefore = beforeState;
          transition.StateAfter = afterState;

          for (auto slice = image.startArr; slice < image.startArr+image.arrSize; ++slice)
          {
            for (auto mip = image.startMip; mip < image.startMip+image.mipSize; ++mip)
            {
              auto subresource = slice * maxMip + mip;
              transition.Subresource = subresource;
              //HIGAN_LOGi("tex: %zd m: %d s: %d transition: %s -> %s\n", image.handle.id, mip, slice, barrierBef.c_str(), barrierAft.c_str());
              m_barriers.emplace_back(D3D12_RESOURCE_BARRIER{type, flag, transition});
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
        if (!m_barriers.empty())
          buffer->ResourceBarrier(static_cast<UINT>(m_barriers.size()), m_barriers.data());
#endif
      }
    }
    void DX12CommandList::addCommands(DX12Device* device, D3D12GraphicsCommandList* buffer, MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver)
    {
      HIGAN_CPU_FUNCTION_SCOPE();
      int drawIndex = 0;
      int framebuffer = 0;
      int renderBlockTimeIndex = 0;
      bool pixevent = false;
      ResourceHandle boundPipeline;
      std::string currentBlock;

      auto barrierInfoIndex = 0;
      auto& barrierInfos = solver.barrierInfos();
      auto barrierInfosSize = barrierInfos.size();
      backend::CommandBuffer::PacketHeader* rpbegin = nullptr;

      DX12Query timestamp;

      if (!m_buffer->dma())
      {
        ID3D12DescriptorHeap* heaps[] = { m_descriptors->native() };
        buffer->SetDescriptorHeaps(1, heaps);
      }

      for (auto&& list : buffers)
      {
        for (auto iter = list->begin(); (*iter)->type != PacketType::EndOfPackets; iter++)
        {
          auto* header = *iter;
          //HIGAN_ILOG("addCommandsVK", "type header %d", header->type);
          if (barrierInfoIndex < barrierInfosSize && barrierInfos[barrierInfoIndex].drawcall == drawIndex)
          {
            addBarrier(device, buffer, solver.runBarrier(barrierInfos[barrierInfoIndex]));
            barrierInfoIndex++;
          }
          switch (header->type)
          {
          case PacketType::RenderBlock:
          {
            auto block = header->data<gfxpacket::RenderBlock>().name.convertToMemView();
            currentBlock = std::string(block.data());
            if (pixevent)
            {
              PIXEndEvent(buffer);
              buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp.endIndex);
              queries.emplace_back(timestamp);
            }
            PIXBeginEvent(buffer, PIX_COLOR_INDEX(drawIndex), block.data());
            pixevent = true;
            
            timestamp = m_queryheap->allocate();
            buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp.beginIndex);
            //HIGAN_LOGi("RenderBlock %s\n", block.data());
            break;
          }
          case PacketType::PrepareForPresent:
          {
            break;
          }
          case PacketType::RenderpassBegin:
          {
            handle(device, buffer, header->data<gfxpacket::RenderPassBegin>());
            rpbegin = header;
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
              auto oldPipe = device->updatePipeline(packet.pipeline, rpbegin->data<gfxpacket::RenderPassBegin>());
              if (oldPipe)
              {
                m_freeResources->pipelines.push_back(oldPipe.value());
              }
              auto& pipe = device->allResources().pipelines[packet.pipeline];
              for (int i = 0; i < HIGANBANA_USABLE_SHADER_ARGUMENT_SETS; i++)
                m_boundSets[i] = {};
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
              auto oldPipe = device->updatePipeline(packet.pipeline);
              if (oldPipe)
              {
                m_freeResources->pipelines.push_back(oldPipe.value());
              }
              auto& pipe = device->allResources().pipelines[packet.pipeline];
              for (int i = 0; i < HIGANBANA_USABLE_SHADER_ARGUMENT_SETS; i++)
                m_boundSets[i] = {};
              buffer->SetComputeRootSignature(pipe.root.Get());
              buffer->SetPipelineState(pipe.pipeline.Get());
              boundPipeline = packet.pipeline;
            }
            break;
          }
          case PacketType::ResourceBindingGraphics:
          {
            gfxpacket::ResourceBindingGraphics& packet = header->data<gfxpacket::ResourceBindingGraphics>();
            handleBindings(device, buffer, packet);
            break;
          }
          case PacketType::ResourceBindingCompute:
          {
            gfxpacket::ResourceBindingCompute& packet = header->data<gfxpacket::ResourceBindingCompute>();
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
            if (m_boundIndexBufferHandle != params.indexbuffer) {
              if (params.indexbuffer.type == ViewResourceType::BufferIBV) {
                auto& ibv = device->allResources().bufIBV[params.indexbuffer];
                m_ib = *ibv.ibv();
              }
              else {
                auto& ibv = device->allResources().dynSRV[params.indexbuffer];
                m_ib = ibv.indexBufferView();
              }
              buffer->IASetIndexBuffer(&m_ib);
              m_boundIndexBufferHandle = params.indexbuffer;
            }

            buffer->DrawIndexedInstanced(params.IndexCountPerInstance, params.instanceCount, params.StartIndexLocation, params.BaseVertexLocation, params.StartInstanceLocation);
            break;
          }
          case PacketType::Dispatch:
          {
            auto params = header->data<gfxpacket::Dispatch>();
            buffer->Dispatch(params.groups.x, params.groups.y, params.groups.z);
            break;
          }
          case PacketType::DynamicBufferCopy:
          {
            auto params = header->data<gfxpacket::DynamicBufferCopy>();
            auto dst = device->allResources().buf[params.dst].native();
            auto src = device->allResources().dynSRV[params.src];
            auto srcOffset = src.offset();
            buffer->CopyBufferRegion(dst, params.dstOffset, src.nativePtr(), srcOffset, params.numBytes);
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

            D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromTexture(texture.native(), params.tex.fullMipSize(), params.mip, params.slice);
            D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromDynamic(dynamic.nativePtr(), dynamic, params.srcbox.size().x, params.srcbox.size().y, texture.desc().desc.format);
            buffer->CopyTextureRegion(&dstLoc, params.dstPos.x, params.dstPos.y, params.dstPos.z, &srcLoc, nullptr);
            break;
          }
          case PacketType::TextureToBufferCopy:
          {
            auto params = header->data<gfxpacket::TextureToBufferCopy>();
            auto dstBuf = device->allResources().buf[params.dstBuffer];
            auto srcTex = device->allResources().tex[params.srcTexture];
            auto rowPitch = sizeFormatRowPitch(int2(params.width, params.height), params.format);

            D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromTexture(srcTex.native(), params.srcTexture.fullMipSize(), params.mip, params.slice);
            D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromBuffer(dstBuf.native(), params.dstOffset, rowPitch, params.width, params.height, params.format);
            buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            break;
          }
          case PacketType::BufferToTextureCopy:
          {
            auto params = header->data<gfxpacket::BufferToTextureCopy>();
            auto dstTex = device->allResources().tex[params.dstTexture];
            auto srcBuf = device->allResources().buf[params.srcBuffer];
            auto rowPitch = sizeFormatRowPitch(int2(params.width, params.height), params.format);

            D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromBuffer(srcBuf.native(), params.srcOffset, rowPitch, params.width, params.height, params.format);
            D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromTexture(dstTex.native(), params.dstTexture.fullMipSize(), params.mip, params.slice);
            buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            break;
          }
          case PacketType::TextureToTextureCopy:
          {
            auto params = header->data<gfxpacket::TextureToTextureCopy>();
            auto dst = device->allResources().tex[params.dst];
            auto src = device->allResources().tex[params.src];

            D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromTexture(src.native(), params.src.fullMipSize(), params.srcMip, params.srcSlice);
            D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromTexture(dst.native(), params.dst.fullMipSize(), params.dstMip, params.dstSlice);
            buffer->CopyTextureRegion(&dstLoc, params.dstPos.x, params.dstPos.y, params.dstPos.z, &srcLoc, nullptr);
            break;
          }
          case PacketType::RenderpassEnd:
          {
            buffer->EndRenderPass();
            break;
          }
          case PacketType::ReadbackTexture:
          {
            auto params = header->data<gfxpacket::ReadbackTexture>();
            auto dstBuf = device->allResources().rbbuf[params.dst];
            auto srcTex = device->allResources().tex[params.src];
            auto ss = params.srcbox.size();
            auto rowPitch = sizeFormatRowPitch(ss, params.format);

            D3D12_TEXTURE_COPY_LOCATION srcLoc = locationFromTexture(srcTex.native(), params.src.fullMipSize(), params.mip, params.slice);
            D3D12_TEXTURE_COPY_LOCATION dstLoc = locationFromBuffer(dstBuf.native(), 0, rowPitch, ss.x, ss.y, params.format);
            buffer->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
            break;
          }
          case PacketType::ReadbackBuffer:
          {
            // TODO: dst offset for not buffers
            auto params = header->data<gfxpacket::ReadbackBuffer>();
            auto& dst = device->allResources().rbbuf[params.dst];
            auto src = device->allResources().buf[params.src].native();
            buffer->CopyBufferRegion(dst.native(), dst.offset(), src, params.srcOffset, params.numBytes);
            break;
          }
          case PacketType::ReadbackShaderDebug:
          {
            auto params = header->data<gfxpacket::ReadbackShaderDebug>();
            auto& dst = device->allResources().rbbuf[params.dst];
            D3D12_RESOURCE_BARRIER barrier{D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER{device->m_shaderDebugBuffer.native(), 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE}};
            buffer->ResourceBarrier(1, &barrier);
            buffer->CopyBufferRegion(dst.native(), dst.offset(), device->m_shaderDebugBuffer.native(), 0, HIGANBANA_SHADER_DEBUG_WIDTH);
            uint clearVals[4] = {};
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            buffer->ResourceBarrier(1, &barrier);
            buffer->ClearUnorderedAccessViewUint(m_shaderDebugTable.gpu, device->m_shaderDebugTableCPU.cpu, device->m_shaderDebugBuffer.native(), clearVals, 0, nullptr);
            break;
          }
          //raytracing

          // reason for this packet is possible temporary buffer implementation later, have to delay address asking.
          case PacketType::RaytracingWriteGpuAddrToInstanceCPU:
          {
            auto params = header->data<gfxpacket::RaytracingWriteGpuAddrToInstanceCPU>();
            auto dst = device->allResources().dynSRV[params.dst];
            auto& addrToWrite = device->allResources().buf[params.addrToWrite];
            // just copy on cpu timeline the data to correct place
            uint structSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
            uint offsetToGPUAddr = structSize - sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
            uint offset = params.instanceId * structSize + offsetToGPUAddr;
            auto ptr = addrToWrite.native()->GetGPUVirtualAddress();
            memcpy(dst.data()+offset, &ptr, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
            break;
          }
          case PacketType::RaytracingWriteGpuAddrToInstanceGPU:
          {
            HIGAN_ASSERT(false, "unimplemented");
            break;
          }
          case PacketType::BuildBLASTriangle:
          {
            auto& params = header->data<gfxpacket::BuildBLASTriangle>();
            vector<D3D12_RAYTRACING_GEOMETRY_DESC> descs;
            for (auto&& view : params.triangles.convertToMemView()) {
              D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
              geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
              auto ibuffer = device->allResources().buf[view.indexBuffer];
              geometryDesc.Triangles.IndexBuffer = ibuffer.native()->GetGPUVirtualAddress()+view.indexByteOffset;
              geometryDesc.Triangles.IndexCount = view.indexCount;
              geometryDesc.Triangles.IndexFormat = formatTodxFormat(view.indexFormat).view;
              //geometryDesc.Triangles.Transform3x4 = device->allResources().buf[view.transformBuffer].native()->GetGPUVirtualAddress()+view.transformByteOffset;
              geometryDesc.Triangles.Transform3x4 = 0;
              geometryDesc.Triangles.VertexFormat = formatTodxFormat(view.vertexFormat).view;
              geometryDesc.Triangles.VertexCount = view.vertexCount;
              auto vbuffer = device->allResources().buf[view.vertexBuffer];
              geometryDesc.Triangles.VertexBuffer.StartAddress = vbuffer.native()->GetGPUVirtualAddress()+view.vertexByteOffset;
              geometryDesc.Triangles.VertexBuffer.StrideInBytes = view.vertexStride;
              if ((view.flags == desc::RaytracingGeometryFlag::Opaque))
                geometryDesc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
              if ((view.flags == desc::RaytracingGeometryFlag::NoDuplicateAnyHitInvocation))
                geometryDesc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
              descs.push_back(geometryDesc);
            }
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blinputs = {};
            blinputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            blinputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
            blinputs.NumDescs = descs.size();
            blinputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            blinputs.pGeometryDescs = descs.data();
             D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
            {
                bottomLevelBuildDesc.Inputs = blinputs;
                bottomLevelBuildDesc.ScratchAccelerationStructureData = device->allResources().buf[params.scratch].native()->GetGPUVirtualAddress();
                bottomLevelBuildDesc.DestAccelerationStructureData = device->allResources().buf[params.dst].native()->GetGPUVirtualAddress();
            }
            buffer->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
            break;
          }
          case PacketType::BuildTLAS:
          {
            auto params = header->data<gfxpacket::BuildTLAS>();
            HIGAN_ASSERT(false, "unimplemented");
            break;
          }
          default:
            break;
          }
          drawIndex++;
        }
      }
      if (pixevent)
      {
        PIXEndEvent(buffer);
        buffer->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp.endIndex);
        queries.emplace_back(timestamp);
        
        readback = m_readback->allocate(m_queryheap->size()*m_queryheap->counterSize());
        buffer->ResolveQueryData(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, 0, UINT(m_queryheap->size()), m_readback->native(), readback.offset);
      }
    }
    // implementations
    void DX12CommandList::reserveConstants(size_t expectedTotalBytes) {
      auto newBlock = m_constants->allocate(expectedTotalBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      HIGAN_ASSERT(newBlock && newBlock.block.offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "What!");
      m_freeResources->uploadBlocks.push_back(newBlock);
      m_constantsAllocator = UploadLinearAllocatorGPU(newBlock);
    }
    void DX12CommandList::beginConstantsDmaList(std::shared_ptr<prototypes::DeviceImpl>) {
      HIGAN_CPU_BRACKET("reset");
      m_buffer->resetList();
      PIXBeginEvent(m_buffer->list(), PIX_COLOR_INDEX(0), "Copy Constants");
      auto timestamp = m_queryheap->allocate();
      queries.push_back(timestamp);
      m_buffer->list()->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, timestamp.beginIndex);
    }
    void DX12CommandList::addConstants(CommandBufferImpl* buffer) {
      DX12CommandList* other = static_cast<DX12CommandList*>(buffer);
      size_t copied = 0;
      for (auto& constants : other->m_freeResources->uploadBlocks)
      {
        m_buffer->list()->CopyBufferRegion(constants.m_resGpuPtr, constants.block.offset, constants.m_resSrcPtr, constants.block.offset, constants.block.size);
        copied += constants.block.size;
      }
      //HIGAN_LOGi("copy %zubytes to gpu\n", copied);
    }
    void DX12CommandList::endConstantsDmaList() {
      PIXEndEvent(m_buffer->list());
      m_buffer->list()->EndQuery(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, queries.back().endIndex);
      readback = m_readback->allocate(m_queryheap->size()*m_queryheap->counterSize());
      m_buffer->list()->ResolveQueryData(m_queryheap->native(), D3D12_QUERY_TYPE_TIMESTAMP, 0, UINT(m_queryheap->size()), m_readback->native(), readback.offset);
      m_buffer->closeList();
    }
    void DX12CommandList::fillWith(std::shared_ptr<prototypes::DeviceImpl> device, MemView<backend::CommandBuffer*>& buffers, BarrierSolver& solver)
    {
      HIGAN_CPU_BRACKET("compile to DX12 CmdList");
      {
        HIGAN_CPU_BRACKET("reset");
        m_buffer->resetList();
      }
      {
        HIGAN_CPU_BRACKET("compile");
        DX12Device* dev = static_cast<DX12Device*>(device.get());
        addCommands(dev, m_buffer->list(), buffers, solver);
      }
      m_buffer->closeList();
    }

    bool DX12CommandList::readbackTimestamps(std::shared_ptr<prototypes::DeviceImpl> device, vector<GraphNodeTiming>& nodes)
    {
      if (queries.empty())
        return false;
      HIGAN_CPU_FUNCTION_SCOPE();
      m_readback->map();
      auto view = m_readback->getView(readback);
      auto ticksPerSecond = m_queryheap->getGpuTicksPerSecond();
      auto properView = reinterpret_memView<uint64_t>(view);
      DX12Device* dev = static_cast<DX12Device*>(device.get());
      auto queueTimeOffset = dev->m_graphicsTimeOffset;
      if (m_type == QueueType::Dma)
        queueTimeOffset = dev->m_dmaTimeOffset;
      else if (m_type == QueueType::Compute)
        queueTimeOffset = dev->m_computeTimeOffset;

      if (nodes.size() >= queries.size())
      {
        for (int i = 0; i < queries.size(); ++i)
        {
          auto query = queries[i];
          auto countParts = [ticksPerSecond](uint64_t timestamp){
            return uint64_t((double(timestamp)/double(ticksPerSecond))*1000000000ull);
          };
          uint64_t begin = countParts(properView[query.beginIndex]);
          uint64_t end = countParts(properView[query.endIndex]);
          auto& node = nodes[i];
          node.gpuTime.begin = begin + queueTimeOffset;
          node.gpuTime.end = end + queueTimeOffset;
        }
      }
      m_readback->unmap();
      m_readback->reset();
      m_queryheap->reset();
      return true;
    }
  }
}
#endif