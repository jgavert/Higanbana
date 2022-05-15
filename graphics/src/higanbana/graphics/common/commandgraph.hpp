#pragma once
#include "higanbana/graphics/common/commandlist.hpp"
#include "higanbana/graphics/common/semaphore.hpp"
#include "higanbana/graphics/common/shader_arguments_binding.hpp"
#include "higanbana/graphics/common/swapchain.hpp"
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/desc/timing.hpp"
#include <higanbana/core/datastructures/vector.hpp>
#include <higanbana/core/system/memview.hpp>
#include <higanbana/core/global_debug.hpp>
#include <higanbana/core/system/SequenceTracker.hpp>
#include <higanbana/core/entity/bitfield.hpp>

#include <higanbana/core/math/utils.hpp>

#include <string>
#include "higanbana/graphics/common/readback.hpp"
#include <memory>

namespace higanbana
{
  class CommandGraphNode
  {
  private:
    std::shared_ptr<CommandList> list;
    std::string name;
    friend struct backend::DeviceGroupData;
    friend struct CommandNodeVector;
    friend struct CommandGraph;
    QueueType type;
    int gpuId;
    std::shared_ptr<backend::SemaphoreImpl> acquireSemaphore;
    bool preparesPresent = false;
    std::shared_ptr<backend::ConstantsAllocator> constantsAllocator;
    vector<backend::LinearConstantsAllocator*> freeAllocators;
    backend::LinearConstantsAllocator* linearConstantsAllocator = nullptr;

    size_t usedConstantMemory = 0;

    uint3 m_currentBaseGroups;

    DynamicBitfield m_filterArguments;
    DynamicBitfield m_referencedBuffers;
    DynamicBitfield m_referencedTextures;

    vector<ResourceHandle> consumesSharedResource; // reads from a SharedResource which likely another gpu produces.
    vector<ResourceHandle> producesSharedResources; // writes to another gpu's resource

    vector<ReadbackPromise> m_readbackPromises;
    GraphNodeTiming timing;
    
    // dummy new constant writer to right place


    const DynamicBitfield& refBuf() const
    {
      return m_referencedBuffers;
    }
    const DynamicBitfield& refTex() const
    {
      return m_referencedTextures;
    }

    void addReadShared(ResourceHandle handle) {
      if (handle.shared()) {
        consumesSharedResource.push_back(handle);
      }
    }
    void addWriteShared(ResourceHandle handle) {
      if (handle.shared()) {
        producesSharedResources.push_back(handle);
      }
    }

    template <typename ViewType>
    void addViewBuf(ViewType type)
    {
      auto h = type.handle();
      m_referencedBuffers.setBit(h.resourceHandle().id);
    }

    template <typename ViewType>
    void addViewTex(ViewType type)
    {
      auto h = type.handle();
      m_referencedTextures.setBit(h.resourceHandle().id);
    }
    
    void addRefArgs(MemView<ShaderArguments> args)
    {
      for (auto&& arg : args)
      {
        if (m_filterArguments.checkBit(arg.handle().id))
          continue;
        m_filterArguments.setBit(arg.handle().id);
        const auto& thing = arg.refBuffers();
        m_referencedBuffers = m_referencedBuffers.unionFields(thing);
        const auto& thing2 = arg.refTextures();
        m_referencedTextures = m_referencedTextures.unionFields(thing2);
      }
    }

    void addConstantSize(size_t size) {
      usedConstantMemory += ((size + 255) & (~255)); // 256 bytes sizes blocks
    }

