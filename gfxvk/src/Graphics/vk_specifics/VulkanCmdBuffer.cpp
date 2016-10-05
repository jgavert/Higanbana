#include "VulkanCmdBuffer.hpp"
#include "VulkanGpuDevice.hpp"
#include "core/src/global_debug.hpp"

#include <utility>
#include <deque>

#define COMMANDBUFFERSIZE 500*1024
using namespace faze;

class VulkanBindingInformation
{
public:
  CommandListVector<std::pair< unsigned, VulkanBufferShaderView >> readBuffers;
  CommandListVector<std::pair< unsigned, VulkanTextureShaderView>> readTextures;
  CommandListVector<std::pair< unsigned, VulkanBufferShaderView >> modifyBuffers;
  CommandListVector<std::pair< unsigned, VulkanTextureShaderView>> modifyTextures;

  VulkanBindingInformation(LinearAllocator& allocator, VulkanDescriptorSet& set)
    : readBuffers(MemView<std::pair<unsigned, VulkanBufferShaderView>>(allocator.allocList<std::pair<unsigned, VulkanBufferShaderView>>(set.readBuffers.size()), set.readBuffers.size()))
    , readTextures(MemView<std::pair<unsigned, VulkanTextureShaderView>>(allocator.allocList<std::pair<unsigned, VulkanTextureShaderView>>(set.readTextures.size()), set.readTextures.size()))
    , modifyBuffers(MemView<std::pair<unsigned, VulkanBufferShaderView>>(allocator.allocList<std::pair<unsigned, VulkanBufferShaderView>>(set.modifyBuffers.size()), set.modifyBuffers.size()))
    , modifyTextures(MemView<std::pair<unsigned, VulkanTextureShaderView>>(allocator.allocList<std::pair<unsigned, VulkanTextureShaderView>>(set.modifyTextures.size()), set.modifyTextures.size()))
  {
    for (size_t i = 0; i < set.readBuffers.size(); i++)
    {
      readBuffers[i] = set.readBuffers[i];
    }
    for (size_t i = 0; i < set.readTextures.size(); i++)
    {
      readTextures[i] = set.readTextures[i];
    }
    for (size_t i = 0; i < set.modifyBuffers.size(); i++)
    {
      modifyBuffers[i] = set.modifyBuffers[i];
    }
    for (size_t i = 0; i < set.modifyTextures.size(); i++)
    {
      modifyTextures[i] = set.modifyTextures[i];
    }
  }
};


////////////////////////////////////////////////////////////////////////////////////////
//////////////////// PACKETS START /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

class PipelineBarrierPacket : public VulkanCommandPacket
{
public:
  vk::PipelineStageFlags srcStageMask;
  vk::PipelineStageFlags dstStageMask;
  vk::DependencyFlags dependencyFlags;
  CommandListVector<vk::MemoryBarrier> memoryBarriers;
  CommandListVector<vk::BufferMemoryBarrier> bufferBarriers;
  CommandListVector<vk::ImageMemoryBarrier> imageBarriers;

  PipelineBarrierPacket(LinearAllocator& allocator,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags,
    MemView<vk::MemoryBarrier> memoryBarriers,
    MemView<vk::BufferMemoryBarrier> bufferBarriers,
    MemView<vk::ImageMemoryBarrier> imageBarriers)
    : srcStageMask(srcStageMask)
    , dstStageMask(dstStageMask)
    , dependencyFlags(dependencyFlags)
    , memoryBarriers(MemView<vk::MemoryBarrier>(allocator.allocList<vk::MemoryBarrier>(memoryBarriers.size()), memoryBarriers.size()))
    , bufferBarriers(MemView<vk::BufferMemoryBarrier>(allocator.allocList<vk::BufferMemoryBarrier>(bufferBarriers.size()), bufferBarriers.size()))
    , imageBarriers(MemView<vk::ImageMemoryBarrier>(allocator.allocList<vk::ImageMemoryBarrier>(imageBarriers.size()), imageBarriers.size()))
  {

  }
  void execute(vk::CommandBuffer& cmd) override
  {
    vk::ArrayProxy<const vk::MemoryBarrier> memory(static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data());
    vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data());
    vk::ArrayProxy<const vk::ImageMemoryBarrier> image(static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
    cmd.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memory, buffer, image);
  }
};

