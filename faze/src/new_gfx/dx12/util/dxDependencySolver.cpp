#include "faze/src/new_gfx/dx12/util/dxDependencySolver.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "faze/src/new_gfx/definitions.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    DX12DependencySolver::UsageHint DX12DependencySolver::getUsageFromAccessFlags(D3D12_RESOURCE_STATES flags) const
    {
      int32_t writeMask = D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_RENDER_TARGET
        | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_RESOLVE_DEST;

      if (flags == D3D12_RESOURCE_STATE_COMMON || flags & writeMask) // common is the most complex state... with value of 0
      {
        return DX12DependencySolver::UsageHint::write;
      }
      return DX12DependencySolver::UsageHint::read;
    }

    bool DX12DependencySolver::promoteFromCommon(D3D12_RESOURCE_STATES targetState, bool bufferOrSimultaneousTexture) const
    {
        if (bufferOrSimultaneousTexture)
        {
            constexpr auto promoteMask = ~(D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ);
            if ((targetState & promoteMask) == targetState) // if survives promoteMask, it can be promoted.
                return true;
        }
        else
        {
            constexpr auto promoteMask = (D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            if ((targetState & promoteMask) == targetState) // if survives promoteMask, it can be promoted.
                return true;
        }
        return false;
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

    void DX12DependencySolver::makeAllBarriers()
    {
      m_resourceCache.clear();

      for (auto&& id : m_uniqueResourcesThisChain)
      {
        auto tesState = m_resourceStates[id];
        auto flags = tesState.state->flags;

        
        if (globalconfig::graphics::GraphicsEnableHandleCommonState)
        {
            if (tesState.state->commonStateOptimisation)
            {
                for (auto&& flag : flags)
                {
                    flag = D3D12_RESOURCE_STATE_COMMON;
                }
            }
        }
        
        vector<DrawCallIndex> lm(flags.size(), -1);
        m_resourceCache[id] = SmallResource{ tesState.texture, tesState.mips, tesState.state->commonStateOptimisation, flags, lm };
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
                  if (globalconfig::graphics::GraphicsEnableHandleCommonState && state == D3D12_RESOURCE_STATE_COMMON && promoteFromCommon(job.access, resource->second.commonPromotable))
                  {
                    state = job.access;
                  }
                  else if (state != job.access)
                  {
                    if (globalconfig::graphics::GraphicsEnableReadStateCombining && getUsageFromAccessFlags(state) == DX12DependencySolver::UsageHint::read && getUsageFromAccessFlags(job.access) == DX12DependencySolver::UsageHint::read)
                    {
                      auto lastUsed = resource->second.lastModified[subresourceIndex];
                      auto offset = m_barriersOffsets[lastUsed];
                      auto end = m_barriersOffsets[lastUsed + 1];
                      for (int i = offset; i < end; ++i)
                      {
                        if (barriers[i].Transition.pResource == resource->second.image)
                        {
                          state |= job.access;
                          barriers[i].Transition.StateAfter = state;
                          if (barriers[i].Type == D3D12_RESOURCE_BARRIER_FLAG_END_ONLY)
                          {
                            for (int k = std::max(i - 1, 0); k >= 0; --k)
                            {
                              if (barriers[k].Transition.pResource == resource->second.image && barriers[k].Type == D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY)
                              {
                                barriers[k].Transition.StateAfter = state;
                                break;
                              }
                            }
                          }
                          break;
                        }
                      }
                      continue;
                    }

                    auto beginDrawIndex = resource->second.lastModified[subresourceIndex] + 1;
                    if (globalconfig::graphics::GraphicsEnableSplitBarriers && beginDrawIndex < drawIndex)
                    {
                      //GFX_ILOG("found possible splitbarrier pos: %d draw: %d dist: %d resource: %zu subresource: %d", resource->second.lastModified[subresourceIndex] + 1, drawIndex, drawIndex - 1 - resource->second.lastModified[subresourceIndex], job.resource, subresourceIndex);
                      auto barrierOffsetForDrawIndex = beginDrawIndex;
                      bool foundBarrierOffset = false;

                      if (faze::globalconfig::graphics::GraphicsSplitBarriersPlaceBeginsOnExistingPoints)
                      {
                        for (int pdidx = barrierOffsetForDrawIndex; pdidx < drawIndex; ++pdidx)
                        {
                          auto possibleOffset = m_barriersOffsets[pdidx];
                          auto maxDrawOffset = (pdidx + 1 >= m_barriersOffsets.size()) ? barriers.size() : m_barriersOffsets[pdidx + 1];
                          for (int barrIdx = possibleOffset; barrIdx < maxDrawOffset; ++barrIdx)
                          {
                            if (barriers[barrIdx].Flags == D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY || barriers[barrIdx].Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE)
                            {
                              barrierOffsetForDrawIndex = pdidx;
                              foundBarrierOffset = true;
                              break;
                            }
                          }
                          if (foundBarrierOffset)
                            break;
                        }
                      }
                      else
                      {
                        foundBarrierOffset = true;
                      }
                      if (foundBarrierOffset)
                      {
                        auto bar = D3D12_RESOURCE_BARRIER{
                          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                          D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY,
                          resource->second.image,
                          static_cast<UINT>(subresourceIndex),
                          state,
                          jobResAccess };
                        ++barriersOffset;
                        auto barrierOffset = m_barriersOffsets[barrierOffsetForDrawIndex];
                        barriers.insert(barriers.begin() + barrierOffset, bar);

                        for (auto iter = barrierOffsetForDrawIndex + 1; iter < m_barriersOffsets.size(); ++iter)
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

      //GFX_LOG("Barriers: %d\n", barriersOffset);
      // function ends
    }

    void DX12DependencySolver::runBarrier(ID3D12GraphicsCommandList* gfx, int nextDrawCall)
    {
      if (nextDrawCall == 0)
      {
        int barrierSize = (m_barriersOffsets.size() > 1) ? m_barriersOffsets[1] : static_cast<int>(barriers.size());
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
      m_drawCallJobOffsets.clear();
      m_jobs.clear();
      m_schedulingResult.clear();
      m_barriersOffsets.clear();
      barriers.clear();
      m_uniqueResourcesThisChain.clear();
      m_resourceCache.clear();
      drawCallsAdded = 0;
      m_resourceStates.clear();
    }
  }
}
#endif