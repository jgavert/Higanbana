#pragma once
#include <graphics/common/handle.hpp>
#include <graphics/desc/resource_state.hpp>
#include <graphics/common/resource_descriptor.hpp>

namespace faze
{
  namespace backend
  {
    class BarrierSolver 
    {
    private:
      using DrawCallIndex = int;

      struct DependencyPacket
      {
        DrawCallIndex drawIndex;
        ResourceHandle resource;
        ResourceState nextAccess;
        SubresourceRange range;
      };

      // general info needed
      unordered_map<DrawCallIndex, int> m_drawCallJobOffsets;

      vector<backend::AccessStage> m_drawCallStage;
      int drawCallsAdded = 0;

      // global state tables
      HandleVector<ResourceState>& m_bufferStates;
      HandleVector<TextureResourceState>& m_textureStates;
      HandleVector<SubresourceRange>& m_texSRVran;
      HandleVector<SubresourceRange>& m_texUAVran;
      HandleVector<SubresourceRange>& m_texRTVran;
      HandleVector<SubresourceRange>& m_texDSVran;

      // actual jobs used to generate DAG
      vector<DependencyPacket> m_jobs;

      // results
      struct ScheduleNode
      {
        DrawCallIndex jobID;
        DrawCallIndex dependency;
      };
      vector<ScheduleNode> m_schedulingResult;

      // caches
      struct WriteCall
      {
        ResourceHandle resource;
        DrawCallIndex draw;
      };
      vector<WriteCall> m_cacheWrites;
      vector<int> m_readRes;

      struct SmallBuffer
      {
        ResourceHandle buffer;
        ResourceState state;
      };

      struct SmallTexture
      {
        ResourceHandle image;
        vector<ResourceState> states;
        int16_t mips;
      };

      unordered_map<uint64_t, SmallBuffer> m_bufferCache;
      unordered_map<uint64_t, SmallTexture> m_imageCache;
    public:
      BarrierSolver(HandleVector<ResourceState>& buffers
      , HandleVector<TextureResourceState>& textures
      , HandleVector<SubresourceRange>& textureSRVran
      , HandleVector<SubresourceRange>& textureUAVran
      , HandleVector<SubresourceRange>& textureRTVran
      , HandleVector<SubresourceRange>& textureDSVran)
        : m_bufferStates(buffers)
        , m_textureStates(textures)
        , m_texSRVran(textureSRVran)
        , m_texUAVran(textureUAVran)
        , m_texRTVran(textureRTVran)
        , m_texDSVran(textureDSVran)
       {}

      int addDrawCall(backend::AccessStage baseFlags);

      // buffers
      void addBuffer(int drawCallIndex, ResourceHandle buffer, ResourceState access);
      // textures
      void addTexture(int drawCallIndex, ResourceHandle texture, ResourceHandle view, int16_t mips, ResourceState access);
      void addTexture(int drawCallIndex, ResourceHandle texture, int16_t mips, ResourceState access, SubresourceRange range);

      // only builds the graph of dependencies.
      // void resolveGraph(); //... hmm, not implementing for now.
      // void printStuff(std::function<void(std::string)> func);
      //void makeAllBarriers();
      //void runBarrier(vk::CommandBuffer& gfx, int nextDrawCall);
      void reset();

    private:
      //UsageHint getUsageFromAccessFlags(vk::AccessFlags flags);
    };
  }
}