class BufferCopyPacket : public VulkanCommandPacket
{
public:
  VulkanBuffer src;
  VulkanBuffer dst;
  CommandListVector<vk::BufferCopy> m_copyList;

  BufferCopyPacket(LinearAllocator& allocator, VulkanBuffer src, VulkanBuffer dst, MemView<vk::BufferCopy> copyList)
    : src(src)
    , dst(dst)
    , m_copyList(MemView<vk::BufferCopy>(allocator.allocList<vk::BufferCopy>(copyList.size()), copyList.size()))
  {
    F_ASSERT(copyList, "Invalid copy commands");
    for (size_t i = 0; i < m_copyList.size(); i++)
    {
      m_copyList[i] = copyList[i];
    }
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    vk::ArrayProxy<const vk::BufferCopy> proxy(static_cast<uint32_t>(m_copyList.size()), m_copyList.data());
    cmd.copyBuffer(src.impl(), dst.impl(), proxy);
  }

  PacketType type() override
  {
    return PacketType::BufferCopy;
  }
};

class BindPipelinePacket : public VulkanCommandPacket
{
public:
  std::shared_ptr<vk::Pipeline> pipeline;
  std::shared_ptr<vk::PipelineLayout> layout;
  std::shared_ptr<vk::DescriptorSetLayout> descriptorLayout;
  vk::PipelineBindPoint point;
  BindPipelinePacket(LinearAllocator&, vk::PipelineBindPoint point
    , std::shared_ptr<vk::Pipeline>& pipeline, std::shared_ptr<vk::PipelineLayout> layout
    , std::shared_ptr<vk::DescriptorSetLayout> descriptorLayout)
    : pipeline(pipeline)
    , layout(layout)
    , descriptorLayout(descriptorLayout)
    , point(point)
  {
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    cmd.bindPipeline(point, *pipeline);
  }

  PacketType type() override
  {
    return PacketType::BindPipeline;
  }
};


class DispatchPacket : public VulkanCommandPacket
{
public:
  VulkanBindingInformation descriptors;
  unsigned dx = 0;
  unsigned dy = 0;
  unsigned dz = 0;
  DispatchPacket(LinearAllocator& allocator, VulkanDescriptorSet& inputs, unsigned x, unsigned y, unsigned z)
    : descriptors(allocator, std::forward<decltype(inputs)>(inputs))
    , dx(x)
    , dy(y)
    , dz(z)
  {
  }
  void execute(vk::CommandBuffer& cmd) override
  {
    cmd.dispatch(dx, dy, dz);
  }

  PacketType type() override
  {
    return PacketType::Dispatch;
  }
};


////////////////////////////////////////////////////////////////////////////////////////
//////////////////// PACKETS END ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void VulkanCmdBuffer::copy(VulkanBuffer& src, VulkanBuffer& dst)
{
  vk::BufferCopy copy;
  auto srcSize = src.desc().m_stride * src.desc().m_width;
  auto dstSize = dst.desc().m_stride * dst.desc().m_width;
  auto maxSize = std::min(srcSize, dstSize);
  copy = copy.setSize(maxSize)
    .setDstOffset(0)
    .setSrcOffset(0);
  m_commandList->insert<BufferCopyPacket>(src, dst, copy);
}

void VulkanCmdBuffer::dispatch(VulkanDescriptorSet& set, unsigned x, unsigned y, unsigned z)
{
  m_commandList->insert<DispatchPacket>(set, x, y, z);
}

void VulkanCmdBuffer::bindComputePipeline(VulkanPipeline& pipeline)
{
  m_commandList->insert<BindPipelinePacket>(vk::PipelineBindPoint::eCompute, pipeline.m_pipeline, pipeline.m_pipelineLayout, pipeline.m_descriptorSetLayout);
}

// MISC

VulkanCmdBuffer::VulkanCmdBuffer(std::shared_ptr<vk::CommandBuffer> buffer, std::shared_ptr<vk::CommandPool> pool)
  : m_cmdBuffer(std::forward<decltype(buffer)>(buffer))
  , m_pool(std::forward<decltype(pool)>(pool))
  , m_closed(false)
  , m_commandList(std::make_shared<CommandList<VulkanCommandPacket>>(LinearAllocator(COMMANDBUFFERSIZE)))
{}

