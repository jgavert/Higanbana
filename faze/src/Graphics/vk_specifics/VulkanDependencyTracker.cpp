#include "VulkanDependencyTracker.hpp"

void DependencyTracker::addDrawCall(int, DrawType name, vk::PipelineStageFlags baseFlags)
{
	m_drawCallInfo.emplace_back(name);
	m_drawCallStage.emplace_back(baseFlags);
	drawCallsAdded++;
}

void DependencyTracker::addReadBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
{
	auto uniqueID = buffer.uniqueId;
	m_lastReferenceToResource[uniqueID] = drawCallIndex;
  m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, ResourceType::buffer,
    flags, vk::ImageLayout::eUndefined, buffer.m_info.offset, buffer.m_info.range });
	if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
	{
		BufferDependency d;
		d.uniqueId = uniqueID;
		d.buffer = buffer.m_info.buffer;
		d.state = buffer.m_state;
		m_bufferStates[uniqueID] = std::move(d);
	}
  m_uniqueBuffersThisChain.insert(uniqueID);
}
void DependencyTracker::addModifyBuffer(int drawCallIndex, VulkanBufferShaderView& buffer, vk::AccessFlags flags)
{
	auto uniqueID = buffer.uniqueId;
	m_lastReferenceToResource[uniqueID] = drawCallIndex;
	m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, ResourceType::buffer,
    flags, vk::ImageLayout::eUndefined, buffer.m_info.offset, buffer.m_info.range });
	if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
	{
		BufferDependency d;
		d.uniqueId = uniqueID;
		d.buffer = buffer.m_info.buffer;
		d.state = buffer.m_state;
		m_bufferStates[uniqueID] = std::move(d);
	}
  m_uniqueBuffersThisChain.insert(uniqueID);
}

void DependencyTracker::addReadBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
{
	auto uniqueID = buffer.uniqueId;
	m_lastReferenceToResource[uniqueID] = drawCallIndex;
	m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, ResourceType::buffer,
    flags, vk::ImageLayout::eUndefined, offset, range });
	if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
	{
		BufferDependency d;
		d.uniqueId = uniqueID;
		d.buffer = *buffer.m_resource;
		d.state = buffer.m_state;
		m_bufferStates[uniqueID] = std::move(d);
	}
  m_uniqueBuffersThisChain.insert(uniqueID);
}
void DependencyTracker::addModifyBuffer(int drawCallIndex, VulkanBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, vk::AccessFlags flags)
{
	auto uniqueID = buffer.uniqueId;
	m_lastReferenceToResource[uniqueID] = drawCallIndex;
	m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, ResourceType::buffer,
    flags, vk::ImageLayout::eUndefined, offset, range });
	if (m_bufferStates.find(uniqueID) == m_bufferStates.end())
	{
		BufferDependency d;
		d.uniqueId = uniqueID;
		d.buffer = *buffer.m_resource;
		d.state = buffer.m_state;
		m_bufferStates[uniqueID] = std::move(d);
	}

  m_uniqueBuffersThisChain.insert(uniqueID);
}

void DependencyTracker::addReadBuffer(int drawCallIndex, VulkanTextureShaderView& texture, vk::AccessFlags flags)
{
  auto uniqueID = texture.uniqueId;
  m_lastReferenceToResource[uniqueID] = drawCallIndex;
  m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, ResourceType::texture,
    flags, texture.m_info.imageLayout, texture.m_subResourceRange.baseMipLevel, texture.m_subResourceRange.baseArrayLayer });

  if (m_textureStates.find(uniqueID) == m_textureStates.end())
  {
    TextureDependency d;
    d.uniqueId = uniqueID;
    d.texture = *texture.m_resource;
    d.state = texture.m_state;
    m_textureStates[uniqueID] = std::move(d);
  }

  m_uniqueTexturesThisChain.insert(uniqueID);
}
void DependencyTracker::addModifyBuffer(int drawCallIndex, VulkanTextureShaderView& texture, vk::AccessFlags flags)
{
  auto uniqueID = texture.uniqueId;
  m_lastReferenceToResource[uniqueID] = drawCallIndex;
  m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, ResourceType::texture,
    flags, texture.m_info.imageLayout, texture.m_subResourceRange.baseMipLevel, texture.m_subResourceRange.baseArrayLayer });

  if (m_textureStates.find(uniqueID) == m_textureStates.end())
  {
    TextureDependency d;
    d.uniqueId = uniqueID;
    d.texture = *texture.m_resource;
    d.state = texture.m_state;
    m_textureStates[uniqueID] = std::move(d);
  }

  m_uniqueTexturesThisChain.insert(uniqueID);
}


