#include "higanbana/graphics/common/barrier_solver.hpp"
#include <higanbana/core/global_debug.hpp>

namespace higanbana
{
  namespace backend
  {
    int BarrierSolver::addDrawCall()
    {
      return drawCallsAdded++; 
    }
    void BarrierSolver::addBuffer(int drawCallIndex, ViewResourceHandle buffer, ResourceState access)
    {
      m_bufferCache[buffer.resourceHandle()].state = ResourceState(backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, QueueType::Unknown);
      m_bufferCache[buffer.resourceHandle()].lastBarrierIndex = -1; 
      m_jobs.push_back(DependencyPacket{drawCallIndex, buffer, access});
      m_uniqueBuffers.insert(buffer.resourceHandle());
    }
    void BarrierSolver::addTexture(int drawCallIndex, ViewResourceHandle texture, ResourceState access)
    {
      const ResourceState exampleState = ResourceState(backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, QueueType::Unknown);
      auto& states = m_imageCache[texture.resourceHandle()].states;
      auto& gstate = (*m_textureStates)[texture.resourceHandle()];
      auto size = gstate.states.size();
      m_imageCache[texture.resourceHandle()].mips = gstate.mips;
      if (states.size() != size)
        states.resize(size);
      for (auto&& state : states) {
        state = exampleState;
      }
        
      m_jobs.push_back(DependencyPacket{drawCallIndex, texture, access});
      m_uniqueTextures.insert(texture.resourceHandle());
    }
    void BarrierSolver::reset(HandleVector<ResourceState>* buffers, HandleVector<TextureResourceState>* textures)
    {
      m_bufferStates = buffers;
      m_textureStates = textures;
      m_uniqueBuffers.clear();
      m_uniqueTextures.clear();
      m_jobs.clear();
      m_drawBarries.clear();
      bufferBarriers.clear();
      imageBarriers.clear();
      // should not need to clear caches...
      //m_bufferCache.clear();
      //m_imageCache.clear();
      drawCallsAdded = 0;
    }