bool VulkanCmdBuffer::isValid()
{
  return m_cmdBuffer.get() != nullptr;
}

bool VulkanCmdBuffer::isClosed()
{
  return m_closed;
}

void VulkanCmdBuffer::prepareForSubmit(VulkanGpuDevice& device, VulkanDescriptorPool& pool)
{
  processBindings(device, pool);
  dependencyFuckup();
}

////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Process Packets ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void VulkanCmdBuffer::close()
{
  m_cmdBuffer->begin(vk::CommandBufferBeginInfo()
    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    .setPInheritanceInfo(nullptr));

  vk::PipelineLayout pipelineLayout;
  int drawOrDispatch = 0;
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BindPipeline:
    {
      BindPipelinePacket* current = static_cast<BindPipelinePacket*>(packet);
      pipelineLayout = *current->layout;
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
    {
      constexpr uint32_t firstSet = 0;
      vk::ArrayProxy<const vk::DescriptorSet> sets(m_updatedSetsPerDraw[drawOrDispatch]);
      vk::ArrayProxy<const uint32_t> setOffsets(0, 0);
      m_cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, firstSet, sets, setOffsets);
      drawOrDispatch++;
      break;
    }
    default:
      break;
    }
    packet->execute(*m_cmdBuffer);
  });


  m_cmdBuffer->end();
  m_closed = true;
}

void VulkanCmdBuffer::processBindings(VulkanGpuDevice& device, VulkanDescriptorPool& pool)
{
  vk::DescriptorSetLayout* descriptorLayout;

  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BindPipeline:
    {
      BindPipelinePacket* current = static_cast<BindPipelinePacket*>(packet);
      descriptorLayout = current->descriptorLayout.get();
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
      {
        // handle binding here, create function that does it in a generic way.
        DispatchPacket* dis = static_cast<DispatchPacket*>(packet);
        auto& bind = dis->descriptors;

        auto result = device.m_device->allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
          .setDescriptorPool(pool.pool)
          .setDescriptorSetCount(1)
          .setPSetLayouts(descriptorLayout));

        std::vector<vk::WriteDescriptorSet> allSets;
        for (auto&& it : bind.readBuffers)
        {
          allSets.push_back(vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(it.second.type())
            .setDstBinding(it.first)
            .setDstSet(result[0])
            .setPBufferInfo(&it.second.info()));
        }
        for (auto&& it : bind.modifyBuffers)
        {
          allSets.push_back(vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(it.second.type())
            .setDstBinding(it.first)
            .setDstSet(result[0])
            .setPBufferInfo(&it.second.info()));
        }
        vk::ArrayProxy<const vk::WriteDescriptorSet> proxy(allSets);
        device.m_device->updateDescriptorSets(proxy, {});
        
        m_updatedSetsPerDraw.push_back(result[0]);
        break;
      }
    default:
      break;
    }
  });
}
/////////////////////////////////////////////////////////////////////////
///////////////////      DependencyTracker     //////////////////////////
/////////////////////////////////////////////////////////////////////////

struct BufferDependency
{
  int64_t uniqueId;
  vk::Buffer buffer;
  vk::DeviceSize offset;
  vk::DeviceSize range;
  std::shared_ptr<VulkanBufferState> state;
};

class DependencyTracker
{
private:
  using DrawCallIndex = int;
  using ResourceUniqueId = int64_t;

  enum class UsageHint
  {
    read,
    write
  };

  struct DependencyPacket
  {
    DrawCallIndex drawIndex;
    ResourceUniqueId resource;
    UsageHint hint;
    vk::AccessFlags access;
  };

  // general info needed
  std::unordered_map<DrawCallIndex, std::string> m_drawCallInfo;
  std::unordered_map<ResourceUniqueId, DrawCallIndex> m_writeRes;
  std::unordered_map<ResourceUniqueId, BufferDependency> m_bufferStates;
  size_t drawCallsAdded = 0;

  // actual jobs used to generate DAG
  std::vector<DependencyPacket> m_jobs;