    [[nodiscard]] uint64_t memcpyConstants(MemView<uint8_t> constants) {
      // todo memcpy somehow somewhere
      addConstantSize(constants.size_bytes());
#if defined(HIGANBANA_DIRECT_CONSTANT_COPIES)
      backend::ConstantsBlock block;
      if (linearConstantsAllocator)
        block = linearConstantsAllocator->allocate(constants.size_bytes());// D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      if (!block.valid())
      {
        //HIGAN_ASSERT(false, "never should come here");
        auto newBlock = constantsAllocator->allocate(256*4*256);//D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * 256 * 4, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        HIGAN_ASSERT(newBlock, "what!");// && newBlock.block.offset % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0, "What!");
        freeAllocators.push_back(newBlock);
        linearConstantsAllocator = newBlock;
        block = linearConstantsAllocator->allocate(constants.size_bytes());//, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      }

      HIGAN_ASSERT(block.valid(), "What!");
      memcpy(block.ptr, constants.data(), constants.size_bytes());
      addConstantSize(constants.size_bytes());
      return block.constantBlock;
#else
      return 0;
#endif
    }

  public:
    CommandGraphNode() {}
    CommandGraphNode(std::string name, QueueType type, int gpuId, CommandList&& buffer, std::shared_ptr<backend::ConstantsAllocator> constantAllocator)
      : list(std::make_shared<CommandList>(std::move(buffer)))
      , name(name)
      , type(type)
      , gpuId(gpuId)
      , constantsAllocator(constantAllocator)
      , timing({})
    {
      list->renderTask(name);
      timing.nodeName = name;
      timing.cpuTime.start();
    }

    void acquirePresentableImage(Swapchain& swapchain)
    {
      acquireSemaphore = swapchain.impl()->acquireSemaphore();
    }

    /*
    void clearRT(TextureRTV& rtv, float4 color)
    {
      addViewTex(rtv);
      list->clearRT(rtv, color);
    }
    */

    void prepareForPresent(TextureRTV& rtv)
    {
      list->prepareForPresent(rtv);
      preparesPresent = true;
    }

    void renderpass(Renderpass& pass, TextureRTV& rtv)
    {
      addViewTex(rtv);
      list->renderpass(pass, {rtv}, {});
    }

    void renderpass(Renderpass& pass, TextureRTV& rtv, TextureDSV& dsv)
    {
      addViewTex(rtv);
      addViewTex(dsv);
      list->renderpass(pass, {rtv}, dsv);
    }

    void renderpass(Renderpass& pass, TextureRTV& rtv, TextureRTV& rtv2, TextureDSV& dsv)
    {
      addViewTex(rtv);
      addViewTex(rtv2);
      addViewTex(dsv);
      TextureRTV temp_rtvs[2] = {rtv, rtv2};
      list->renderpass(pass, temp_rtvs, dsv);
    }

    void renderpass(Renderpass& pass, TextureDSV& dsv)
    {
      addViewTex(dsv);
      list->renderpass(pass, {}, dsv);
    }

    void endRenderpass()
    {
      list->renderpassEnd();
    }

    // bindings

    ShaderArgumentsBinding bind(GraphicsPipeline& pipeline)
    {
      list->bindPipeline(pipeline);

      return ShaderArgumentsBinding(pipeline);
    }

    ShaderArgumentsBinding bind(ComputePipeline& pipeline)
    {
      list->bindPipeline(pipeline);
      m_currentBaseGroups = pipeline.descriptor.shaderGroups;

      return ShaderArgumentsBinding(pipeline);
    }

    void setScissor(int2 tl, int2 br)
    {
      list->setScissorRect(tl, br);
    }

    void setShadingRate(ShadingRate rate)
    {
      list->setShadingRate(rate);
    }

    // draws/dispatches

    void draw(
      ShaderArgumentsBinding& binding,
      unsigned vertexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned startVertex = 0,
      unsigned startInstance = 0)
    {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      HIGAN_ASSERT(vertexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. draw %d %d %d %d", vertexCountPerInstance, instanceCount, startVertex, startInstance);
      list->draw(vertexCountPerInstance, instanceCount, startVertex, startInstance);
      timing.draws++;
    }

    void drawIndexed(
      ShaderArgumentsBinding& binding,
      DynamicBufferView view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      HIGAN_ASSERT(IndexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. drawIndexed %d %d %d %d %d", IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      list->drawDynamicIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      timing.draws++;
    }

    void drawIndexed(
      ShaderArgumentsBinding& binding,
      BufferIBV view,
      unsigned IndexCountPerInstance,
      unsigned instanceCount = 1,
      unsigned StartIndexLocation = 0,
      int BaseVertexLocation = 0,
      unsigned StartInstanceLocation = 0)
    {
      HIGAN_ASSERT(IndexCountPerInstance > 0, "index count 0 doesn't draw anything");
      HIGAN_ASSERT(instanceCount > 0, "instance count 0 doesn't draw anything");
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());

      addReadShared(view.handle().resourceHandle());
      m_referencedBuffers.setBit(view.handle().id);

      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      HIGAN_ASSERT(IndexCountPerInstance > 0 && instanceCount > 0, "Index/instance count was 0, nothing would be drawn. drawIndexed %d %d %d %d %d", IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      list->drawIndexed(view, IndexCountPerInstance, instanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
      timing.draws++;
    }

    void dispatchThreads(
      ShaderArgumentsBinding& binding, uint3 groups)
    {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      list->bindComputeResources(binding.bShaderArguments(), binding.bConstants(), block);
      unsigned x = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.x), static_cast<uint64_t>(m_currentBaseGroups.x)));
      unsigned y = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.y), static_cast<uint64_t>(m_currentBaseGroups.y)));
      unsigned z = static_cast<unsigned>(divideRoundUp(static_cast<uint64_t>(groups.z), static_cast<uint64_t>(m_currentBaseGroups.z)));
      HIGAN_ASSERT(x*y*z > 0, "One of the parameters was 0, no threadgroups would be launched. dispatch %d %d %d", x, y, z);
      list->dispatch(uint3(x,y,z));
      timing.dispatches++;
    }

    void dispatch(
      ShaderArgumentsBinding& binding, uint3 groups)
    {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      list->bindComputeResources(binding.bShaderArguments(), binding.bConstants(), block);
      HIGAN_ASSERT(groups.x*groups.y*groups.z > 0, "One of the parameters was 0, no threadgroups would be launched. dispatch %d %d %d", groups.x, groups.y, groups.z);
      list->dispatch(groups);
      timing.dispatches++;
    }

    void dispatchMesh(ShaderArgumentsBinding& binding, uint3 groups)
    {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      HIGAN_ASSERT(groups.x*groups.y*groups.z > 0, "One of the parameters was 0, no threadgroups would be launched. dispatch %d %d %d", groups.x, groups.y, groups.z);
      HIGAN_ASSERT(groups.y == 1 && groups.z == 1, "Only x group is read for now, because of vulkan limitations");
      list->dispatchMesh(groups.x);
    }

    void drawIndirect(ShaderArgumentsBinding& binding, uint maxCommands, const BufferSRV& indirect, BufferSRV count = BufferSRV(), uint countOffsetBytes = 0) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      if (count.buffer().handle().id != ResourceHandle::InvalidId){
        addReadShared(count.buffer().handle());
        m_referencedBuffers.setBit(count.buffer().handle().id);
      }
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->drawIndirect(maxCommands, indirect, count, countOffsetBytes);
    }

    void drawIndexedIndirect(ShaderArgumentsBinding& binding, DynamicBufferView& ibv, uint maxCommands, const BufferSRV& indirect, BufferSRV count = BufferSRV(), uint countOffsetBytes = 0) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      if (count.buffer().handle().id != ResourceHandle::InvalidId){
        addReadShared(count.buffer().handle());
        m_referencedBuffers.setBit(count.buffer().handle().id);
      }
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->drawIndexedIndirect(ibv, maxCommands, indirect, count, countOffsetBytes);
    }

    void drawIndexedIndirect(ShaderArgumentsBinding& binding, const BufferIBV& ibv, uint maxCommands, const BufferSRV& indirect, BufferSRV count = BufferSRV(), uint countOffsetBytes = 0) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(ibv.buffer().handle());
      m_referencedBuffers.setBit(ibv.buffer().handle().id);
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      if (count.buffer().handle().id != ResourceHandle::InvalidId){
        addReadShared(count.buffer().handle());
        m_referencedBuffers.setBit(count.buffer().handle().id);
      }
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->drawIndexedIndirect(ibv, maxCommands, indirect, count, countOffsetBytes);
    }

    void dispatchIndirect(ShaderArgumentsBinding& binding, const BufferSRV& indirect) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      list->bindComputeResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->dispatchIndirect(indirect);
    }

    void dispatchRaysIndirect(ShaderArgumentsBinding& binding, uint maxCommands, const BufferSRV& indirect, BufferSRV count = BufferSRV(), uint countOffsetBytes = 0) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      if (count.buffer().handle().id != ResourceHandle::InvalidId){
        m_referencedBuffers.setBit(count.buffer().handle().id);
        addReadShared(count.buffer().handle());
      }
      list->bindComputeResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->dispatchRaysIndirect(maxCommands, indirect, count, countOffsetBytes);
    }

    void dispatchMeshIndirect(ShaderArgumentsBinding& binding, uint maxCommands, const BufferSRV& indirect, BufferSRV count = BufferSRV(), uint countOffsetBytes = 0) {
      addRefArgs(binding.bShaderArguments());
      auto block = memcpyConstants(binding.bConstants());
      addReadShared(indirect.buffer().handle());
      m_referencedBuffers.setBit(indirect.buffer().handle().id);
      if (count.buffer().handle().id != ResourceHandle::InvalidId){
        m_referencedBuffers.setBit(count.buffer().handle().id);
        addReadShared(count.buffer().handle());
      }
      list->bindGraphicsResources(binding.bShaderArguments(), binding.bConstants(), block);
      list->dispatchMeshIndirect(maxCommands, indirect, count, countOffsetBytes);
    }

    void copy(Buffer target, int64_t elementOffset, Texture source, Subresource sub)
    {
      m_referencedBuffers.setBit(target.handle().id);
      auto mipDim = calculateMipDim(source.desc().size(), sub.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list->copy(target, elementOffset, source, sub, srcBox);
    }

    void copy(Texture target, Texture source)
    {
      m_referencedTextures.setBit(target.handle().id);
      m_referencedTextures.setBit(source.handle().id);
      HIGAN_ASSERT(false, "Not implemented.");
      //auto lol = source.dependency();
      /*
      SubresourceRange range{
        static_cast<int16_t>(lol.mip()),
        static_cast<int16_t>(lol.mipLevels()),
        static_cast<int16_t>(lol.slice()),
        static_cast<int16_t>(lol.arraySize()) };
      list.copy(target, source, range);
      */
    }

    void copy(Texture target, Subresource dstSub, int3 dstPos, Texture source, Subresource srcSub, Box srcBox)
    {
      addWriteShared(target.handle());
      addReadShared(source.handle());
      m_referencedTextures.setBit(target.handle().id);
      m_referencedTextures.setBit(source.handle().id);
      list->copy(target, dstSub, dstPos, source, srcSub, srcBox);
    }

    void copy(Buffer target, Buffer source)
    {
      addWriteShared(target.handle());
      addReadShared(source.handle());
      m_referencedBuffers.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      list->copy(target, 0, source, 0, source.desc().desc.width);
    }

    void copy(Buffer target, uint targetElementOffset, Buffer source, uint sourceElementOffset = 0, int sourceElementsToCopy = -1)
    {
      addWriteShared(target.handle());
      addReadShared(source.handle());
      if (sourceElementsToCopy == -1)
      {
        sourceElementsToCopy = source.desc().desc.width;
      }
      m_referencedBuffers.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      list->copy(target, targetElementOffset, source, sourceElementOffset, sourceElementsToCopy);
    }

    void copy(Buffer target, DynamicBufferView source)
    {
      addWriteShared(target.handle());
      auto elements = source.logicalSize() / source.elementSize();
      m_referencedBuffers.setBit(target.handle().id);
      list->copy(target, 0, source, 0, elements);
    }
    void copyBuffer(Buffer target, uint destElement, DynamicBufferView source, uint srcElement)
    {
      addWriteShared(target.handle());
      auto elements = source.logicalSize() / source.elementSize();
      m_referencedBuffers.setBit(target.handle().id);
      list->copy(target, destElement, source, srcElement, elements-srcElement);
    }

    void copy(Buffer target, uint targetElementOffset, DynamicBufferView source, uint sourceElementOffset = 0, int sourceElementsToCopy = -1)
    {
      addWriteShared(target.handle());
      if (sourceElementsToCopy == -1)
      {
        sourceElementsToCopy = source.logicalSize() / source.elementSize();
      }
      m_referencedBuffers.setBit(target.handle().id);
      list->copy(target, targetElementOffset, source, sourceElementOffset, sourceElementsToCopy);
    }

    void copy(Texture target, Buffer source, Subresource sub)
    {
      addWriteShared(target.handle());
      addReadShared(source.handle());
      m_referencedTextures.setBit(target.handle().id);
      m_referencedBuffers.setBit(source.handle().id);
      HIGAN_ASSERT(false, "Not implemented");
    }

    void copy(Texture target, DynamicBufferView source, Subresource sub)
    {
      addWriteShared(target.handle());
      m_referencedTextures.setBit(target.handle().id);
      auto size = target.size3D();
      list->updateTexture(target, sub, int3(0,0,0), source, Box(uint3(0,0,0), uint3(size)));
    }

    void copy(Texture target, Subresource dstSub, int3 dstPos, DynamicBufferView source, Box srcBox)
    {
      addWriteShared(target.handle());
      m_referencedTextures.setBit(target.handle().id);
      list->updateTexture(target, dstSub, dstPos, source, srcBox);
    }

    void copy(Buffer dst, size_t dstOffset, Texture src, Subresource sub) {
      addWriteShared(dst.handle());
      addReadShared(src.handle());
      m_referencedBuffers.setBit(dst.handle().id);
      m_referencedTextures.setBit(src.handle().id);
      list->copy(dst, dstOffset, src, sub);
    }

    void copy(Texture dst, Subresource sub, Buffer src, size_t srcOffset) {
      addWriteShared(dst.handle());
      addReadShared(src.handle());
      m_referencedTextures.setBit(dst.handle().id);
      m_referencedBuffers.setBit(src.handle().id);
      list->copy(dst, sub, src, srcOffset);
    }

    ReadbackFuture readback(Texture tex, Subresource resource = Subresource())
    {
      addReadShared(tex.handle());
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedTextures.setBit(tex.handle().id);
      auto mipDim = calculateMipDim(tex.desc().size(), resource.mipLevel);
      Box srcBox(uint3(0), mipDim);
      list->readback(tex, resource, srcBox);
      return promise.future();
    }

    ReadbackFuture readback(Buffer buffer, int offset = -1, int elements = -1)
    {
      addReadShared(buffer.handle());
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedBuffers.setBit(buffer.handle().id);

      if (offset == -1)
      {
        offset = 0;
      }

      if (elements == -1)
      {
        elements = buffer.desc().desc.width - offset;
      }

      HIGAN_ASSERT(offset+elements <= buffer.desc().desc.width, "elements should be less than total elements");

      auto stride = buffer.desc().desc.stride;
      list->readback(buffer, offset*stride, elements*stride);
      return promise.future();
    }

    ReadbackFuture readbackBytes(Buffer buffer, unsigned offsetBytes, unsigned sizeBytes = 0)
    {
      addReadShared(buffer.handle());
      auto promise = ReadbackPromise({nullptr, std::make_shared<std::promise<ReadbackData>>()});
      m_readbackPromises.push_back(promise);
      m_referencedBuffers.setBit(buffer.handle().id);
      auto stride = buffer.desc().desc.stride;
      if (sizeBytes == 0)
      {
        sizeBytes = buffer.desc().desc.width*stride - offsetBytes;
      }
      HIGAN_ASSERT(offsetBytes+sizeBytes <= buffer.desc().desc.width*stride, "totalBytes should't exceed buffer. offset: %u size: %u, max size: %u", offsetBytes, sizeBytes, buffer.desc().desc.width*stride);

      list->readback(buffer, offsetBytes, sizeBytes);
      return promise.future();
    }

    void raytracingWriteGPUAddrToInstanceDesc(DynamicBufferView dst, const BufferRTAS& addrToWrite, uint instanceIndex) {
      list->raytracingWriteGPUAddrToInstanceDescCPU(dst, addrToWrite, instanceIndex);
    }

    void raytracingWriteGPUAddrToInstanceDesc(Buffer& dst, const BufferRTAS& addrToWrite, uint instanceIndex) {
      addWriteShared(dst.handle());
      m_referencedBuffers.setBit(dst.handle().id);
      list->raytracingWriteGPUAddrToInstanceDescGPU(dst, addrToWrite, instanceIndex);
    }

    void buildAccelerationStructure(BufferRTAS& dst, desc::RaytracingAccelerationStructureInputs& asInputs, Buffer& scratchBuffer) {
      addWriteShared(scratchBuffer.handle());
      m_referencedBuffers.setBit(scratchBuffer.handle().id);
      addWriteShared(dst.buffer().handle());
      m_referencedBuffers.setBit(dst.buffer().handle().id);
      if (asInputs.desc.type == desc::AccelerationStructureType::BottomLevel) {
        list->buildBottomLevelAccelerationStructure(dst, asInputs, scratchBuffer);
      }
      else if (asInputs.desc.type == desc::AccelerationStructureType::TopLevel) {
        list->buildTopLevelAccelerationStructure(dst, asInputs, scratchBuffer);
      }
      else {
        HIGAN_ASSERT(false, "unimplemented");
      }
    }
  };

  class CommandNodeVector
  {
    CommandBufferPool& m_buffers;
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    std::shared_ptr<backend::ConstantsAllocator> m_constantsAllocator;
    friend struct CommandGraph;
  public:
    CommandNodeVector(CommandBufferPool& buffers, std::shared_ptr<backend::ConstantsAllocator> constantAllocator)
      : m_buffers(buffers)
      , m_nodes{ std::make_shared<vector<CommandGraphNode>>() }
      , m_constantsAllocator(constantAllocator)
    {
    }

    CommandGraphNode createPass(std::string name, QueueType type = QueueType::Graphics, int gpu = 0)
    {
      return CommandGraphNode(name, type, gpu, m_buffers.allocate(), m_constantsAllocator);
    }

    void addPass(CommandGraphNode&& node)
    {
      node.timing.cpuTime.stop();
      node.timing.cpuSizeBytes = node.list->sizeBytesUsed();
      m_nodes->emplace_back(std::move(node));
    }
  };
  class CommandGraph
  {
    SeqNum m_sequence;
    SubmitTiming m_timing;
    CommandBufferPool& m_buffers;
    std::shared_ptr<vector<CommandGraphNode>> m_nodes;
    std::shared_ptr<backend::ConstantsAllocator> m_constantsAllocator;
    friend struct backend::DeviceGroupData;
  public:
    CommandGraph(SeqNum seq, CommandBufferPool& buffers, std::shared_ptr<backend::ConstantsAllocator> constantAllocator)
      : m_sequence(seq)
      , m_buffers(buffers)
      , m_nodes{ std::make_shared<vector<CommandGraphNode>>() }
      , m_constantsAllocator(constantAllocator)
    {
      m_timing.timeBeforeSubmit.start();
    }

    CommandGraphNode createPass(std::string name, QueueType type = QueueType::Graphics, int gpu = 0)
    {
      return CommandGraphNode(name, type, gpu, m_buffers.allocate(), m_constantsAllocator);
    }

    void addPass(CommandGraphNode&& node)
    {
      node.timing.cpuTime.stop();
      node.timing.cpuSizeBytes = node.list->sizeBytesUsed();
      m_nodes->emplace_back(std::move(node));
    }

    CommandNodeVector localThreadVector() {
      return CommandNodeVector(m_buffers, m_constantsAllocator);
    }
    
    void addVectorOfPasses(CommandNodeVector&& vec)
    {
      for (auto&& node : *vec.m_nodes) {
        m_nodes->emplace_back(std::move(node));
      }
    }
  };
}