    void BarrierSolver::localBarrierPass1(bool allowCommonOptimization)
    {
      int jobsSize = static_cast<int>(m_jobs.size());
      int jobIndex = 0;
      int bufferBarrierOffsets = 0;
      int imageBarrierOffsets = 0;

      int drawIndex = -1;

      BarrierInfo currentInfo{};

      while (jobIndex < jobsSize)
      {
        int draw = m_jobs[jobIndex].drawIndex;
        if (currentInfo.bufferOffset != bufferBarrierOffsets || currentInfo.imageOffset != imageBarrierOffsets)
        {
          currentInfo.bufferCount = bufferBarrierOffsets - currentInfo.bufferOffset;
          currentInfo.imageCount = imageBarrierOffsets - currentInfo.imageOffset;
          currentInfo.drawcall = drawIndex;
          m_drawBarries.push_back(currentInfo);
          currentInfo.bufferOffset = bufferBarrierOffsets;
          currentInfo.imageOffset = imageBarrierOffsets;
        }
        drawIndex = draw;

        while (jobIndex < jobsSize && m_jobs[jobIndex].drawIndex == draw)
        {
          auto& job = m_jobs[jobIndex];
          auto jobResAccess = job.nextAccess;
          if (job.resource.resourceHandle().type == ResourceType::Buffer)
          {
            auto& resource = m_bufferCache[job.resource.resourceHandle()];
            auto lastAccess = resource.state;
            auto lastWriting = lastAccess.usage == AccessUsage::ReadWrite || lastAccess.usage == AccessUsage::Write;
            auto nextWriting = jobResAccess.usage == AccessUsage::ReadWrite || jobResAccess.usage == AccessUsage::Write;
            auto readread = !lastWriting && !nextWriting;
            auto uavFlush = lastAccess.stage != AccessStage::Rendertarget && lastWriting == nextWriting;
            if (lastAccess.stage == AccessStage::AccelerationStructure && lastAccess.stage == jobResAccess.stage && lastWriting)
              uavFlush = true;
            if (lastAccess.stage == AccessStage::AccelerationStructure)
              HIGAN_ASSERT(jobResAccess.stage == lastAccess.stage, "not allowed to change accelerationstructure from acceleration structure");
            auto isDifferentFromCommon = (lastAccess.usage != AccessUsage::Unknown && lastAccess.stage != AccessStage::Common) || !allowCommonOptimization;

            if (readread) {
              // combine
              jobResAccess.stage = lastAccess.stage | jobResAccess.stage;
            }

            if (lastAccess.queue_index != jobResAccess.queue_index && jobResAccess.stage == AccessStage::Common)
            {
              // acquire/release barrier
              if (lastAccess.queue_index != QueueType::Unknown )
              {
                bool acquire = jobResAccess.usage == AccessUsage::Read;
                auto src = jobResAccess;
                src.usage = AccessUsage::Unknown;
                src.queue_index = lastAccess.queue_index;
                auto dst = jobResAccess;
                dst.usage = AccessUsage::Unknown;
                if (acquire)
                {
                  src.queue_index = dst.queue_index;
                  dst.queue_index = lastAccess.queue_index;
                }
                else
                {
                  resource.state.queue_index = jobResAccess.queue_index;
                }
                bufferBarriers.emplace_back(BufferBarrier{src, dst, job.resource.resourceHandle()});
                ++bufferBarrierOffsets;
              }
            }
            else if ((isDifferentFromCommon &&  (jobResAccess.stage != lastAccess.stage || jobResAccess.usage != lastAccess.usage)) || uavFlush)
            {
              auto src = lastAccess;
              src.queue_index = QueueType::Unknown;
              auto dst = jobResAccess;
              dst.queue_index = QueueType::Unknown;
              bool handled = false;
              if (readread && resource.lastBarrierIndex != -1) {
                auto& b = bufferBarriers[resource.lastBarrierIndex];
                b.after = dst;
                handled = true;
              }
              if (!handled) {
                bufferBarriers.emplace_back(BufferBarrier{src, dst, job.resource.resourceHandle()});
                resource.lastBarrierIndex = bufferBarrierOffsets;
                ++bufferBarrierOffsets;
              }
              resource.state = dst;
            }
            else
            {
              resource.state = job.nextAccess;
            }
          }
          else
          {
            auto& resource = m_imageCache[job.resource.resourceHandle()];
            {
              // check if we are doing queue transfer first, if yes, skip res
              if (job.nextAccess.queue_index != resource.states[0].queue_index && job.nextAccess.stage == AccessStage::Common)
              {
                if (resource.states[0].queue_index != QueueType::Unknown)
                {
                  auto src = resource.states[0];
                  auto dst = resource.states[0];
                  dst.queue_index = job.nextAccess.queue_index;
                  // how does I all subresources
                  // so just need mip size, can calculate arraysize then
                  auto allSubresources = resource.states.size();
                  uint32_t mips = resource.mips;
                  uint32_t arrSize = static_cast<uint32_t>(allSubresources) / mips;
                  bool acquire = jobResAccess.usage == AccessUsage::Read;
                  if (acquire)
                  {
                    dst.queue_index = src.queue_index;
                    src.queue_index = jobResAccess.queue_index;
                  }
                  else
                  {
                    auto transitionToCommon = src;
                    transitionToCommon.usage = AccessUsage::Read;
                    transitionToCommon.stage = AccessStage::Common;
                    dst.stage = AccessStage::Common;
                    dst.usage = AccessUsage::Read;

                    // todo: check all states to fix them correctly
                    imageBarriers.emplace_back(ImageBarrier{src, transitionToCommon, job.resource.resourceHandle(), 0, mips, 0, arrSize});
                    ++imageBarrierOffsets;
                    src.stage = AccessStage::Common;
                    src.usage = AccessUsage::Read;
                  }

                  imageBarriers.emplace_back(ImageBarrier{src, dst, job.resource.resourceHandle(), 0, mips, 0, arrSize});
                  ++imageBarrierOffsets;
                  // fix all the state
                  if (!acquire)
                  {
                    for (auto&& state : resource.states)
                    {
                      state.queue_index = job.nextAccess.queue_index;
                      state.stage = AccessStage::Common;
                    }
                  }
                }
              }
              else
              {
                // gothrough all subresources that are modified, create subresource ranges at the same time encompassing all similar states.

                struct RangePerAccessType
                {
                  uint32_t startMip : 4;
                  uint32_t mipSize : 4;
                  uint32_t startArr : 12;
                  uint32_t arrSize : 12;
                  ResourceState access;
                };

                auto addImageBarrier = [&](const RangePerAccessType& range)
                {
                  auto src = range.access;
                  src.queue_index = QueueType::Unknown;
                  auto dst = job.nextAccess;
                  dst.queue_index = QueueType::Unknown;
                  imageBarriers.emplace_back(ImageBarrier{src, dst, job.resource.resourceHandle(), range.startMip, range.mipSize, range.startArr, range.arrSize});
                  ++imageBarrierOffsets;
                  
                  //HIGAN_LOGi("barrier %zd mip %d-%d slice %d-%d layouts %s -> %s\n", job.resource.resourceHandle().id, range.startMip, range.mipSize, range.startArr, range.arrSize, toString(src.layout), toString(dst.layout));
                };
                RangePerAccessType current{};
                auto mipLevels = resource.mips;
                unsigned subresourceIndex;

                for (auto mip = job.resource.startMip(); mip < job.resource.startMip() + job.resource.mipSize(); ++mip)
                {
                  for (auto slice = job.resource.startArr(); slice < job.resource.startArr() + job.resource.arrSize(); ++slice)
                  {
                    subresourceIndex = slice * mipLevels + mip;
                    auto& state = resource.states[subresourceIndex];

                    bool usageDiffers = state.usage != job.nextAccess.usage;
                    bool stageDiffers = state.stage != job.nextAccess.stage;
                    bool layoutDiffers = state.layout != job.nextAccess.layout;

                    if (usageDiffers || stageDiffers ||  layoutDiffers) // something was different
                    {
                      // first check if we have fresh range
                      //   if so, fill it with all data
                      // else check if we can continue current range
                      //   check if any of those above differ to this range
                      // else create new range and fill it with data.

                      if (current.mipSize == 0 && current.arrSize == 0) // nothing added to range yet.
                      {
                        current.access = state;
                        current.startMip = mip;
                        current.mipSize = 1;
                        current.startArr = slice;
                        current.arrSize = 1;
                      }
                      else if (state.raw == current.access.raw && current.startArr >= slice) // can continue current range
                      {
                        current.mipSize = (mip - current.startMip) + 1; // +1 to have at least one, mip is ensured to always be bigger.
                        current.arrSize = (slice - current.startArr) + 1; // +1 to have at least one
                      }
                      else // push back the current range and and create new one.
                      {
                        addImageBarrier(current);
                        current = RangePerAccessType{};
                        current.mipSize = 0;
                        current.arrSize = 0;
                      }
                      // handled by now.
                      state = job.nextAccess;
                    }
                    else if (current.mipSize > 0) {
                      addImageBarrier(current);
                      current = RangePerAccessType{};
                      current.mipSize = 0;
                      current.arrSize = 0;
                    }
                  }
                }
                if (current.mipSize != 0 && current.arrSize != 0)
                {
                  addImageBarrier(current);
                }
              }
            }
          }
          ++jobIndex;
        }
      }
      if (currentInfo.bufferOffset != bufferBarrierOffsets || currentInfo.imageOffset != imageBarrierOffsets)
      {
        currentInfo.bufferCount = bufferBarrierOffsets - currentInfo.bufferOffset;
        currentInfo.imageCount = imageBarrierOffsets - currentInfo.imageOffset;
        currentInfo.drawcall = drawIndex;
        m_drawBarries.push_back(currentInfo);
      }
    }