  // results
  struct ScheduleNode
  {
    DrawCallIndex jobID;
    DrawCallIndex dependency;
  };
  std::vector<ScheduleNode> m_schedulingResult;

public:
  void addDrawCall(int drawCallIndex, std::string name)
  {
    m_drawCallInfo[drawCallIndex] = name;
    drawCallsAdded++;
  }

  void addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
  {
    auto uniqueID = buffer.uniqueId;
    m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, flags });
    if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
    {
      BufferDependency d;
      d.uniqueId = buffer.uniqueId;
      d.buffer = buffer.m_info.buffer;
      d.offset = buffer.m_info.offset;
      d.range = buffer.m_info.range;
      d.state = buffer.m_state;
      m_bufferStates[uniqueID] = std::move(d);
    }
  }
  void addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
  {
    auto uniqueID = buffer.uniqueId;
    m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, flags });
    if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
    {
      BufferDependency d;
      d.uniqueId = buffer.uniqueId;
      d.buffer = buffer.m_info.buffer;
      d.offset = buffer.m_info.offset;
      d.range = buffer.m_info.range;
      d.state = buffer.m_state;
      m_bufferStates[uniqueID] = std::move(d);
    }
    m_writeRes[uniqueID] = drawCallIndex;
  }

  void addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
  {
    auto uniqueID = buffer.uniqueId;
    m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, flags });
    if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
    {
      BufferDependency d;
      d.uniqueId = buffer.uniqueId;
      d.buffer = *buffer.m_resource;
      d.offset = offset;
      d.range = range;
      d.state = buffer.m_state;
      m_bufferStates[uniqueID] = std::move(d);
    }
  }
  void addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
  {
    auto uniqueID = buffer.uniqueId;
    m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, flags });
    if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
    {
      BufferDependency d;
      d.uniqueId = buffer.uniqueId;
      d.buffer = *buffer.m_resource;
      d.offset = offset;
      d.range = range;
      d.state = buffer.m_state;
      m_bufferStates[uniqueID] = std::move(d);
    }
    m_writeRes[uniqueID] = drawCallIndex;
  }

  // only builds the graph of dependencies.
  void resolveGraph()
  {
    auto currentSize = m_jobs.size();
    // create DAG(directed acyclic graph) from scratch
    m_schedulingResult.clear();
    m_schedulingResult.reserve(m_jobs.size());

    int currentJobId = 0;
    for (int i = 0; i < static_cast<int>(currentSize); ++i)
    {
      auto& obj = m_jobs[i];
      currentJobId = obj.drawIndex;
      // find all resources?
      std::vector<int> readRes;

      // move 'i' to next object.
      for (; i < static_cast<int>(currentSize); ++i)
      {
        if (m_jobs[i].drawIndex != currentJobId)
          break;
        if (m_jobs[i].hint == UsageHint::read)
        {
          readRes.push_back(i);
        }
      }
      --i; // backoff one, loop exits with extra appended value;

      ScheduleNode n;
      n.jobID = currentJobId;
      n.dependency = -1;
      if (readRes.empty()) // doesn't read any results of other jobs in this graph.
      {
        // which means its fine to be run
        m_schedulingResult.emplace_back(n);
      }
      else
      {
        // has read dependencies;
        // need to search which jobs produce our dependency
        // this is purely optional
        // reading resources is valid even if nobody produces them
        bool foundEvenOne = false;
        for (auto&& it : readRes)
        {
          auto& readr = m_jobs[it].resource;
          auto found = m_writeRes.find(readr);
          if (found != m_writeRes.end())
          {
            foundEvenOne = true;
            n.dependency = found->second;
            m_schedulingResult.emplace_back(n);
          }
        }
        if (!foundEvenOne)
          m_schedulingResult.emplace_back(n);
      }
    }

    // order is based on insert order
    drawCallsAdded = 0;
  }

  void DependencyTracker::printStuff(std::function<void(std::string)> func)
  {
    // print graph in some way
    func("// Dependency Graph\n");

    // create dot format output
    std::string tmp = "\n";

    tmp += "\ndigraph{\nrankdir = LR;\ncompound = true;\nnode[shape = record];\n";

    // all resources one by one
    int i = 0;

    // the graph, should be on top
    tmp += "// Dependency Graph...\n";
    tmp += "subgraph cluster_";
    tmp += std::to_string(i);
    tmp += " {\nlabel = \"Dependency Graph (DAG)\";\n";
    func(tmp); tmp.clear();
    for (auto&& it : m_schedulingResult)
    {
      tmp += std::to_string(it.jobID);
      tmp += "[label=\"";
      tmp += std::to_string(it.jobID);
      tmp += ". ";
      tmp += m_drawCallInfo[it.jobID];
      tmp += "\"];\n";
      func(tmp); tmp.clear();
    }
    tmp += "}\n";
    tmp += "// edges\n";
    func(tmp); tmp.clear();
    for (auto&& it : m_schedulingResult)
    {
      if (it.dependency == -1) // didn't have any dependencies 
        continue;
      tmp += std::to_string(it.dependency);
      tmp += " -> ";
      tmp += std::to_string(it.jobID);
      tmp += ";\n";
      func(tmp); tmp.clear();
    }
    ++i;
    tmp += "// Execution order...\n";
    tmp += "subgraph cluster_";
    tmp += std::to_string(i);
    tmp += "{\nlabel = \"Execution order\";\n";
    func(tmp); tmp.clear();
    int fb = 0;
    for (auto&& it : m_schedulingResult)
    {
      tmp += std::to_string(it.jobID + m_jobs.size());
      tmp += "[label=\"";
      tmp += std::to_string(fb);
      tmp += ". ";
      tmp += m_drawCallInfo[it.jobID];
      tmp += "\"];\n";
      func(tmp); tmp.clear();
      fb++;
    }
    tmp += "}\n";
    tmp += "// edges\n";
    func(tmp); tmp.clear();

    auto lastOne = m_schedulingResult[0].jobID;

    for (size_t k = 1; k < m_schedulingResult.size(); ++k)
    {
      tmp += std::to_string(lastOne + m_jobs.size());
      tmp += " -> ";
      tmp += std::to_string(m_schedulingResult[k].jobID + m_jobs.size());
      tmp += ";\n";
      lastOne = m_schedulingResult[k].jobID;
      func(tmp); tmp.clear();
    }

    tmp += "}\n";
    func(tmp);
  }

};

