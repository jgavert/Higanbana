#pragma once

#include "core/src/datastructures/proxy.hpp"
#include "faze/src/new_gfx/common/commandpackets.hpp"
#include "faze/src/new_gfx/vk/vkresources.hpp"

namespace faze
{
  namespace backend
  {
    class VulkanDependencySolver
    {
    private:
      using ResourceUniqueId = int64_t;

      struct BufferDependency
      {
        ResourceUniqueId uniqueId;
        vk::Buffer buffer;
        std::shared_ptr<VulkanBufferState> state;
      };

      struct TextureDependency
      {
        ResourceUniqueId uniqueId;
        vk::Image texture;
        int16_t mips;
        vk::ImageAspectFlags aspectMask;
        std::shared_ptr<VulkanTextureState> state;
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
        vk::AccessFlags access;
        vk::ImageLayout layout;
        int queueIndex;
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
      vector<vk::PipelineStageFlags> m_drawCallStage;
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
      vector<vk::BufferMemoryBarrier> bufferBarriers;
      vector<int> m_barrierOffsets;

      vector<vk::ImageMemoryBarrier> imageBarriers;
      vector<int> m_imageBarrierOffsets;

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
        vk::Buffer buffer;
        vk::AccessFlags flags;
        int queueIndex;
      };

      struct SmallTexture
      {
        vk::Image image;
        int16_t mips;
        vk::ImageAspectFlags aspectMask;
        vector<TextureStateFlags> states;
      };

      unordered_map<ResourceUniqueId, SmallBuffer> m_bufferCache;
      unordered_map<ResourceUniqueId, SmallTexture> m_imageCache;
    public:
      VulkanDependencySolver() {}

      int addDrawCall(CommandPacket::PacketType name, vk::PipelineStageFlags baseFlags);

      // buffers
      void addBuffer(int drawCallIndex, int64_t id, VulkanBuffer& buffer, vk::AccessFlags flags);
      // textures
      void addTexture(int drawCallIndex, int64_t id, VulkanTexture& texture, VulkanTextureView& view, int16_t mips, vk::ImageLayout layout, vk::AccessFlags flags);
      void addTexture(int drawCallIndex, int64_t id, VulkanTexture& texture, int16_t mips, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout, vk::AccessFlags flags, SubresourceRange range);

      // only builds the graph of dependencies.
      // void resolveGraph(); //... hmm, not implementing for now.
      // void printStuff(std::function<void(std::string)> func);
      void makeAllBarriers();
      void runBarrier(vk::CommandBuffer gfx, int nextDrawCall);
      void reset();

    private:
      UsageHint getUsageFromAccessFlags(vk::AccessFlags flags);
    };
  }
}