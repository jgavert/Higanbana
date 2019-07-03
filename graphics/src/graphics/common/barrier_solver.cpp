#include <graphics/common/barrier_solver.hpp>

namespace faze
{
  namespace backend
  {
    int BarrierSolver::addDrawCall()
    {
      //auto index = m_drawCallStage.size();
      //m_drawCallStage.push_back(baseFlags);
      return drawCallsAdded++; 
    }
    void BarrierSolver::addBuffer(int drawCallIndex, ViewResourceHandle buffer, ResourceState access)
    {
      //m_bufferStates[buffer].state = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      m_bufferCache[buffer.resource].state = ResourceState(backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::General, m_bufferStates[buffer.resource].queue_index);
      m_jobs.push_back(DependencyPacket{drawCallIndex, buffer, access});
      m_uniqueBuffers.insert(buffer.resource);
    }
    void BarrierSolver::addTexture(int drawCallIndex, ViewResourceHandle texture, ResourceState access)
    {
      //m_textureStates[buffer] = ResourceState( backend::AccessUsage::Unknown, backend::AccessStage::Common, backend::TextureLayout::Undefined, 0);
      //m_imageCache[texture].states.resize(m_textureStates[texture.resource].states.size());
      m_imageCache[texture.resource].states = m_textureStates[texture.resource].states;
      m_jobs.push_back(DependencyPacket{drawCallIndex, texture, access});
      m_uniqueTextures.insert(texture.resource);
    }
    void BarrierSolver::reset()
    {
      m_jobs.clear();
      m_bufferCache.clear();
      m_imageCache.clear();
    }

    void BarrierSolver::makeAllBarriers()
    {
      int jobsSize = static_cast<int>(m_jobs.size());
      int jobIndex = 0;
      int bufferBarrierOffsets = 0;
      int imageBarrierOffsets = 0;

      int drawIndex = -1;

      while (jobIndex < jobsSize)
      {
        int draw = m_jobs[jobIndex].drawIndex;
        for (int skippedDraw = drawIndex; skippedDraw < draw; ++skippedDraw)
        {
          m_barrierOffsets.emplace_back(bufferBarrierOffsets);
          m_imageBarrierOffsets.emplace_back(imageBarrierOffsets);
        }
        drawIndex = draw;

        while (jobIndex < jobsSize && m_jobs[jobIndex].drawIndex == draw)
        {
          auto& job = m_jobs[jobIndex];
          auto jobResAccess = job.nextAccess;
          if (job.resource.resource.type == ResourceType::Buffer)
          {
            auto& resource = m_bufferCache[job.resource.resource];
            auto lastAccess = resource.state;
            if (lastAccess.queue_index != jobResAccess.queue_index && jobResAccess.stage == AccessStage::Common)
            {
              if (lastAccess.queue_index != QueueType::Unknown )
              {
                // acquire/release barrier
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
                bufferBarriers.emplace_back(BufferBarrier{src, dst, job.resource.resource});
                ++bufferBarrierOffsets;
              }
            }
            else if (lastAccess.usage != AccessUsage::Unknown && lastAccess.stage != AccessStage::Common && (jobResAccess.stage != lastAccess.stage || jobResAccess.usage != lastAccess.usage))
            {
              auto src = lastAccess;
              src.queue_index = QueueType::Unknown;
              auto dst = jobResAccess;
              dst.queue_index = QueueType::Unknown;
              bufferBarriers.emplace_back(BufferBarrier{src, dst, job.resource.resource});
              ++bufferBarrierOffsets;
              resource.state = job.nextAccess;
            }
            else
            {
              resource.state = job.nextAccess;
            }
          }
          else
          {
            auto& resource = m_imageCache[job.resource.resource];
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
                  uint32_t mips = job.resource.fullMipSize();
                  uint32_t arrSize = static_cast<uint32_t>(allSubresources) / mips;
                  bool acquire = jobResAccess.usage == AccessUsage::Read;
                  if (acquire)
                  {
                    dst.queue_index = src.queue_index;
                    src.queue_index = jobResAccess.queue_index;
                  }

                  imageBarriers.emplace_back(ImageBarrier{src, dst, job.resource.resource, 0, mips, 0, arrSize});
                  ++imageBarrierOffsets;
                  // fix all the state
                  if (!acquire)
                  {
                    for (auto&& state : resource.states)
                    {
                      state.queue_index = job.nextAccess.queue_index;
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
                  imageBarriers.emplace_back(ImageBarrier{src, dst, job.resource.resource, range.startMip, range.mipSize, range.startArr, range.arrSize});
                  ++imageBarrierOffsets;
                };
                RangePerAccessType current{};
                auto mipLevels = job.resource.fullMipSize();
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
      for (int skippedDraw = drawIndex; skippedDraw < drawCallsAdded; ++skippedDraw)
      {
        m_barrierOffsets.emplace_back(bufferBarrierOffsets);
        m_imageBarrierOffsets.emplace_back(imageBarrierOffsets);
      }

      // update global state
      for (auto&& obj : m_uniqueBuffers)
      {
        m_bufferStates[obj] = m_bufferCache[obj].state;
      }

      for (auto&& obj : m_uniqueTextures)
      {
        auto& globalState = m_textureStates[obj].states;
        auto& localState = m_imageCache[obj].states;
        int globalSize = static_cast<int>(globalState.size());
        for (int i = 0; i < globalSize; ++i)
        {
          globalState[i] = localState[i];
        }
      }
    }

    MemoryBarriers BarrierSolver::runBarrier(int drawCall)
    {
      MemoryBarriers barrier;

      auto bufferOffset = m_barrierOffsets[drawCall];
      auto bufferSize = m_barrierOffsets[drawCall + 1] - bufferOffset;
      auto imageOffset = m_imageBarrierOffsets[drawCall];
      auto imageSize = m_imageBarrierOffsets[drawCall + 1] - imageOffset;

      barrier.buffers = MemView<BufferBarrier>(bufferBarriers.data()+bufferOffset, bufferSize);
      barrier.textures = MemView<ImageBarrier>(imageBarriers.data()+imageOffset, imageSize);

      return barrier;
    }
  }
}