    void BarrierSolver::globalBarrierPass2()
    {
      // patch list
      //HIGAN_LOGi("globalBarrierPass2\n");
      //HIGAN_LOGi("Patch list\n");
      for (auto&& buffer : bufferBarriers)
      {
        if (buffer.before.usage == AccessUsage::Unknown)
        {
          buffer.before = (*m_bufferStates)[buffer.handle];
        }
      }
      for (auto&& image : imageBarriers)
      {
        //HIGAN_LOGi("\t checking... tex %d before usage: \"%10s\" stage: \"%14s\" layout: \"%16s\"\n", image.handle.id, toString(image.before.usage), toString(image.before.stage), toString(image.before.layout));
        if (image.before.usage == AccessUsage::Unknown)
        {
          auto& ginfo = (*m_textureStates)[image.handle];
          auto refState = ginfo.states[image.startArr * ginfo.mips + image.startMip];
          refState.queue_index = QueueType::Unknown;
          for (int slice = image.startArr; slice < image.startArr + image.arrSize; ++slice)
          {
            for (int mip = image.startMip; mip < image.startMip + image.mipSize; ++mip)
            {
              int index = slice * ginfo.mips + mip;
              if ((refState.layout != ginfo.states[index].layout)
              || (refState.stage != ginfo.states[index].stage)
              || (refState.usage != ginfo.states[index].usage))
              {
                HIGAN_ASSERT(false, "oh no");
              }
            }
          }
          /*
          HIGAN_LOGi("\ttex %d barrier slice [%d - %d] mip [%d - %d]\n", image.handle.id, image.startArr, image.startArr+image.arrSize, image.startMip, image.startMip+image.mipSize);
          HIGAN_LOGi("\t\tbefore usage: \"%10s\" stage: \"%14s\" layout: \"%16s\"\n", toString(image.before.usage), toString(image.before.stage), toString(image.before.layout));
          HIGAN_LOGi("\t\tafter  usage: \"%10s\" stage: \"%14s\" layout: \"%16s\"\n", toString(refState.usage), toString(refState.stage), toString(refState.layout)); */
          image.before = refState;
        }
      }

      // update global state
      for (auto&& obj : m_uniqueBuffers)
      {
        (*m_bufferStates)[obj] = m_bufferCache[obj].state;
      }
      //HIGAN_LOGi("update global state\n");
      for (auto&& obj : m_uniqueTextures)
      {
        auto& globalState = (*m_textureStates)[obj].states;
        auto& localState = m_imageCache[obj].states;
        int globalSize = static_cast<int>(globalState.size());
        for (int i = 0; i < globalSize; ++i)
        {
          if (localState[i].usage != AccessUsage::Unknown) {
            /*
            HIGAN_LOGi("\ttex %d sub res %d\n", obj.id, i);
            HIGAN_LOGi("\t\tbefore usage: \"%10s\" stage: \"%14s\" layout: \"%16s\"\n", toString(globalState[i].usage), toString(globalState[i].stage), toString(globalState[i].layout));
            HIGAN_LOGi("\t\tafter  usage: \"%10s\" stage: \"%14s\" layout: \"%16s\"\n", toString(localState[i].usage), toString(localState[i].stage), toString(localState[i].layout));*/
            globalState[i] = localState[i];
          }
        }
      }
    }

