#pragma once
#include "core/platform/definitions.hpp"
#if defined(FAZE_PLATFORM_WINDOWS)
#include "core/datastructures/proxy.hpp"
#include "graphics/common/commandpackets.hpp"
#include "graphics/dx12/dx12resources.hpp"

namespace faze
{
  namespace backend
  {
    class DX12DependencySolver
    {
    private:
      using ResourceUniqueId = size_t;

      using DrawCallIndex = int;

      struct ResourceLifetime
      {
        DrawCallIndex begin;
        DrawCallIndex end;
      };

      struct ResourceDependency
      {
        ResourceUniqueId uniqueId;
        ID3D12Resource* texture;
        int16_t mips;
        DX12ResourceState* state;
      };

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
        D3D12_RESOURCE_STATES access; // state required for this draw, so this is either split_end or immediate barrier, so the begin packet either happens or doesn't.
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
      unordered_map<ResourceUniqueId, ResourceLifetime> m_resourceLFS;

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

      struct SmallResource
      {
        ID3D12Resource* image;
        int16_t mips;
        bool commonPromotable;
        vector<D3D12_RESOURCE_STATES> states;
        vector<DrawCallIndex> lastModified;
      };

      unordered_map<ResourceUniqueId, SmallResource> m_resourceCache;
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
      inline UsageHint getUsageFromAccessFlags(D3D12_RESOURCE_STATES flags) const;
      inline bool promoteFromCommon(D3D12_RESOURCE_STATES targetState, bool bufferOrSimultaneousTexture) const;
    };
  }
}
#endif