void DependencyTracker::addReadTexture(int drawCallIndex, VulkanTexture& texture, vk::AccessFlags flags, vk::ImageLayout layoutUsedInView, uint64_t miplevel, uint64_t arraylevel)
{
	// TODO: implement texture tracking and dependencies, it will just be added on top of buffer things
  auto uniqueID = texture.uniqueId;
  m_lastReferenceToResource[uniqueID] = drawCallIndex;
  m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::read, ResourceType::texture,
    flags, layoutUsedInView, miplevel, arraylevel });

  if (m_textureStates.find(uniqueID) == m_textureStates.end())
  {
    TextureDependency d;
    d.uniqueId = uniqueID;
    d.texture = *texture.m_resource;
    d.state = texture.m_state;
    m_textureStates[uniqueID] = std::move(d);
  }

  m_uniqueTexturesThisChain.insert(uniqueID);
}

void DependencyTracker::addWriteTexture(int drawCallIndex, VulkanTexture& texture, vk::AccessFlags flags, vk::ImageLayout layoutUsedInView, uint64_t miplevel, uint64_t arraylevel)
{
  auto uniqueID = texture.uniqueId;
  m_lastReferenceToResource[uniqueID] = drawCallIndex;
  m_jobs.emplace_back(DependencyPacket{ drawCallIndex, uniqueID, UsageHint::write, ResourceType::texture,
    flags, layoutUsedInView, miplevel, arraylevel});

  if (m_textureStates.find(uniqueID) == m_textureStates.end())
  {
    TextureDependency d;
    d.uniqueId = uniqueID;
    d.texture = *texture.m_resource;
    d.state = texture.m_state;
    m_textureStates[uniqueID] = std::move(d);
  }

  m_uniqueTexturesThisChain.insert(uniqueID);
}

