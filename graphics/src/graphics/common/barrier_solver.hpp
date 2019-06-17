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
        ViewResourceHandle resource;
        ResourceState nextAccess;
      };

      // general info needed
      unordered_map<DrawCallIndex, int> m_drawCallJobOffsets;

      vector<backend::AccessStage> m_drawCallStage;
      int drawCallsAdded = 0;

      // global state tables
      HandleVector<ResourceState>& m_bufferStates;
      HandleVector<TextureResourceState>& m_textureStates;

      unordered_set<ResourceHandle> m_uniqueBuffers;
      unordered_set<ResourceHandle> m_uniqueTextures;

      // actual jobs used to generate DAG
      vector<DependencyPacket> m_jobs;

      // results
      //struct ScheduleNode
      //{
      //  DrawCallIndex jobID;
      //  DrawCallIndex dependency;
      //};
      //vector<ScheduleNode> m_schedulingResult;

      // caches
      //struct WriteCall
      //{
      //  ViewResourceHandle resource;
      //  DrawCallIndex draw;
      //};
      //vector<WriteCall> m_cacheWrites;
      //vector<int> m_readRes;

      vector<int> m_barrierOffsets;
      vector<int> m_imageBarrierOffsets;
      vector<BufferBarrier> bufferBarriers;
      vector<ImageBarrier> imageBarriers;

      struct SmallBuffer
      {
        ResourceState state;
      };

      struct SmallTexture
      {
        vector<ResourceState> states;
      };

      HandleVector<SmallBuffer> m_bufferCache;
      HandleVector<SmallTexture> m_imageCache;
    public:
      BarrierSolver(HandleVector<ResourceState>& buffers
      , HandleVector<TextureResourceState>& textures)
        : m_bufferStates(buffers)
        , m_textureStates(textures)
       {}

      int addDrawCall(backend::AccessStage baseFlags);

      // buffers
      void addBuffer(int drawCallIndex, ViewResourceHandle buffer, ResourceState access);
      // textures
      void addTexture(int drawCallIndex, ViewResourceHandle view, ResourceState access);

      // only builds the graph of dependencies.
      // void resolveGraph(); //... hmm, not implementing for now.
      // void printStuff(std::function<void(std::string)> func);
      void makeAllBarriers();
      //void runBarrier(vk::CommandBuffer& gfx, int nextDrawCall);
      void reset();

    private:
      //UsageHint getUsageFromAccessFlags(vk::AccessFlags flags);
    };
  }
}