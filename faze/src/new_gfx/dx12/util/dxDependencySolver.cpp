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

    void DX12DependencySolver::addResource(int drawCallIndex, backend::TrackedState state, D3D12_RESOURCE_STATES flags)
    {
      size_t uniqueID = state.resPtr;
      if (uniqueID == 0)
        return;

      SubresourceRange range{};

      range.sliceOffset = static_cast<int16_t>(state.slice());
      range.arraySize = static_cast<int16_t>(state.arraySize());
      range.mipOffset = static_cast<int16_t>(state.mip());
      range.mipLevels = static_cast<int16_t>(state.mipLevels());

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, flags, range });
      if (m_resourceStates.find(uniqueID) == m_resourceStates.end())
      {
        ResourceDependency d{};
        d.uniqueId = uniqueID;
        d.mips = static_cast<int16_t>(state.totalMipLevels());
        d.texture = reinterpret_cast<ID3D12Resource*>(state.resPtr);
        d.state = reinterpret_cast<DX12ResourceState*>(state.statePtr);
        m_resourceStates[uniqueID] = std::move(d);
      }
      m_uniqueResourcesThisChain.insert(uniqueID);
    }

    void DX12DependencySolver::addResource(int drawCallIndex, backend::TrackedState state, D3D12_RESOURCE_STATES flags, SubresourceRange range)
    {
      size_t uniqueID = state.resPtr;
      if (uniqueID == 0)
        return;
      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, flags, range });

      if (m_resourceStates.find(uniqueID) == m_resourceStates.end())
      {
        ResourceDependency d{};
        d.uniqueId = uniqueID;
        d.mips = static_cast<int16_t>(state.totalMipLevels());
        d.texture = reinterpret_cast<ID3D12Resource*>(state.resPtr);
        d.state = reinterpret_cast<DX12ResourceState*>(state.statePtr);
        m_resourceStates[uniqueID] = std::move(d);
      }
      m_uniqueResourcesThisChain.insert(uniqueID);
    }

    // buffers
    /*
    void DX12DependencySolver::addBuffer(int drawCallIndex, int64_t, DX12Buffer& buffer, D3D12_RESOURCE_STATES flags)
    {
      size_t uniqueID = reinterpret_cast<size_t>(buffer.native());
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

      if (m_resourceStates.find(uniqueID) == m_resourceStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = 1;
        d.texture = buffer.native();
        d.state = buffer.state().get();
        m_resourceStates[uniqueID] = std::move(d);
      }
      m_uniqueBuffersThisChain.insert(uniqueID);
    }
    */
    // textures
    /*
    void DX12DependencySolver::addTexture(int drawCallIndex, int64_t, DX12Texture& texture, DX12TextureView& view, int16_t mips, D3D12_RESOURCE_STATES flags)
    {
      auto uniqueID = reinterpret_cast<size_t>(texture.native());

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

      if (m_resourceStates.find(uniqueID) == m_resourceStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = mips;
        d.texture = texture.native();
        d.state = texture.state().get();
        m_resourceStates[uniqueID] = std::move(d);
      }

      m_uniqueResourcesThisChain.insert(uniqueID);
    }
    */

    void DX12DependencySolver::makeAllBarriers()
    {
      m_resourceCache.clear();

      for (auto&& id : m_uniqueResourcesThisChain)
      {
        auto tesState = m_resourceStates[id];
        auto flags = tesState.state->flags;

        /*
        auto commonDecayMask = (D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        for (auto&& flag : flags)
        {
            if ((flag & commonDecayMask) == flag) // if survives decayMask, it can be decayed.
                flag = D3D12_RESOURCE_STATE_COMMON;
        }
        */
        vector<DrawCallIndex> lm(flags.size(), -1);
        m_resourceCache[id] = SmallResource{ tesState.texture, tesState.mips, flags, lm };
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

        while (jobIndex < jobsSize && m_jobs[jobIndex].drawIndex == draw)
        {
          auto& job = m_jobs[jobIndex];
          auto jobResAccess = job.access;
          {
            auto resource = m_resourceCache.find(job.resource);
            if (resource != m_resourceCache.end())
            {
              int16_t mipLevels = resource->second.mips;
              int16_t subresourceIndex;

              bool uav = false;
              for (int16_t mip = job.range.mipOffset; mip < job.range.mipOffset + job.range.mipLevels; ++mip)
              {
                for (int16_t slice = job.range.sliceOffset; slice < job.range.sliceOffset + job.range.arraySize; ++slice)
                {
                  subresourceIndex = slice * mipLevels + mip;
                  auto& state = resource->second.states[subresourceIndex];

                  if (D3D12_RESOURCE_STATE_UNORDERED_ACCESS == state && D3D12_RESOURCE_STATE_UNORDERED_ACCESS == job.access)
                  {
                    uav = true;
                  }
                  if (state == D3D12_RESOURCE_STATE_COMMON)
                  {
                    constexpr const auto mask = (D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    if ((job.access & mask) == job.access)
                    {
                      resource->second.lastModified[subresourceIndex] = drawIndex;
                      //GFX_ILOG("common modified draw: %d resource: %zu subresource: %d", drawIndex, job.resource, subresourceIndex);
                      state = job.access;
                      continue;
                    }
                  }

                  if (state != job.access)
                  {
                    auto beginDrawIndex = resource->second.lastModified[subresourceIndex] + 1;
                    if (globalconfig::graphics::GraphicsEnableSplitBarriers && beginDrawIndex < drawIndex)
                    {
                      //GFX_ILOG("found possible splitbarrier pos: %d draw: %d dist: %d resource: %zu subresource: %d", resource->second.lastModified[subresourceIndex] + 1, drawIndex, drawIndex - 1 - resource->second.lastModified[subresourceIndex], job.resource, subresourceIndex);
                      auto barrierOffsetForDrawIndex = m_barriersOffsets[beginDrawIndex + 1];
                      auto bar = D3D12_RESOURCE_BARRIER{
                        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
                        resource->second.image,
                        static_cast<UINT>(subresourceIndex),
                        state,
                        jobResAccess };
                      ++barriersOffset;
                      barriers.insert(barriers.begin() + barrierOffsetForDrawIndex, bar);

                      for (auto iter = beginDrawIndex + 1; iter < m_barriersOffsets.size(); ++iter)
                      {
                        m_barriersOffsets[iter] += 1;
                      }
                      // end barrier
                      barriers.emplace_back(D3D12_RESOURCE_BARRIER{
                        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        D3D12_RESOURCE_BARRIER_FLAG_END_ONLY,
                        resource->second.image,
                        static_cast<UINT>(subresourceIndex),
                        state,
                        jobResAccess });
                      //GFX_ILOG("begin %d end: %d resource: %zu subresource: %d before: 0x%09x after: 0x%09x", beginDrawIndex, drawIndex, job.resource, subresourceIndex, state, job.access);
                    }
                    else
                    {
                      barriers.emplace_back(D3D12_RESOURCE_BARRIER{
                        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        D3D12_RESOURCE_BARRIER_FLAG_NONE,
                        resource->second.image,
                        static_cast<UINT>(subresourceIndex),
                        state,
                        jobResAccess });
                      //GFX_ILOG("modified draw: %d resource: %zu subresource: %d before: 0x%09x after: 0x%09x", drawIndex, job.resource, subresourceIndex, state, job.access);
                    }
                    state = jobResAccess;
                    resource->second.lastModified[subresourceIndex] = drawIndex;
                    ++barriersOffset;
                  }

                  //GFX_ILOG("required by draw: %d resource: %zu subresource: %d state: 0x%09x", drawIndex, job.resource, subresourceIndex, state);
                  resource->second.lastModified[subresourceIndex] = drawIndex;
                }
              }
              if (uav)
              {
                barriers.emplace_back(D3D12_RESOURCE_BARRIER{
                    D3D12_RESOURCE_BARRIER_TYPE_UAV,
                    D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    resource->second.image });
                ++barriersOffset;
              }
            }
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

      for (auto&& obj : m_uniqueResourcesThisChain)
      {
        auto& globalState = m_resourceStates[obj].state->flags;
        auto& localState = m_resourceCache[obj].states;
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
      m_uniqueResourcesThisChain.clear();
    }
  }
}