// only builds the graph of dependencies.
void DependencyTracker::resolveGraph()
{
	auto currentSize = m_jobs.size();
	// create DAG(directed acyclic graph) from scratch
	m_schedulingResult.clear();
	//m_schedulingResult.reserve(m_jobs.size());

	m_cacheWrites.clear(); // resource producer list

	auto cacheContains = [&](ResourceUniqueId id) -> DrawCallIndex
	{
		auto found = std::find_if(m_cacheWrites.begin(), m_cacheWrites.end(), [&](const WriteCall& obj)
		{
			return obj.resource == id;
		});
		if (found != m_cacheWrites.end())
		{
			return found->draw;
		}
		return -1;
	};

	auto cacheInsertOrReplace = [&](ResourceUniqueId id, DrawCallIndex draw)
	{
		auto found = std::find_if(m_cacheWrites.begin(), m_cacheWrites.end(), [&](const WriteCall& obj)
		{
			return obj.resource == id;
		});
		if (found != m_cacheWrites.end())
		{
			found->draw = draw;
		}
		else
		{
			m_cacheWrites.emplace_back(WriteCall{ id, draw });
		}
	};

	int currentJobId = 0;
	for (int i = 0; i < static_cast<int>(currentSize); ++i)
	{
		auto& obj = m_jobs[i];
		currentJobId = obj.drawIndex;
		// find all resources?
		m_readRes.clear();
		// move 'i' to next object.
		for (; i < static_cast<int>(currentSize); ++i)
		{
			auto& job = m_jobs[i];
			if (job.drawIndex != currentJobId)
				break;
			if (job.hint == UsageHint::read)
			{
				m_readRes.push_back(i);
			}
			if (job.hint == UsageHint::write)
			{
				cacheInsertOrReplace(job.resource, job.drawIndex);
			}
		}
		--i; // backoff one, loop exits with extra appended value;

		ScheduleNode n;
		n.jobID = currentJobId;
		n.dependency = -1;
		if (m_readRes.empty()) // doesn't read any results of other jobs in this graph.
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
			for (auto&& it : m_readRes)
			{
				auto& readr = m_jobs[it].resource;
				auto found = cacheContains(readr);
				if (found != -1)
				{
					foundEvenOne = true;
					n.dependency = found;
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

void DependencyTracker::makeAllBarriers()
{
  /*
	int jobsSize = static_cast<int>(m_jobs.size());

	// this function should figure out transitions within a commandbuffer
	// We should use a small cache of currently "live" resouces that will be referenced in future
	// those that won't be, they should be evicted.
	// resources should contain "last drawcall where referenced"

	// so that we can build the barriers needed after first barrier
	// we just need to create barrier for first that should move all relevant resources to their next needed state
  m_bufferCache.clear();
  m_imageCache.clear();

	// fill cache with all resources seen.
	for (auto&& id : m_uniqueBuffersThisChain)
	{
		auto flags = m_bufferStates[id].state->flags;
		if (flags != vk::AccessFlags(vk::AccessFlagBits(0)))
      m_bufferCache[id] = SmallBuffer{ m_bufferStates[id].buffer, flags };
	}

  for (auto&& id : m_uniqueTexturesThisChain)
  {
    auto tesState = m_textureStates[id];
    auto flags = tesState.state->flags;
    //if (flags != vk::AccessFlags(vk::AccessFlagBits(0)))
    m_imageCache[id] = SmallTexture{ tesState.texture, flags, tesState.state->layout };
  }

	int i = 0;

	int bufferBarrierOffsets = 0;
	int imageBarrierOffsets = 0;
	while (i < jobsSize)
	{
		int draw = m_jobs[i].drawIndex;

		m_barrierOffsets.emplace_back(bufferBarrierOffsets);
    m_imageBarrierOffsets.emplace_back(imageBarrierOffsets);
		while (i < jobsSize && m_jobs[i].drawIndex == draw)
		{
      auto& job = m_jobs[i];
			auto jobResAccess = job.access;
      if (job.type == ResourceType::buffer)
      {
        auto resource = m_bufferCache.find(job.resource);
        if (resource != m_bufferCache.end())
        {
          auto lastAccess = resource->second.flags;
          if (jobResAccess != lastAccess)
          {
            aaargh.emplace_back(vk::BufferMemoryBarrier()
              .setSrcAccessMask(lastAccess)
              .setDstAccessMask(jobResAccess)
              .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setBuffer(resource->second.buffer)
              .setOffset(0)
              .setSize(VK_WHOLE_SIZE));
            resource->second.flags = jobResAccess;
            ++bufferBarrierOffsets;
          }
        }
        else // resource that we hadn't seen before
        {
          m_bufferCache[job.resource] = SmallBuffer{ m_bufferStates[job.resource].buffer, jobResAccess };
        }
      }
      else
      {
        auto resource = m_imageCache.find(job.resource);
        if (resource != m_imageCache.end())
        {
          auto lastAccess = resource->second.flags;
          if (jobResAccess != lastAccess)
          {
            imageBarriers.emplace_back(vk::ImageMemoryBarrier()
              .setSrcAccessMask(lastAccess)
              .setDstAccessMask(jobResAccess)
              .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setNewLayout(job.layout)
              .setOldLayout(resource->second.layout)
              .setImage(resource->second.image)
              .setSubresourceRange(vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(static_cast<uint32_t>(job.custom1))
                .setBaseArrayLayer(static_cast<uint32_t>(job.custom2))
                .setLevelCount(VK_REMAINING_MIP_LEVELS)
                .setLayerCount(VK_REMAINING_ARRAY_LAYERS)));
            resource->second.flags = jobResAccess;
            ++imageBarrierOffsets;
          }
        }
        else // resource that we hadn't seen before
        {
          m_imageCache[job.resource] = SmallTexture{ m_textureStates[job.resource].texture, jobResAccess, job.layout };
        }
			}
			++i;
		}
	}
	m_barrierOffsets.emplace_back(static_cast<int>(aaargh.size()));
  m_imageBarrierOffsets.emplace_back(static_cast<int>(imageBarriers.size()));

	// update global state
	for (auto&& obj : m_bufferCache)
	{
		m_bufferStates[obj.first].state->flags = obj.second.flags;
	}

  for (auto&& obj : m_imageCache)
  {
    m_textureStates[obj.first].state->flags = obj.second.flags;
    m_textureStates[obj.first].state->layout = obj.second.layout;
  }
  */
}

void DependencyTracker::runBarrier(vk::CommandBuffer gfx, int nextDrawCall)
{
	if (nextDrawCall == 0)
	{
		int barrierOffset = 0;
		int barrierSize = (m_barrierOffsets.size() > 1 && m_barrierOffsets[1] > 0) ? m_barrierOffsets[1] : static_cast<int>(aaargh.size());
		int imageBarrierOffset = 0;
		int imageBarrierSize = (m_imageBarrierOffsets.size() > 1 && m_imageBarrierOffsets[1] > 0) ? m_imageBarrierOffsets[1] : static_cast<int>(imageBarriers.size());
		if (barrierSize == 0 && imageBarrierSize == 0)
		{
			return;
		}
		vk::PipelineStageFlags last = vk::PipelineStageFlagBits::eAllCommands;
		vk::PipelineStageFlags next = m_drawCallStage[0];
		vk::ArrayProxy<const vk::MemoryBarrier> memory(0, 0);
		vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), aaargh.data() + barrierOffset);
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
	vk::ArrayProxy<const vk::BufferMemoryBarrier> buffer(static_cast<uint32_t>(barrierSize), aaargh.data() + barrierOffset);
	vk::ArrayProxy<const vk::ImageMemoryBarrier> image(static_cast<uint32_t>(imageBarrierSize), imageBarriers.data() + imageBarrierOffset);
	gfx.pipelineBarrier(last, next, vk::DependencyFlags(), memory, buffer, image);
}

void DependencyTracker::reset()
{
	m_drawCallInfo.clear();
	m_drawCallStage.clear();
	m_jobs.clear();
	m_lastReferenceToResource.clear();
	m_schedulingResult.clear();
	m_barrierOffsets.clear();
	aaargh.clear();
  m_uniqueBuffersThisChain.clear();

  m_imageBarrierOffsets.clear();
  imageBarriers.clear();
  m_uniqueTexturesThisChain.clear();
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
		tmp += drawTypeToString(m_drawCallInfo[it.jobID]);
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
		tmp += drawTypeToString(m_drawCallInfo[it.jobID]);
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

