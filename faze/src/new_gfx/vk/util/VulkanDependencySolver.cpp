#include "faze/src/new_gfx/vk/util/VulkanDependencySolver.hpp"
#include "faze/src/new_gfx/definitions.hpp"
#include "core/src/global_debug.hpp"

namespace faze
{
  namespace backend
  {
    VulkanDependencySolver::UsageHint VulkanDependencySolver::getUsageFromAccessFlags(vk::AccessFlags flags)
    {
      int32_t writeMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX;

      if (flags.operator unsigned int() & writeMask)
      {
        return VulkanDependencySolver::UsageHint::write;
      }
      return VulkanDependencySolver::UsageHint::read;
    }

    int VulkanDependencySolver::addDrawCall(CommandPacket::PacketType name, vk::PipelineStageFlags baseFlags)
    {
      m_drawCallInfo.emplace_back(name);
      m_drawCallStage.emplace_back(baseFlags);
      m_drawCallJobOffsets[drawCallsAdded] = static_cast<int>(m_jobs.size());
      return drawCallsAdded++;
    }
    // buffers
    void VulkanDependencySolver::addBuffer(int drawCallIndex, int64_t id, VulkanBuffer& buffer, vk::AccessFlags flags)
    {
      auto uniqueID = id;
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
        m_resourceUsageInLastAdd[id] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::buffer,
        flags, vk::ImageLayout::eUndefined, 0, SubresourceRange{} });
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
    void VulkanDependencySolver::addTexture(int drawCallIndex, int64_t id, VulkanTexture& texture, VulkanTextureView& view, int16_t mips, vk::ImageLayout layout, vk::AccessFlags flags)
    {
      auto uniqueID = id;

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
        m_resourceUsageInLastAdd[id] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::texture,
        flags, layout, 0, view.native().subResourceRange });

      if (m_textureStates.find(uniqueID) == m_textureStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = mips;
        d.texture = texture.native();
        d.state = texture.state();
        d.aspectMask = view.native().aspectMask;
        m_textureStates[uniqueID] = std::move(d);
      }

      m_uniqueTexturesThisChain.insert(uniqueID);
    }
    void VulkanDependencySolver::addTexture(int drawCallIndex, int64_t id, VulkanTexture& texture, int16_t mips, vk::ImageAspectFlags aspectMask, vk::ImageLayout layout, vk::AccessFlags flags, SubresourceRange range)
    {
      auto uniqueID = id;

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
        m_resourceUsageInLastAdd[id] = LastSeenUsage{ currentUsage, drawCallIndex };
      }

      m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, ResourceType::texture,
        flags, layout, 0, range });

      if (m_textureStates.find(uniqueID) == m_textureStates.end())
      {
        TextureDependency d;
        d.uniqueId = uniqueID;
        d.mips = mips;
        d.texture = texture.native();
        d.state = texture.state();
        d.aspectMask = aspectMask;
        m_textureStates[uniqueID] = std::move(d);
      }

      m_uniqueTexturesThisChain.insert(uniqueID);
    }

    void VulkanDependencySolver::makeAllBarriers()
    {
      m_bufferCache.clear();
      m_imageCache.clear();

      // fill cache with all resources seen.
      for (auto&& id : m_uniqueBuffersThisChain)
      {
        m_bufferCache[id] = SmallBuffer{ m_bufferStates[id].buffer, m_bufferStates[id].state->flags, m_bufferStates[id].state->queueIndex };
      }

      for (auto&& id : m_uniqueTexturesThisChain)
      {
        auto tesState = m_textureStates[id];
        auto flags = tesState.state->flags;
        m_imageCache[id] = SmallTexture{ tesState.texture, tesState.mips, tesState.aspectMask, flags };
      }
      int jobsSize = static_cast<int>(m_jobs.size());
      int jobIndex = 0;
      int bufferBarrierOffsets = 0;
      int imageBarrierOffsets = 0;

      int drawIndex = 0;

      while (jobIndex < jobsSize)
      {
        int draw = m_jobs[jobIndex].drawIndex;
        for (int skippedDraw = drawIndex; skippedDraw <= draw; ++skippedDraw)
        {
          m_barrierOffsets.emplace_back(bufferBarrierOffsets);
          m_imageBarrierOffsets.emplace_back(imageBarrierOffsets);
        }
        drawIndex = draw;

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
              if (jobResAccess != vk::AccessFlagBits(0) && jobResAccess != lastAccess)
              {
                bufferBarriers.emplace_back(vk::BufferMemoryBarrier()
                  .setSrcAccessMask(lastAccess)
                  .setDstAccessMask(jobResAccess)
                  .setSrcQueueFamilyIndex(resource->second.queueIndex)
                  .setDstQueueFamilyIndex(job.queueIndex)
                  .setBuffer(resource->second.buffer)
                  .setOffset(0)
                  .setSize(VK_WHOLE_SIZE));
                resource->second.flags = jobResAccess;
                ++bufferBarrierOffsets;
              }
            }
          }
          else
          {
            auto resource = m_imageCache.find(job.resource);
            if (resource != m_imageCache.end())
            {
              // gothrough all subresources that are modified, create subresource ranges at the same time encompassing all similar states.

              struct RangePerAccessType
              {
                vk::ImageSubresourceRange range;
                vk::AccessFlags access;
                vk::ImageLayout layout;
                int queueIndex;
              };

              auto addImageBarrier = [&](const RangePerAccessType& range)
              {
                imageBarriers.emplace_back(vk::ImageMemoryBarrier()
                  .setSrcAccessMask(range.access)
                  .setDstAccessMask(job.access)
                  .setSrcQueueFamilyIndex(range.queueIndex)
                  .setDstQueueFamilyIndex(job.queueIndex)
                  .setNewLayout(job.layout)
                  .setOldLayout(range.layout)
                  .setImage(resource->second.image)
                  .setSubresourceRange(range.range));
                ++imageBarrierOffsets;
              };
              RangePerAccessType current{};
              int16_t mipLevels = resource->second.mips;
              int16_t subresourceIndex;

              for (int16_t mip = job.range.mipOffset; mip < job.range.mipOffset + job.range.mipLevels; ++mip)
              {
                for (int16_t slice = job.range.sliceOffset; slice < job.range.sliceOffset + job.range.arraySize; ++slice)
                {
                  subresourceIndex = slice * mipLevels + mip;
                  auto& state = resource->second.states[subresourceIndex];

                  bool accessDiffers = state.access != job.access;
                  bool queueDiffers = state.queueIndex != job.queueIndex;
                  bool layoutDiffers = state.layout != job.layout;

                  if (accessDiffers || queueDiffers || layoutDiffers) // something was different
                  {
                    // first check if we have fresh range
                    //   if so, fill it with all data
                    // else check if we can continue current range
                    //   check if any of those above differ to this range
                    // else create new range and fill it with data.

                    if (current.range.levelCount == 0 && current.range.layerCount == 0) // nothing added to range yet.
                    {
                      current.access = state.access;
                      current.queueIndex = state.queueIndex;
                      current.layout = state.layout;
                      current.range.aspectMask = resource->second.aspectMask;
                      current.range.baseMipLevel = mip;
                      current.range.levelCount = 1;
                      current.range.baseArrayLayer = slice;
                      current.range.layerCount = 1;
                    }
                    else if (state.access == current.access && state.queueIndex == current.queueIndex && state.layout == current.layout && static_cast<int16_t>(current.range.baseArrayLayer) >= slice) // can continue current range
                    {
                      current.range.levelCount = (mip - current.range.baseMipLevel) + 1; // +1 to have at least one, mip is ensured to always be bigger.
                      current.range.layerCount = (slice - current.range.baseArrayLayer) + 1; // +1 to have at least one
                    }
                    else // push back the current range and and create new one.
                    {
                      addImageBarrier(current);
                      current = RangePerAccessType{};
                      current.access = state.access;
                      current.queueIndex = state.queueIndex;
                      current.layout = state.layout;
                      current.range.aspectMask = resource->second.aspectMask;
                      current.range.baseMipLevel = mip;
                      current.range.levelCount = 1;
                      current.range.baseArrayLayer = slice;
                      current.range.layerCount = 1;
                    }
                    // handled by now.
                    state.access = job.access;
                    state.queueIndex = job.queueIndex;
                    state.layout = job.layout;
                  }
                }
              }
              if (current.range.levelCount != 0 && current.range.layerCount != 0)
              {
                addImageBarrier(current);
              }
            }
          }
          ++jobIndex;
        }
      }
      m_barrierOffsets.emplace_back(static_cast<int>(bufferBarriers.size()));
      m_imageBarrierOffsets.emplace_back(static_cast<int>(imageBarriers.size()));

      // update global state
      for (auto&& obj : m_uniqueBuffersThisChain)
      {
        m_bufferStates[obj].state->flags = m_bufferCache[obj].flags;
        m_bufferStates[obj].state->queueIndex = m_bufferCache[obj].queueIndex;
      }

      for (auto&& obj : m_uniqueTexturesThisChain)
      {
        auto& globalState = m_textureStates[obj].state->flags;
        auto& localState = m_imageCache[obj].states;
        auto globalSize = globalState.size();
        for (int i = 0; i < globalSize; ++i)
        {
          globalState[i].access = localState[i].access;
          globalState[i].layout = localState[i].layout;
          globalState[i].queueIndex = localState[i].queueIndex;
        }
      }
      // function ends
    }

    void VulkanDependencySolver::runBarrier(vk::CommandBuffer gfx, int nextDrawCall)
    {
      if (nextDrawCall == 0)
      {
        int barrierOffset = 0;
        int barrierSize = (m_barrierOffsets.size() > 1 && m_barrierOffsets[1] > 0) ? m_barrierOffsets[1] : static_cast<int>(bufferBarriers.size());
        int imageBarrierOffset = 0;
        int imageBarrierSize = (m_imageBarrierOffsets.size() > 1 && m_imageBarrierOffsets[1] > 0) ? m_imageBarrierOffsets[1] : static_cast<int>(imageBarriers.size());
        if (barrierSize == 0 && imageBarrierSize == 0)
        {
          return;
        }
        vk::PipelineStageFlags last = vk::PipelineStageFlagBits::eBottomOfPipe; // full trust in perfect synchronization
        vk::PipelineStageFlags next = m_drawCallStage[0];
        vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
        vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), bufferBarriers.data() + barrierOffset);
        vk::ArrayProxy<const vk::ImageMemoryBarrier> image(static_cast<uint32_t>(imageBarrierSize), imageBarriers.data() + imageBarrierOffset);
        gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
        return;
      }

      // after first and second, nextDrawCall == 1
      // so we need barriers from offset 0
      int barrierOffset = m_barrierOffsets[nextDrawCall];
      int barrierSize = m_barrierOffsets[nextDrawCall + 1] - barrierOffset;

      int imageBarrierOffset = m_imageBarrierOffsets[nextDrawCall];
      int imageBarrierSize = m_imageBarrierOffsets[nextDrawCall + 1] - imageBarrierOffset;

      if (barrierSize == 0 && imageBarrierSize == 0)
      {
        return;
      }
      vk::PipelineStageFlags last = m_drawCallStage[nextDrawCall - 1];
      vk::PipelineStageFlags next = m_drawCallStage[nextDrawCall];

      vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
      vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), bufferBarriers.data() + barrierOffset);
      vk::ArrayProxy<const vk::ImageMemoryBarrier> image(static_cast<uint32_t>(imageBarrierSize), imageBarriers.data() + imageBarrierOffset);
      gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
    }

    void VulkanDependencySolver::reset()
    {
      m_drawCallInfo.clear();
      m_drawCallStage.clear();
      m_jobs.clear();
      m_schedulingResult.clear();
      m_barrierOffsets.clear();
      bufferBarriers.clear();
      m_uniqueBuffersThisChain.clear();
      m_imageBarrierOffsets.clear();
      imageBarriers.clear();
      m_uniqueTexturesThisChain.clear();
    }
  }
}