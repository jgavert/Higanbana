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
      m_bufferCache[buffer.resource].state = ResourceState(backend::AccessUsage::Read, backend::AccessStage::Common, backend::TextureLayout::General, 0);
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
            if (jobResAccess.stage != lastAccess.stage || jobResAccess.usage != lastAccess.usage)
            {
              bufferBarriers.emplace_back(BufferBarrier{lastAccess, jobResAccess, job.resource.resource});
              resource.state = jobResAccess;
              ++bufferBarrierOffsets;
            }
          }
          else
          {
            auto& resource = m_imageCache[job.resource.resource];
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
                imageBarriers.emplace_back(ImageBarrier{range.access, job.nextAccess, job.resource.resource, range.startMip, range.mipSize, range.startArr, range.arrSize});
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
                  bool queueDiffers = state.queue_index != job.nextAccess.queue_index;
                  bool layoutDiffers = state.layout != job.nextAccess.layout;

                  if (usageDiffers || stageDiffers || queueDiffers || layoutDiffers) // something was different
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

      barrier.buffers = MemView<BufferBarrier>(&bufferBarriers[bufferOffset], bufferSize);
      barrier.textures = MemView<ImageBarrier>(&imageBarriers[imageOffset], imageSize);

      return barrier;
    }
  }
}