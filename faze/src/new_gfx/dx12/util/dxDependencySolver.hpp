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
      using ResourceUniqueId = int64_t;

      struct BufferDependency
      {
        ResourceUniqueId uniqueId;
        ID3D12Resource* buffer;
        std::shared_ptr<D3D12_RESOURCE_STATES> state;
      };

      struct TextureDependency
      {
        ResourceUniqueId uniqueId;
        ID3D12Resource* texture;
        int16_t mips;
        std::shared_ptr<DX12TextureState> state;
      };

      using DrawCallIndex = int;

      enum class UsageHint
      {
        unknown,
        read,
        write
      };

      enum class ResourceType
      {
        buffer,
        texture
      };

      struct DependencyPacket
      {
        DrawCallIndex drawIndex;
        ResourceUniqueId resource;
        ResourceType type;
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
      unordered_map<ResourceUniqueId, LastSeenUsage> m_resourceUsageInLastAdd;

      vector<CommandPacket::PacketType> m_drawCallInfo;
      unordered_set<ResourceUniqueId> m_uniqueBuffersThisChain;
      unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
      int drawCallsAdded = 0;

      //needed for textures
      unordered_set<ResourceUniqueId> m_uniqueTexturesThisChain;
      unordered_map<ResourceUniqueId, TextureDependency> m_textureStates;

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

      struct SmallBuffer
      {
        ID3D12Resource* buffer;
        D3D12_RESOURCE_STATES flags;
      };

      struct SmallTexture
      {
        ID3D12Resource* image;
        int16_t mips;
        vector<D3D12_RESOURCE_STATES> states;
      };

      unordered_map<ResourceUniqueId, SmallBuffer> m_bufferCache;
      unordered_map<ResourceUniqueId, SmallTexture> m_imageCache;
      unordered_set<ID3D12Resource*> m_uavCache;
    public:
      DX12DependencySolver() {}

      int addDrawCall(CommandPacket::PacketType name);

      // buffers
      void addBuffer(int drawCallIndex, int64_t id, DX12Buffer& buffer, D3D12_RESOURCE_STATES flags);
      // textures
      void addTexture(int drawCallIndex, int64_t id, DX12Texture& texture, DX12TextureView& view, int16_t mips, D3D12_RESOURCE_STATES flags);
      void addTexture(int drawCallIndex, int64_t id, DX12Texture& texture, int16_t mips, D3D12_RESOURCE_STATES flags, SubresourceRange range);

      void makeAllBarriers();
      void runBarrier(ID3D12GraphicsCommandList* gfx, int nextDrawCall);
      void reset();

    private:
      UsageHint getUsageFromAccessFlags(D3D12_RESOURCE_STATES flags);
    };
  }
}