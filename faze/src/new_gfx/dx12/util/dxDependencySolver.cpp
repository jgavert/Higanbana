#include "faze/src/new_gfx/dx12/util/dxDependencySolver.hpp"
#include "faze/src/new_gfx/definitions.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12DependencySolver::UsageHint DX12DependencySolver::getUsageFromAccessFlags(D3D12_RESOURCE_STATES flags)
    {
      int32_t writeMask = D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_RENDER_TARGET
        | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_RESOLVE_DEST;

      if (flags == D3D12_RESOURCE_STATE_COMMON || flags & writeMask) // common is the most complex state... with value of 0
      {
        return DX12DependencySolver::UsageHint::write;
      }
      return DX12DependencySolver::UsageHint::read;
    }

    int DX12DependencySolver::addDrawCall(CommandPacket::PacketType name)
    {
      m_drawCallInfo.emplace_back(name);
      m_drawCallJobOffsets[drawCallsAdded] = static_cast<int>(m_jobs.size());
      return drawCallsAdded++;
    }
    // buffers
    void DX12DependencySolver::addBuffer(int drawCallIndex, int64_t, DX12Buffer& buffer, D3D12_RESOURCE_STATES flags)
    {
      int64_t uniqueID = reinterpret_cast<int64_t>(buffer.native());
      if (faze::globalconfig::graphics::GraphicsEnableReadStateCombining) // disable optimization, merges last seen usages to existing. Default on. Combines only reads.
      {
        auto currentUsage = getUsageFromAccessFlags(flags);
        if (currentUsage == UsageHint::read)
        {
          auto* obj = m_resourceUsageInLastAdd.find(uniqueID);
          if (obj)
          {
            if (obj->second.type == currentUsage)
            {
              // Try to recognize here that last time we saw the same id, we already used it in read/write state, combine the states.
              auto* jobIndex = m_drawCallJobOffsets.find(obj->second.index);
              if (jobIndex)
              {
                int index = jobIndex->second;
                while (index < static_cast<int>(m_jobs.size()) && obj->second.index == m_jobs[index].drawIndex)
                {
                  if (m_jobs[index].resource == uniqueID)
                  {
                    break;
                  }
                  index++;
                }
                m_jobs[index].access |= flags;
                return; // found and could merge... or rather forcefully merged.
              }
            }
          }
        }
        m_resourceUsageInLastAdd[uniqueID] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::buffer,
        flags, SubresourceRange{} });
      if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
      {
        BufferDependency d;
        d.uniqueId = uniqueID;
        d.buffer = buffer.native();
        d.state = buffer.state();
        m_bufferStates[uniqueID] = std::move(d);
      }
      m_uniqueBuffersThisChain.insert(uniqueID);
    }
    // textures
    void DX12DependencySolver::addTexture(int drawCallIndex, int64_t, DX12Texture& texture, DX12TextureView& view, int16_t mips, D3D12_RESOURCE_STATES flags)
    {
      auto uniqueID = reinterpret_cast<int64_t>(texture.native());

      if (faze::globalconfig::graphics::GraphicsEnableReadStateCombining)
      {
        auto currentUsage = getUsageFromAccessFlags(flags);
        if (currentUsage == UsageHint::read)
        {
          auto* obj = m_resourceUsageInLastAdd.find(uniqueID);
          if (obj)
          {
            if (obj->second.type != currentUsage)
            {
              F_SLOG("DependencySolver", "Possible optimization here!\n");
              // Try to recognize here that last time we saw the same id, we already used it in read/write state, combine the states.
              // Texture... needs to look at all the subresources seen before and combine their state... ugh, this looks not to be easy.
            }
          }
        }
        m_resourceUsageInLastAdd[uniqueID] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::texture,
        flags, view.range() });

      if (m_textureStates.find(uniqueID) == m_textureStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = mips;
        d.texture = texture.native();
        d.state = texture.state();
        m_textureStates[uniqueID] = std::move(d);
      }

      m_uniqueTexturesThisChain.insert(uniqueID);
    }
    void DX12DependencySolver::addTexture(int drawCallIndex, int64_t, DX12Texture& texture, int16_t mips, D3D12_RESOURCE_STATES flags, SubresourceRange range)
    {
      auto uniqueID = reinterpret_cast<int64_t>(texture.native());

      if (faze::globalconfig::graphics::GraphicsEnableReadStateCombining)
      {
        auto currentUsage = getUsageFromAccessFlags(flags);
        if (currentUsage == UsageHint::read)
        {
          auto* obj = m_resourceUsageInLastAdd.find(uniqueID);
          if (obj)
          {
            if (obj->second.type != currentUsage)
            {
              F_SLOG("DependencySolver", "Possible optimization here!\n");
              // Somehow feels hard to figure out where all last subresources were. so that we could merge the use to them...
            }
          }
        }
        m_resourceUsageInLastAdd[uniqueID] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::texture,
        flags, range });

      if (m_textureStates.find(uniqueID) == m_textureStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = mips;
        d.texture = texture.native();
        d.state = texture.state();
        m_textureStates[uniqueID] = std::move(d);
      }

      m_uniqueTexturesThisChain.insert(uniqueID);
    }

    void DX12DependencySolver::makeAllBarriers()
    {
      m_bufferCache.clear();
      m_imageCache.clear();

      // fill cache with all resources seen.
      for (auto&& id : m_uniqueBuffersThisChain)
      {
        m_bufferCache[id] = SmallBuffer{ m_bufferStates[id].buffer, *m_bufferStates[id].state };
      }

      for (auto&& id : m_uniqueTexturesThisChain)
      {
        auto tesState = m_textureStates[id];
        auto flags = tesState.state->flags;
        m_imageCache[id] = SmallTexture{ tesState.texture, tesState.mips, flags };
      }
      int jobsSize = static_cast<int>(m_jobs.size());
      int jobIndex = 0;
      int barriersOffset = 0;

      int drawIndex = -1;

      while (jobIndex < jobsSize)
      {
        int draw = m_jobs[jobIndex].drawIndex;
        for (int skippedDraw = drawIndex; skippedDraw < draw; ++skippedDraw)
        {
          m_barriersOffsets.emplace_back(barriersOffset);
        }
        drawIndex = draw;

        m_uavCache.clear();

        while (jobIndex < jobsSize && m_jobs[jobIndex].drawIndex == draw)
        {
          auto& job = m_jobs[jobIndex];
          auto jobResAccess = job.access;
          if (job.type == ResourceType::buffer)
          {
            auto resource = m_bufferCache.find(job.resource);
            if (resource != m_bufferCache.end())
            {
              auto lastAccess = resource->second.flags;

              if (lastAccess & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) // everytime the last access was unordered access, make uav barrier.
              {
                m_uavCache.emplace(resource->second.buffer);
              }

              if (jobResAccess != lastAccess)
              {
                barriers.emplace_back(D3D12_RESOURCE_BARRIER{
                  D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                  D3D12_RESOURCE_BARRIER_FLAG_NONE,
                  resource->second.buffer,
                  static_cast<UINT>(-1),
                  lastAccess,
                  jobResAccess });
                resource->second.flags = jobResAccess;
                ++barriersOffset;
              }
            }
          }
          else
          {
            auto resource = m_imageCache.find(job.resource);
            if (resource != m_imageCache.end())
            {
              int16_t mipLevels = resource->second.mips;
              int16_t subresourceIndex;

              for (int16_t mip = job.range.mipOffset; mip < job.range.mipOffset + job.range.mipLevels; ++mip)
              {
                for (int16_t slice = job.range.sliceOffset; slice < job.range.sliceOffset + job.range.arraySize; ++slice)
                {
                  subresourceIndex = slice * mipLevels + mip;
                  auto& state = resource->second.states[subresourceIndex];

                  if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
                  {
                    m_uavCache.emplace(resource->second.image);
                  }

                  if (state != job.access)
                  {
                    barriers.emplace_back(D3D12_RESOURCE_BARRIER{
                      D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                      D3D12_RESOURCE_BARRIER_FLAG_NONE,
                      resource->second.image,
                      static_cast<UINT>(-1),
                      state,
                      jobResAccess });
                    state = jobResAccess;
                    ++barriersOffset;
                  }
                }
              }
            }
          }

          for (auto&& it : m_uavCache)
          {
            barriers.emplace_back(D3D12_RESOURCE_BARRIER{
              D3D12_RESOURCE_BARRIER_TYPE_UAV,
              D3D12_RESOURCE_BARRIER_FLAG_NONE,
              it });
            ++barriersOffset;
          }

          ++jobIndex;
        }
      }
      for (int skippedDraw = drawIndex; skippedDraw <= drawCallsAdded; ++skippedDraw)
      {
        m_barriersOffsets.emplace_back(static_cast<int>(barriers.size()));
      }
      m_barriersOffsets.emplace_back(static_cast<int>(barriers.size()));

      // update global state
      for (auto&& obj : m_uniqueBuffersThisChain)
      {
        *m_bufferStates[obj].state = m_bufferCache[obj].flags;
      }

      for (auto&& obj : m_uniqueTexturesThisChain)
      {
        auto& globalState = m_textureStates[obj].state->flags;
        auto& localState = m_imageCache[obj].states;
        auto globalSize = globalState.size();
        for (int i = 0; i < globalSize; ++i)
        {
          globalState[i] = localState[i];
        }
      }
      // function ends
    }

    void DX12DependencySolver::runBarrier(ID3D12GraphicsCommandList* gfx, int nextDrawCall)
    {
      if (nextDrawCall == 0)
      {
        int barrierSize = (m_barriersOffsets.size() > 1 && m_barriersOffsets[1] > 0) ? m_barriersOffsets[1] : static_cast<int>(barriers.size());
        if (barrierSize == 0)
        {
          return;
        }
        gfx->ResourceBarrier(barrierSize, barriers.data());
        return;
      }

      // after first and second, nextDrawCall == 1
      // so we need barriers from offset 0
      int barrierOffset = m_barriersOffsets[nextDrawCall];
      int barrierSize = m_barriersOffsets[nextDrawCall + 1] - barrierOffset;

      if (barrierSize == 0)
      {
        return;
      }

      gfx->ResourceBarrier(barrierSize, barriers.data() + barrierOffset);
    }

    void DX12DependencySolver::reset()
    {
      m_drawCallInfo.clear();
      m_jobs.clear();
      m_schedulingResult.clear();
      m_barriersOffsets.clear();
      barriers.clear();
      m_uniqueBuffersThisChain.clear();
      m_uniqueTexturesThisChain.clear();
    }
  }
}