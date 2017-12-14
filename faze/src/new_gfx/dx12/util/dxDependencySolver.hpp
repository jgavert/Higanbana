#pragma once

#include "core/src/datastructures/proxy.hpp"
#include "faze/src/new_gfx/common/commandpackets.hpp"
#include "faze/src/new_gfx/dx12/dx12resources.hpp"

namespace faze
{
  namespace backend
  {
    class DX12DependencySolver
    {
    private:
      using ResourceUniqueId = size_t;

      struct ResourceDependency
      {
        ResourceUniqueId uniqueId;
        ID3D12Resource* texture;
        int16_t mips;
        DX12ResourceState* state;
      };

      using DrawCallIndex = int;

      enum class UsageHint : unsigned char
      {
        unknown,
        read,
        write
      };

      struct DependencyPacket
      {
        DrawCallIndex drawIndex;
        ResourceUniqueId resource;
        D3D12_RESOURCE_STATES access;
        SubresourceRange range;
      };

      struct LastSeenUsage
      {
        UsageHint type;
        DrawCallIndex index;
      };

      // general info needed
      unordered_map<DrawCallIndex, int> m_drawCallJobOffsets;

      vector<CommandPacket::PacketType> m_drawCallInfo;
      int drawCallsAdded = 0;

      unordered_set<ResourceUniqueId> m_uniqueResourcesThisChain;
      unordered_map<ResourceUniqueId, ResourceDependency> m_resourceStates;

      // actual jobs used to generate DAG
      vector<DependencyPacket> m_jobs;

      // results
      struct ScheduleNode
      {
        DrawCallIndex jobID;
        DrawCallIndex dependency;
      };
      vector<ScheduleNode> m_schedulingResult;

      // barriers

      vector<D3D12_RESOURCE_BARRIER> barriers;
      vector<int> m_barriersOffsets;

      // caches
      struct WriteCall
      {
        ResourceUniqueId resource;
        DrawCallIndex draw;
      };
      vector<WriteCall> m_cacheWrites;
      vector<int> m_readRes;

      struct SmallResource
      {
        ID3D12Resource* image;
        int16_t mips;
        vector<D3D12_RESOURCE_STATES> states;
      };

      unordered_map<ResourceUniqueId, SmallResource> m_resourceCache;
      unordered_set<ID3D12Resource*> m_uavCache;
    public:
      DX12DependencySolver() {}

      int addDrawCall(CommandPacket::PacketType name);

      // buffers
      void addResource(int drawCallIndex, backend::TrackedState state, D3D12_RESOURCE_STATES flags);
      void addResource(int drawCallIndex, backend::TrackedState state, D3D12_RESOURCE_STATES flags, SubresourceRange range);

      void makeAllBarriers();
      void runBarrier(ID3D12GraphicsCommandList* gfx, int nextDrawCall);
      void reset();

    private:
      UsageHint getUsageFromAccessFlags(D3D12_RESOURCE_STATES flags);
    };
  }
}