    MemoryBarriers BarrierSolver::runBarrier(const BarrierInfo& drawCall)
    {
      MemoryBarriers barrier;

      barrier.buffers = MemView<BufferBarrier>(bufferBarriers.data()+drawCall.bufferOffset, drawCall.bufferCount);
      barrier.textures = MemView<ImageBarrier>(imageBarriers.data()+drawCall.imageOffset, drawCall.imageCount);

      /*
      if (!barrier.textures.empty())
      {
        HIGAN_LOGi("%d Barrier with textures\n", drawCall.drawcall);
      }
      for (auto&& tex : barrier.textures)
      {
        HIGAN_LOGi("\ttex %d barrier slice [%d - %d] mip [%d - %d]\n", tex.handle.id, tex.startArr, tex.startArr+tex.arrSize, tex.startMip, tex.startMip+tex.mipSize);
        HIGAN_LOGi("\t\tbefore usage: \"%10s\" stage: \"%14s\" layout: \"%16s\" queue: %s\n", toString(tex.before.usage), toString(tex.before.stage), toString(tex.before.layout), toString(tex.before.queue_index));
        HIGAN_LOGi("\t\tafter  usage: \"%10s\" stage: \"%14s\" layout: \"%16s\" queue: %s\n", toString(tex.after.usage), toString(tex.after.stage), toString(tex.after.layout), toString(tex.after.queue_index));
      }*/
      

      return barrier;
    }
  }
}