/////////////////////////////////////////////////////////////////////////
///////////////////      DependencyTracker     //////////////////////////
/////////////////////////////////////////////////////////////////////////

void VulkanCmdBuffer::dependencyFuckup()
{
  DependencyTracker tracker;

  int drawCallIndex = 0;
  m_commandList->foreach([&](VulkanCommandPacket* packet)
  {
    switch (packet->type())
    {
    case VulkanCommandPacket::PacketType::BufferCopy:
    {
      BufferCopyPacket* p = static_cast<BufferCopyPacket*>(packet);
      F_ASSERT(p->m_copyList.size() == 1, "Dependency tracker doesn't support more than 1 copy.");
      auto first = p->m_copyList[0];

      tracker.addDrawCall(drawCallIndex, "BufferCopy");
      tracker.addReadBuffer(drawCallIndex, p->src, first.srcOffset, first.size, vk::AccessFlagBits::eTransferRead);
      tracker.addModifyBuffer(drawCallIndex, p->dst, first.dstOffset, first.size, vk::AccessFlagBits::eTransferWrite);

      drawCallIndex++;
      break;
    }
    case VulkanCommandPacket::PacketType::Dispatch:
    {
      DispatchPacket* p = static_cast<DispatchPacket*>(packet);
      auto& bind = p->descriptors;

      tracker.addDrawCall(drawCallIndex, "Dispatch");
      for (auto&& it : bind.readBuffers)
      {
        tracker.addReadBuffer(drawCallIndex, it.second, vk::AccessFlagBits::eShaderRead);
      }
      for (auto&& it : bind.modifyBuffers)
      {
        tracker.addModifyBuffer(drawCallIndex, it.second, vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);
      }
      drawCallIndex++;
      break;
    }
    default:
      break;
    }
  });

  tracker.resolveGraph();
  tracker.printStuff([](std::string data)
  {
    F_LOG_UNFORMATTED("%s", data.c_str());
  });
}

