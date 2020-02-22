#pragma once
#include "higanbana/graphics/common/handle.hpp"
#include "higanbana/graphics/desc/resource_state.hpp"
#include "higanbana/graphics/common/resource_descriptor.hpp"

namespace higanbana
{
  namespace backend
  {
    struct MemoryBarriers
    {
      MemView<BufferBarrier> buffers;
      MemView<ImageBarrier> textures;
    };

    struct BarrierInfo
    {
      int drawcall;
      int bufferOffset;
      int bufferCount;
      int imageOffset;
      int imageCount;
    };

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
      //unordered_map<DrawCallIndex, int> m_drawCallJobOffsets;

      int drawCallsAdded = 0;

      // global state tables
      HandleVector<ResourceState>* m_bufferStates;
      HandleVector<TextureResourceState>* m_textureStates;

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
      vector<BarrierInfo> m_drawBarries;

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
      BarrierSolver()
        : m_bufferStates(nullptr)
        , m_textureStates(nullptr)
       {}
      BarrierSolver(HandleVector<ResourceState>& buffers
                  , HandleVector<TextureResourceState>& textures)
        : m_bufferStates(&buffers)
        , m_textureStates(&textures)
       {}

      int addDrawCall();

      // buffers
      void addBuffer(int drawCallIndex, ViewResourceHandle buffer, ResourceState access);
      // textures
      void addTexture(int drawCallIndex, ViewResourceHandle view, ResourceState access);

      // only builds the graph of dependencies.
      // void resolveGraph(); //... hmm, not implementing for now.
      // void printStuff(std::function<void(std::string)> func);
      void localBarrierPass1(bool allowCommonOptimization);
      void globalBarrierPass2();
      void reset();

      MemoryBarriers runBarrier(const BarrierInfo& drawCall);
      vector<BarrierInfo>& barrierInfos()
      {
        return m_drawBarries;
      }
    };
  }
}